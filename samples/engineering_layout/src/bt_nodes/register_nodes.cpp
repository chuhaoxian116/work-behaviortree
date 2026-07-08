#include "engineering_layout/bt_nodes/register_nodes.hpp"

#include "engineering_layout/bt_nodes/battery_ok.hpp"
#include "engineering_layout/bt_nodes/move_to.hpp"
#include "engineering_layout/bt_nodes/say.hpp"

namespace engineering_layout
{

void registerNodes(BT::BehaviorTreeFactory& factory)
{
    factory.registerNodeType<BatteryOK>("BatteryOK");
    factory.registerNodeType<MoveTo>("MoveTo");
    factory.registerNodeType<Say>("Say");
}

}  // namespace engineering_layout
