#ifndef EL6695_ETHERCAT_BRIDGE_BRIDGE_NODE_H
#define EL6695_ETHERCAT_BRIDGE_BRIDGE_NODE_H

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <std_msgs/msg/bool.hpp>

#include "el6695_ethercat_bridge/el6695_slave.h"

namespace el6695_bridge
{

    class BridgeNode : public rclcpp::Node
    {
    public:
        BridgeNode();
        ~BridgeNode();

    private:
        void ethercatLoop();
        void toTwinCATCallback(const std_msgs::msg::Int32::SharedPtr msg);
        void publishState();
        void toTwinCATBoolCallback(const std_msgs::msg::Bool::SharedPtr msg);

        std::thread      ec_thread_;
        std::atomic<bool> running_;
        std::mutex       mutex_;

        ToTwinCAT   tx_;
        FromTwinCAT rx_;

        ec_master_t*                 master_;
        std::shared_ptr<EL6695Slave> slave_;

        rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr   pub_;
        rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sub_;
        rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr pub_bool_;
        rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr sub_bool_;
        rclcpp::TimerBase::SharedPtr                         timer_;
    };

} // namespace el6695_bridge
#endif