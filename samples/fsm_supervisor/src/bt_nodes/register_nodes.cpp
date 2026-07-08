#include "fsm_supervisor/bt_nodes/register_nodes.hpp"

#include "fsm_supervisor/bt_nodes/grasp_nodes.hpp"
#include "fsm_supervisor/bt_nodes/recover_nodes.hpp"

namespace fsm_supervisor::bt_nodes
{

/**
 * 注册本示例中的抓取节点和恢复节点。
 */
void registerNodes(BT::BehaviorTreeFactory& factory)
{
    factory.registerNodeType<DetectCloth>("DetectCloth");
    factory.registerNodeType<MoveToCloth>("MoveToCloth");
    factory.registerNodeType<CloseGripper>("CloseGripper");

    factory.registerNodeType<ResetMotor>("ResetMotor");
    factory.registerNodeType<ClearHardwareError>("ClearHardwareError");
}

}  // namespace fsm_supervisor::bt_nodes
