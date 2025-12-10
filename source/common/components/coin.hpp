#pragma once

#include "../ecs/component.hpp"

namespace our
{

    // This component marks an entity as a collectible coin
    class CoinComponent : public Component
    {
    public:
        int value = 1;               // Score value when collected
        float rotationSpeed = 90.0f; // Rotation speed in degrees per second

        static std::string getID() { return "Coin"; }

        void deserialize(const nlohmann::json &data) override;
    };

}
