#include "entity.hpp"
#include "../deserialize-utils.hpp"

#include <glm/gtx/euler_angles.hpp>

namespace our {

    // This function computes and returns a matrix that represents this transform
    // Remember that the order of transformations is: Scaling, Rotation then Translation
    // HINT: to convert euler angles to a rotation matrix, you can use glm::yawPitchRoll
    glm::mat4 Transform::toMat4() const {
        //TODO: (Req 3) Write this function
        // 1. Create the translation matrix
        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
        
        // 2. Create the rotation matrix using yaw, pitch, and roll
        // The header file specifies rotation is (y: yaw, x: pitch, z: roll)
        glm::mat4 R = glm::yawPitchRoll(rotation.y, rotation.x, rotation.z);
        
        // 3. Create the scale matrix
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        
        // 4. Combine them in T * R * S order
        // This means the vertex will first be scaled, then rotated, then translated.
        return T * R * S;
        //return glm::mat4(1.0f); 
    }

     // Deserializes the entity data and components from a json object
    void Transform::deserialize(const nlohmann::json& data){
        position = data.value("position", position);
        rotation = glm::radians(data.value("rotation", glm::degrees(rotation)));
        scale    = data.value("scale", scale);
    }

}