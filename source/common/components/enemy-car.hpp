#pragma once

#include "../ecs/component.hpp"

namespace our
{

    // This component marks an entity as an enemy car that moves toward the player
    class EnemyCarComponent : public Component
    {
    public:
        float speed = 20.0f; // Speed at which enemy moves toward player

        static std::string getID() { return "Enemy Car"; }

        void deserialize(const nlohmann::json &data) override;
    };

}
