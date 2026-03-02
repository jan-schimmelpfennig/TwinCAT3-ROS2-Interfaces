//
// Created by biromed on 2/18/26.
//
#include "el6695_ethercat_bridge/bridge_node.h"

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<el6695_bridge::BridgeNode>());
    rclcpp::shutdown();
    return 0;
}