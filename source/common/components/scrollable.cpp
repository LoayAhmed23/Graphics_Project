#include "scrollable.hpp"

namespace our
{

    void ScrollableComponent::deserialize(const nlohmann::json &data)
    {
        roadLength = data.value("roadLength", roadLength);
    }

}
