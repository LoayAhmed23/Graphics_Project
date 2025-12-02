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


    // LitMaterial setup - binds all PBR textures to their respective texture units
    void LitMaterial::setup() const {
        // Setup pipeline state and shader
        Material::setup();
        
        int textureUnit = 0;
        
        // Bind albedo texture to unit 0
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        if (albedo) {
            albedo->bind();
        } else {
            Texture2D::unbind();
        }
        shader->set("material.albedo", textureUnit);
        textureUnit++;
        
        // Bind specular texture to unit 1
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        if (specular) {
            specular->bind();
        } else {
            Texture2D::unbind();
        }
        shader->set("material.specular", textureUnit);
        textureUnit++;
        
        // Bind roughness texture to unit 2
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        if (roughness) {
            roughness->bind();
        } else {
            Texture2D::unbind();
        }
        shader->set("material.roughness", textureUnit);
        textureUnit++;
        
        // Bind ambient occlusion texture to unit 3
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        if (ambient_occlusion) {
            ambient_occlusion->bind();
        } else {
            Texture2D::unbind();
        }
        shader->set("material.ambient_occlusion", textureUnit);
        textureUnit++;
        
        // Bind emission texture to unit 4
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        if (emission) {
            emission->bind();
        } else {
            Texture2D::unbind();
        }
        shader->set("material.emission", textureUnit);
        textureUnit++;
        
        // Bind sampler to all texture units
        if (sampler) {
            for (int i = 0; i < textureUnit; i++) {
                sampler->bind(i);
            }
        }
        
        // Set material properties
        shader->set("material.albedoTint", albedoTint);
        shader->set("material.specularStrength", specularStrength);
        shader->set("material.roughnessValue", roughnessValue);
        shader->set("material.emissionTint", emissionTint);
    }

    // Deserialize LitMaterial from JSON
    void LitMaterial::deserialize(const nlohmann::json& data){
        Material::deserialize(data);
        if(!data.is_object()) return;
        
        // Load textures
        albedo = AssetLoader<Texture2D>::get(data.value("albedo", ""));
        specular = AssetLoader<Texture2D>::get(data.value("specular", ""));
        roughness = AssetLoader<Texture2D>::get(data.value("roughness", ""));
        ambient_occlusion = AssetLoader<Texture2D>::get(data.value("ambient_occlusion", ""));
        emission = AssetLoader<Texture2D>::get(data.value("emission", ""));
        
        // Load sampler
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
        
        // Load material properties
        albedoTint = data.value("albedoTint", glm::vec3(1.0f));
        specularStrength = data.value("specularStrength", 1.0f);
        roughnessValue = data.value("roughnessValue", 0.5f);
        emissionTint = data.value("emissionTint", glm::vec3(0.0f));
    }

}
