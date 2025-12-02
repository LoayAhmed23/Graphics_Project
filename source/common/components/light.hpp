#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>

namespace our
{

    // Enum for light types
    enum class LightType
    {
        DIRECTIONAL = 0,
        POINT = 1,
        SPOT = 2
    };

    // This component represents a light source in the scene
    // Position and Direction can be either from TransformComponent OR specified directly
    class LightComponent : public Component
    {
    public:
        LightType lightType;   // Type of light: Directional, Point, or Spot
        glm::vec3 color;       // Light color (RGB)
        glm::vec2 coneAngles;  // For spot lights: (inner angle, outer angle) in radians
        glm::vec3 attenuation; // Attenuation coefficients (constant, linear, quadratic)
        glm::vec3 direction;   // Optional explicit direction (if not zero, overrides transform)
        glm::vec3 position;    // Optional explicit position (if not zero, overrides transform)

        // The ID of this component type is "Light"
        static std::string getID() { return "Light"; }

        // Constructor with default values
        LightComponent()
        {
            lightType = LightType::DIRECTIONAL;
            color = glm::vec3(1.0f, 1.0f, 1.0f);
            coneAngles = glm::vec2(glm::radians(30.0f), glm::radians(45.0f)); // Default spot light angles
            attenuation = glm::vec3(1.0f, 0.0f, 0.0f);                        // No attenuation by default
            direction = glm::vec3(0.0f);                                       // Zero means use transform
            position = glm::vec3(0.0f);                                        // Zero means use transform
        }

        // Reads the light data from a json object
        void deserialize(const nlohmann::json &data) override;
    };

}
