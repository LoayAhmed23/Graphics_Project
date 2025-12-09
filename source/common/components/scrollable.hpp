#pragma once

#include "../ecs/component.hpp"

namespace our
{

    // This component marks an entity as scrollable (moves with the road)
    class ScrollableComponent : public Component
    {
    public:
        float roadLength = 100.0f; // Total length of the road segment

        static std::string getID() { return "Scrollable"; }

        void deserialize(const nlohmann::json &data) override;
    };

}
