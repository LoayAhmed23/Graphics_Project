#include "light.hpp"
#include "../deserialize-utils.hpp"

namespace our
{

    void LightComponent::deserialize(const nlohmann::json &data)
    {
        if (!data.is_object())
            return;

        // Deserialize light type
        std::string typeStr = data.value("lightType", "directional");
        if (typeStr == "directional")
        {
            lightType = LightType::DIRECTIONAL;
        }
        else if (typeStr == "point")
        {
            lightType = LightType::POINT;
        }
        else if (typeStr == "spot")
        {
            lightType = LightType::SPOT;
        }

        // Deserialize color (default white)
        color = data.value("color", glm::vec3(1.0f, 1.0f, 1.0f));

        // Deserialize direction (for directional/spot lights)
        direction = data.value("direction", glm::vec3(0.0f, 0.0f, 0.0f));

        // Deserialize position (for point/spot lights)
        position = data.value("position", glm::vec3(0.0f, 0.0f, 0.0f));

        // Deserialize attenuation (constant, linear, quadratic)
        attenuation = data.value("attenuation", glm::vec3(1.0f, 0.0f, 0.0f));

        // Deserialize cone angles (inner, outer) for spot lights
        // These can be specified in degrees in JSON and will be converted to radians
        if (data.contains("coneAngles"))
        {
            glm::vec2 angles = data["coneAngles"].get<glm::vec2>();
            // Convert degrees to radians
            coneAngles = glm::radians(angles);
        }
        else
        {
            // Default: 22.5 and 45 degrees
            coneAngles = glm::vec2(glm::radians(22.5f), glm::radians(45.0f));
        }
    }

}
