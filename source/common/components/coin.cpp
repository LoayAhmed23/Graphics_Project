#include "coin.hpp"
#include "../deserialize-utils.hpp"

namespace our
{

    void CoinComponent::deserialize(const nlohmann::json &data)
    {
        value = data.value("value", value);
        rotationSpeed = data.value("rotationSpeed", rotationSpeed);
    }

}
