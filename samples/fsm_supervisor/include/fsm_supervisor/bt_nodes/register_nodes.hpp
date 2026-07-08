#pragma once

#include <behaviortree_cpp_v3/bt_factory.h>

namespace fsm_supervisor::bt_nodes
{

/**
 * 注册本示例使用到的所有 BT 节点类型。
 *
 * 把注册集中在一个函数里，可以让 main/supervisor 不直接依赖具体节点类。
 * 如果后续改成插件版，这个函数通常会放到 BT_REGISTER_NODES 里调用。
 */
void registerNodes(BT::BehaviorTreeFactory& factory);

}  // namespace fsm_supervisor::bt_nodes
