#include "enemy-car.hpp"
#include "../deserialize-utils.hpp"

namespace our
{

    void EnemyCarComponent::deserialize(const nlohmann::json &data)
    {
        speed = data.value("speed", speed);
    }

}
