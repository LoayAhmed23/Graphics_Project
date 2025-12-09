#include "car-controller.hpp"
#include "../deserialize-utils.hpp"

namespace our
{

    void CarControllerComponent::deserialize(const nlohmann::json &data)
    {
        speed = data.value("speed", speed);
        minX = data.value("minX", minX);
        maxX = data.value("maxX", maxX);
    }

}
