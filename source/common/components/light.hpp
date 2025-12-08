#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace our
{

    // Light type enumeration
    enum class LightType
    {
        DIRECTIONAL = 0,
        POINT = 1,
        SPOT = 2
    };

    // Light component that can be attached to entities to create light sources
    class LightComponent : public Component
    {
    public:
        LightType lightType = LightType::DIRECTIONAL;

        // Light color/intensity (RGB)
        glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

        // Direction for directional/spot lights (in world space, direction FROM light)
        // If zero, will be computed from entity's transform (-Z forward)
        glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);

        // Position for point/spot lights (in world space)
        // If zero, will be extracted from entity's transform
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);

        // Attenuation coefficients (constant, linear, quadratic)
        // Attenuation = 1 / (constant + linear*d + quadratic*d²)
        glm::vec3 attenuation = glm::vec3(1.0f, 0.0f, 0.0f); // No attenuation by default

        // Cone angles for spot lights (inner, outer) in radians
        // Inner cone: full intensity
        // Outer cone: falloff to zero
        glm::vec2 coneAngles = glm::vec2(glm::quarter_pi<float>() * 0.5f, glm::half_pi<float>() * 0.5f);

        // The ID of this component type
        static std::string getID() { return "Light"; }

        // Deserialize from JSON
        void deserialize(const nlohmann::json &data) override;
    };

}
