#include "laser.hpp"
#include "../deserialize-utils.hpp"

namespace our
{

    void LaserComponent::deserialize(const nlohmann::json &data)
    {
        speed = data.value("speed", speed);
        lifetime = data.value("lifetime", lifetime);
    }

}
