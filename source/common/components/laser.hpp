#pragma once

#include "../ecs/component.hpp"

namespace our
{

    // This component marks an entity as a laser beam fired by the player
    class LaserComponent : public Component
    {
    public:
        float speed = 100.0f;     // Speed at which laser travels (negative Z direction)
        float lifetime = 2.0f;    // Maximum lifetime in seconds before despawn
        float currentLife = 0.0f; // Current time alive

        static std::string getID() { return "Laser"; }

        void deserialize(const nlohmann::json &data) override;
    };

}
