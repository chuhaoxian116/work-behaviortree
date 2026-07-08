#include "fsm_supervisor/supervisor/robot_state.hpp"

namespace fsm_supervisor
{

const char* toString(RobotState state)
{
    switch(state)
    {
        case RobotState::IDLE:
            return "IDLE";
        case RobotState::GRASPING:
            return "GRASPING";
        case RobotState::ERROR:
            return "ERROR";
        case RobotState::RECOVERING:
            return "RECOVERING";
    }
    return "UNKNOWN";
}

const char* toString(BT::NodeStatus status)
{
    switch(status)
    {
        case BT::NodeStatus::IDLE:
            return "IDLE";
        case BT::NodeStatus::RUNNING:
            return "RUNNING";
        case BT::NodeStatus::SUCCESS:
            return "SUCCESS";
        case BT::NodeStatus::FAILURE:
            return "FAILURE";
    }
    return "UNKNOWN";
}

}  // namespace fsm_supervisor
