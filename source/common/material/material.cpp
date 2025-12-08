#include "material.hpp"

#include "../asset-loader.hpp"
#include "deserialize-utils.hpp"

namespace our
{

    // This function should setup the pipeline state and set the shader to be used
    void Material::setup() const
    {
        // TODO: (Req 7) Write this function
        //  1. Setup the pipeline state
        pipelineState.setup();
        // 2. Use the shader program
        shader->use();
    }

    // This function read the material data from a json object
    void Material::deserialize(const nlohmann::json &data)
    {
        if (!data.is_object())
            return;

        if (data.contains("pipelineState"))
        {
            pipelineState.deserialize(data["pipelineState"]);
        }
        shader = AssetLoader<ShaderProgram>::get(data["shader"].get<std::string>());
        transparent = data.value("transparent", false);
    }

    // This function should call the setup of its parent and
    // set the "tint" uniform to the value in the member variable tint
    void TintedMaterial::setup() const
    {
        // TODO: (Req 7) Write this function
        //  1. Call the parent's setup
        Material::setup();
        // 2. Set the "tint" uniform
        shader->set("tint", tint);
    }

    // This function read the material data from a json object
    void TintedMaterial::deserialize(const nlohmann::json &data)
    {
        Material::deserialize(data);
        if (!data.is_object())
            return;
        tint = data.value("tint", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // This function should call the setup of its parent and
    // set the "alphaThreshold" uniform to the value in the member variable alphaThreshold
    // Then it should bind the texture and sampler to a texture unit and send the unit number to the uniform variable "tex"
    void TexturedMaterial::setup() const
    {
        // TODO: (Req 7) Write this function
        //  1. Call the parent's setup (which sets the tint)
        TintedMaterial::setup();

        // 2. Set the "alphaThreshold" uniform
        shader->set("alphaThreshold", alphaThreshold);

        // 3. Set the texture and sampler
        // We'll use texture unit 0
        glActiveTexture(GL_TEXTURE0);

        // If the texture is not null, bind it
        if (texture)
        {
            texture->bind();
        }
        else
        {
            // Otherwise, unbind the texture unit
            Texture2D::unbind();
        }

        // If the sampler is not null, bind it
        if (sampler)
        {
            sampler->bind(0); // Bind to texture unit 0
        }
        else
        {
            // Otherwise, unbind the sampler from the unit
            Sampler::unbind(0);
        }

        // 4. Send the texture unit index (0) to the "tex" uniform
        shader->set("tex", 0);
    }

    // This function read the material data from a json object
    void TexturedMaterial::deserialize(const nlohmann::json &data)
    {
        TintedMaterial::deserialize(data);
        if (!data.is_object())
            return;
        alphaThreshold = data.value("alphaThreshold", 0.0f);
        texture = AssetLoader<Texture2D>::get(data.value("texture", ""));
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
    }

    // LitMaterial implementation
    void LitMaterial::setup() const
    {
        // Call parent's setup (handles pipeline state, shader, texture, tint, alphaThreshold)
        TexturedMaterial::setup();

        // Set material lighting properties
        shader->set("material.ambient", ambient);
        shader->set("material.diffuse", diffuse);
        shader->set("material.specular", specular);
        shader->set("material.shininess", shininess);

        // Set global ambient light
        shader->set("ambientLight", ambientLight);

        // Set emission color
        shader->set("emissive_color", emissive_color);

        // Bind specular map (texture unit 1)
        if (specular_map)
        {
            glActiveTexture(GL_TEXTURE1);
            specular_map->bind();
            if (sampler)
                sampler->bind(1);
            shader->set("specular_map", 1);
            shader->set("use_specular_map", true);
        }
        else
        {
            shader->set("use_specular_map", false);
        }

        // Bind roughness map (texture unit 2)
        if (roughness_map)
        {
            glActiveTexture(GL_TEXTURE2);
            roughness_map->bind();
            if (sampler)
                sampler->bind(2);
            shader->set("roughness_map", 2);
            shader->set("use_roughness_map", true);
        }
        else
        {
            shader->set("use_roughness_map", false);
        }

        // Bind ambient occlusion map (texture unit 3)
        if (ambient_occlusion_map)
        {
            glActiveTexture(GL_TEXTURE3);
            ambient_occlusion_map->bind();
            if (sampler)
                sampler->bind(3);
            shader->set("ambient_occlusion_map", 3);
            shader->set("use_ao_map", true);
        }
        else
        {
            shader->set("use_ao_map", false);
        }

        // Bind emissive map (texture unit 4)
        if (emissive_map)
        {
            glActiveTexture(GL_TEXTURE4);
            emissive_map->bind();
            if (sampler)
                sampler->bind(4);
            shader->set("emissive_map", 4);
            shader->set("use_emissive_map", true);
        }
        else
        {
            shader->set("use_emissive_map", false);
        }

        // Reset active texture to unit 0
        glActiveTexture(GL_TEXTURE0);
    }

    void LitMaterial::deserialize(const nlohmann::json &data)
    {
        // Call parent's deserialize (handles shader, tint, texture, sampler, alphaThreshold)
        TexturedMaterial::deserialize(data);
        if (!data.is_object())
            return;

        // Deserialize Phong material properties
        ambient = data.value("ambient", glm::vec3(0.1f, 0.1f, 0.1f));
        diffuse = data.value("diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
        specular = data.value("specular", glm::vec3(0.5f, 0.5f, 0.5f));
        shininess = data.value("shininess", 32.0f);

        // Deserialize global ambient light
        ambientLight = data.value("ambientLight", glm::vec3(0.1f, 0.1f, 0.1f));

        // Deserialize emission color
        emissive_color = data.value("emissive_color", glm::vec3(0.0f, 0.0f, 0.0f));

        // Load texture maps
        specular_map = AssetLoader<Texture2D>::get(data.value("specular_map", ""));
        roughness_map = AssetLoader<Texture2D>::get(data.value("roughness_map", ""));
        ambient_occlusion_map = AssetLoader<Texture2D>::get(data.value("ambient_occlusion_map", ""));
        emissive_map = AssetLoader<Texture2D>::get(data.value("emissive_map", ""));
    }

}