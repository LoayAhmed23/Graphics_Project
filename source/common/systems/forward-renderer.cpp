#include "forward-renderer.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"
#include "../components/light.hpp"

namespace our
{

    void ForwardRenderer::setupLightingUniforms(World *world, ShaderProgram *shader, const glm::vec3 &cameraPosition)
    {
        // Query all entities with light components
        std::vector<LightComponent *> lights;
        for (auto entity : world->getEntities())
        {
            if (auto light = entity->getComponent<LightComponent>())
            {
                lights.push_back(light);
                if (lights.size() >= 16)
                    break; // MAX_LIGHTS
            }
        }

        // Set light count
        shader->set("lightCount", static_cast<int>(lights.size()));

        // Set camera position for specular calculations
        shader->set("cameraPosition", cameraPosition);

        // Upload each light's data to the shader
        for (size_t i = 0; i < lights.size(); i++)
        {
            LightComponent *light = lights[i];
            Entity *lightEntity = light->getOwner();

            // Use explicit direction/position if provided, otherwise extract from transform
            glm::vec3 position;
            glm::vec3 direction;

            if (glm::length(light->direction) > 0.001f)
            {
                // Use explicit direction
                direction = glm::normalize(light->direction);
            }
            else
            {
                // Extract direction from transform
                glm::mat4 lightTransform = lightEntity->getLocalToWorldMatrix();
                direction = glm::normalize(glm::vec3(lightTransform * glm::vec4(0, 0, -1, 0)));
            }

            if (glm::length(light->position) > 0.001f)
            {
                // Use explicit position
                position = light->position;
            }
            else
            {
                // Extract position from transform
                glm::mat4 lightTransform = lightEntity->getLocalToWorldMatrix();
                position = glm::vec3(lightTransform[3]);
            }

            // Build uniform name prefix
            std::string prefix = "lights[" + std::to_string(i) + "].";

            // Set light properties
            shader->set(prefix + "type", static_cast<int>(light->lightType));
            shader->set(prefix + "color", light->color);
            shader->set(prefix + "position", position);
            shader->set(prefix + "direction", direction);
            shader->set(prefix + "coneAngles", light->coneAngles);
            shader->set(prefix + "attenuation", light->attenuation);
        }
    }

    void ForwardRenderer::initialize(glm::ivec2 windowSize, const nlohmann::json &config)
    {
        // First, we store the window size for later use
        this->windowSize = windowSize;

        // Then we check if there is a sky texture in the configuration
        if (config.contains("sky"))
        {
            // First, we create a sphere which will be used to draw the sky
            this->skySphere = mesh_utils::sphere(glm::ivec2(16, 16));

            // We can draw the sky using the same shader used to draw textured objects
            ShaderProgram *skyShader = new ShaderProgram();
            skyShader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
            skyShader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
            skyShader->link();

            // TODO: (Req 10) Pick the correct pipeline state to draw the sky
            //  Hints: the sky will be draw after the opaque objects so we would need depth testing but which depth funtion should we pick?
            //  We will draw the sphere from the inside, so what options should we pick for the face culling.
            PipelineState skyPipelineState;
            skyPipelineState.depthTesting.enabled = true;
            skyPipelineState.depthTesting.function = GL_LEQUAL;
            skyPipelineState.faceCulling.enabled = true;
            skyPipelineState.faceCulling.culledFace = GL_FRONT;
            skyPipelineState.faceCulling.frontFace = GL_CCW;

            // Load the sky texture (note that we don't need mipmaps since we want to avoid any unnecessary blurring while rendering the sky)
            std::string skyTextureFile = config.value<std::string>("sky", "");
            Texture2D *skyTexture = texture_utils::loadImage(skyTextureFile, false);

            // Setup a sampler for the sky
            Sampler *skySampler = new Sampler();
            skySampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_WRAP_S, GL_REPEAT);
            skySampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Combine all the aforementioned objects (except the mesh) into a material
            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = skyShader;
            this->skyMaterial->texture = skyTexture;
            this->skyMaterial->sampler = skySampler;
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 1.0f;
            this->skyMaterial->transparent = false;
        }

