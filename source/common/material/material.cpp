#include "material.hpp"

#include "../asset-loader.hpp"
#include "deserialize-utils.hpp"

namespace our {

    // This function should setup the pipeline state and set the shader to be used
    void Material::setup() const {
        //TODO: (Req 7) Write this function
        // 1. Setup the pipeline state
        pipelineState.setup();
        // 2. Use the shader program
        shader->use();
    }

    // This function read the material data from a json object
    void Material::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;

        if(data.contains("pipelineState")){
            pipelineState.deserialize(data["pipelineState"]);
        }
        shader = AssetLoader<ShaderProgram>::get(data["shader"].get<std::string>());
        transparent = data.value("transparent", false);
    }

    // This function should call the setup of its parent and
    // set the "tint" uniform to the value in the member variable tint 
    void TintedMaterial::setup() const {
        //TODO: (Req 7) Write this function
        // 1. Call the parent's setup
        Material::setup();
        // 2. Set the "tint" uniform
        shader->set("tint", tint);
    }

    // This function read the material data from a json object
    void TintedMaterial::deserialize(const nlohmann::json& data){
        Material::deserialize(data);
        if(!data.is_object()) return;
        tint = data.value("tint", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // This function should call the setup of its parent and
    // set the "alphaThreshold" uniform to the value in the member variable alphaThreshold
    // Then it should bind the texture and sampler to a texture unit and send the unit number to the uniform variable "tex" 
    void TexturedMaterial::setup() const {
        //TODO: (Req 7) Write this function
        // 1. Call the parent's setup (which sets the tint)
        TintedMaterial::setup();
        
        // 2. Set the "alphaThreshold" uniform
        shader->set("alphaThreshold", alphaThreshold);

        // 3. Set the texture and sampler
        // We'll use texture unit 0
        glActiveTexture(GL_TEXTURE0);

        // If the texture is not null, bind it
        if (texture) {
            texture->bind();
        } else {
            // Otherwise, unbind the texture unit
            Texture2D::unbind();
        }

        // If the sampler is not null, bind it
        if (sampler) {
            sampler->bind(0); // Bind to texture unit 0
        } else {
            // Otherwise, unbind the sampler from the unit
            Sampler::unbind(0);
        }

        // 4. Send the texture unit index (0) to the "tex" uniform
        shader->set("tex", 0);
    }

    // This function read the material data from a json object
    void TexturedMaterial::deserialize(const nlohmann::json& data){
        TintedMaterial::deserialize(data);
        if(!data.is_object()) return;
        alphaThreshold = data.value("alphaThreshold", 0.0f);
        texture = AssetLoader<Texture2D>::get(data.value("texture", ""));
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
    }

}