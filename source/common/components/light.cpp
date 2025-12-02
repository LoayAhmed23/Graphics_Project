#include "light.hpp"
#include "../deserialize-utils.hpp"

namespace our
{

    // Reads the light component data from a json object
    void LightComponent::deserialize(const nlohmann::json &data)
    {
        if (!data.is_object())
            return;

        // Read light type
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

        // Read color
        color = data.value("color", glm::vec3(1.0f, 1.0f, 1.0f));

        // Read cone angles (for spot lights) - convert from degrees to radians
        if (data.contains("coneAngles"))
        {
            glm::vec2 angles = data["coneAngles"];
            coneAngles = glm::vec2(glm::radians(angles.x), glm::radians(angles.y));
        }
        else
        {
            coneAngles = glm::vec2(glm::radians(30.0f), glm::radians(45.0f));
        }

        // Read attenuation coefficients
        attenuation = data.value("attenuation", glm::vec3(1.0f, 0.0f, 0.0f));

        // Read optional explicit direction (for directional/spot lights)
        direction = data.value("direction", glm::vec3(0.0f));

        // Read optional explicit position (for point/spot lights)
        position = data.value("position", glm::vec3(0.0f));
    }

}