        // Then we check if there is a postprocessing shader in the configuration
        if (config.contains("postprocess"))
        {
            // TODO: (Req 11) Create a framebuffer
            glGenFramebuffers(1, &postprocessFrameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);

            // TODO: (Req 11) Create a color and a depth texture and attach them to the framebuffer
            //  Hints: The color format can be (Red, Green, Blue and Alpha components with 8 bits for each channel).
            //  The depth format can be (Depth component with 24 bits).
            colorTarget = texture_utils::empty(GL_RGBA8, windowSize);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTarget->getOpenGLName(), 0);

            depthTarget = texture_utils::empty(GL_DEPTH_COMPONENT24, windowSize);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTarget->getOpenGLName(), 0);

            // TODO: (Req 11) Unbind the framebuffer just to be safe
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Create a vertex array to use for drawing the texture
            glGenVertexArrays(1, &postProcessVertexArray);

            // Create a sampler to use for sampling the scene texture in the post processing shader
            Sampler *postprocessSampler = new Sampler();
            postprocessSampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            postprocessSampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create the post processing shader
            ShaderProgram *postprocessShader = new ShaderProgram();
            postprocessShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            postprocessShader->attach(config.value<std::string>("postprocess", ""), GL_FRAGMENT_SHADER);
            postprocessShader->link();

            // Create a post processing material
            postprocessMaterial = new TexturedMaterial();
            postprocessMaterial->shader = postprocessShader;
            postprocessMaterial->texture = colorTarget;
            postprocessMaterial->sampler = postprocessSampler;
            // The default options are fine but we don't need to interact with the depth buffer
            // so it is more performant to disable the depth mask
            postprocessMaterial->pipelineState.depthMask = false;
        }
    }

    void ForwardRenderer::destroy()
    {
        // Delete all objects related to the sky
        if (skyMaterial)
        {
            delete skySphere;
            delete skyMaterial->shader;
            delete skyMaterial->texture;
            delete skyMaterial->sampler;
            delete skyMaterial;
        }
        // Delete all objects related to post processing
        if (postprocessMaterial)
        {
            glDeleteFramebuffers(1, &postprocessFrameBuffer);
            glDeleteVertexArrays(1, &postProcessVertexArray);
            delete colorTarget;
            delete depthTarget;
            delete postprocessMaterial->sampler;
            delete postprocessMaterial->shader;
            delete postprocessMaterial;
        }
    }

    void ForwardRenderer::render(World *world)
    {
        // First of all, we search for a camera and for all the mesh renderers
        CameraComponent *camera = nullptr;
        opaqueCommands.clear();
        transparentCommands.clear();
        for (auto entity : world->getEntities())
        {
            // If we hadn't found a camera yet, we look for a camera in this entity
            if (!camera)
                camera = entity->getComponent<CameraComponent>();
            // If this entity has a mesh renderer component
            if (auto meshRenderer = entity->getComponent<MeshRendererComponent>(); meshRenderer)
            {
                // We construct a command from it
                RenderCommand command;
                command.localToWorld = meshRenderer->getOwner()->getLocalToWorldMatrix();
                command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                command.mesh = meshRenderer->mesh;
                command.material = meshRenderer->material;
                // if it is transparent, we add it to the transparent commands list
                if (command.material->transparent)
                {
                    transparentCommands.push_back(command);
                }
                else
                {
                    // Otherwise, we add it to the opaque command list
                    opaqueCommands.push_back(command);
                }
            }
        }

        // If there is no camera, we return (we cannot render without a camera)
        if (camera == nullptr)
            return;

        // TODO: (Req 9) Modify the following line such that "cameraForward" contains a vector pointing the camera forward direction
        //  HINT: See how you wrote the CameraComponent::getViewMatrix, it should help you solve this one
        glm::vec3 cameraForward = camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0.0, 0.0, -1.0f, 0.0);

        std::sort(transparentCommands.begin(), transparentCommands.end(), [cameraForward](const RenderCommand &first, const RenderCommand &second)
                  {
            //TODO: (Req 9) Finish this function
            // HINT: the following return should return true "first" should be drawn before "second". 
            // Objects farther from camera should be drawn first (back to front)
            float firstDist = glm::dot(first.center, cameraForward);
            float secondDist = glm::dot(second.center, cameraForward);
            return firstDist > secondDist; });

        // TODO: (Req 9) Get the camera ViewProjection matrix and store it in VP
        glm::mat4 VP = camera->getProjectionMatrix(windowSize) * camera->getViewMatrix();

        // TODO: (Req 9) Set the OpenGL viewport using viewportStart and viewportSize
        glViewport(0, 0, windowSize.x, windowSize.y);

        // TODO: (Req 9) Set the clear color to black and the clear depth to 1
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);

        // TODO: (Req 9) Set the color mask to true and the depth mask to true (to ensure the glClear will affect the framebuffer)
        glColorMask(true, true, true, true);
        glDepthMask(true);

        // If there is a postprocess material, bind the framebuffer
        if (postprocessMaterial)
        {
            // TODO: (Req 11) bind the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);
        }

        // TODO: (Req 9) Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get camera position for lighting calculations
        glm::vec3 cameraPosition = glm::vec3(camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

        // TODO: (Req 9) Draw all the opaque commands
        //  Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        for (const auto &command : opaqueCommands)
        {
            command.material->setup();

            // Check if this is a lit material and setup lighting
            LitMaterial *litMaterial = dynamic_cast<LitMaterial *>(command.material);
            if (litMaterial)
            {
                // For lit materials, set model, view, projection matrices separately
                glm::mat4 view = camera->getViewMatrix();
                glm::mat4 projection = camera->getProjectionMatrix(windowSize);

                litMaterial->shader->set("model", command.localToWorld);
                litMaterial->shader->set("view", view);
                litMaterial->shader->set("projection", projection);
                litMaterial->shader->set("model_inverse_transpose", glm::transpose(glm::inverse(command.localToWorld)));

                // Setup lighting uniforms
                setupLightingUniforms(world, litMaterial->shader, cameraPosition);
            }
            else
            {
                // For non-lit materials, use the combined transform
                glm::mat4 transform = VP * command.localToWorld;
                command.material->shader->set("transform", transform);
            }

            command.mesh->draw();
        }

        // For requirement 10, setup sky rendering
        if (this->skyMaterial)
        {
            // Setup sky material
            this->skyMaterial->setup();

            // Get camera position
            glm::vec3 cameraPosition = glm::vec3(camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

            // Create sky model matrix that follows camera
            glm::mat4 skyModelMatrix = glm::translate(glm::mat4(1.0f), cameraPosition);

            // Update the alwaysBehindTransform matrix to ensure sky is drawn at maximum depth
            glm::mat4 alwaysBehindTransform = glm::mat4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 1.0f);

            // Set transform uniform for sky
            glm::mat4 skyTransform = alwaysBehindTransform * VP * skyModelMatrix;
            skyMaterial->shader->set("transform", skyTransform);

            // Draw sky sphere
            skySphere->draw();
        }

        // TODO: (Req 9) Draw all the transparent commands
        //  Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        for (const auto &command : transparentCommands)
        {
            command.material->setup();

            // Check if this is a lit material and setup lighting
            LitMaterial *litMaterial = dynamic_cast<LitMaterial *>(command.material);
            if (litMaterial)
            {
                // For lit materials, set model, view, projection matrices separately
                glm::mat4 view = camera->getViewMatrix();
                glm::mat4 projection = camera->getProjectionMatrix(windowSize);

                litMaterial->shader->set("model", command.localToWorld);
                litMaterial->shader->set("view", view);
                litMaterial->shader->set("projection", projection);
                litMaterial->shader->set("model_inverse_transpose", glm::transpose(glm::inverse(command.localToWorld)));

                // Setup lighting uniforms
                setupLightingUniforms(world, litMaterial->shader, cameraPosition);
            }
            else
            {
                // For non-lit materials, use the combined transform
                glm::mat4 transform = VP * command.localToWorld;
                command.material->shader->set("transform", transform);
            }

            command.mesh->draw();
        }

        // If there is a postprocess material, apply postprocessing
        if (postprocessMaterial)
        {
            // TODO: (Req 11) Return to the default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // TODO: (Req 11) Setup the postprocess material and draw the fullscreen triangle
            postprocessMaterial->setup();
            glBindVertexArray(postProcessVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

}