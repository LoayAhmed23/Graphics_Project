#pragma once

#include "../ecs/component.hpp"

namespace our
{

    // This component marks an entity as a player-controlled car
    class CarControllerComponent : public Component
    {
    public:
        float speed = 10.0f; // Movement speed
        float minX = -12.0f; // Left road boundary
        float maxX = 9.0f;   // Right road boundary

        static std::string getID() { return "Car Controller"; }

        void deserialize(const nlohmann::json &data) override;
    };

}
