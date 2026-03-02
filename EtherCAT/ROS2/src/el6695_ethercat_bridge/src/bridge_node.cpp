#include "el6695_ethercat_bridge/bridge_node.h"
#include <chrono>



namespace el6695_bridge
{

BridgeNode::BridgeNode()
: Node("el6695_bridge"), running_(true)
{
    RCLCPP_INFO(this->get_logger(), "Initializing EL6695 bridge node...");

    master_ = ecrt_request_master(0);
    if (!master_) {
        RCLCPP_FATAL(this->get_logger(), "Failed to get EtherCAT master");
        throw std::runtime_error("Failed to get EtherCAT master");
    }
    RCLCPP_INFO(this->get_logger(), "EtherCAT master acquired");

    slave_ = std::make_shared<EL6695Slave>(master_, this->get_logger());

    if (!slave_->configure()) {
        RCLCPP_FATAL(this->get_logger(), "Slave configuration failed — shutting down");
        throw std::runtime_error("Failed to configure slave");
    }

    pub_ = create_publisher<std_msgs::msg::Int32>("from_twincat_counter", 10);

    sub_ = create_subscription<std_msgs::msg::Int32>(
        "to_twincat_counter", 10,
        std::bind(&BridgeNode::toTwinCATCallback, this, std::placeholders::_1)
    );

    // Publisher for the bool coming from TwinCAT
    pub_bool_ = create_publisher<std_msgs::msg::Bool>("from_twincat_bool", 10);

    // Subscriber for the bool to send to TwinCAT
    sub_bool_ = create_subscription<std_msgs::msg::Bool>(
        "to_twincat_bool", 10,
        std::bind(&BridgeNode::toTwinCATBoolCallback, this, std::placeholders::_1)
    );


    timer_ = create_wall_timer(
        std::chrono::milliseconds(10),
        std::bind(&BridgeNode::publishState, this)
    );

    ec_thread_ = std::thread(&BridgeNode::ethercatLoop, this);
    RCLCPP_INFO(this->get_logger(), "EtherCAT loop thread started (1 ms cycle)");

    RCLCPP_INFO(this->get_logger(), "Bridge node ready");
    }

    BridgeNode::~BridgeNode()
    {
    RCLCPP_INFO(this->get_logger(), "Shutting down bridge node...");
    running_ = false;
    if (ec_thread_.joinable()) {
        ec_thread_.join();
        RCLCPP_INFO(this->get_logger(), "EtherCAT thread joined — shutdown complete");
        }
    }

void BridgeNode::ethercatLoop()
{
    RCLCPP_DEBUG(this->get_logger(), "EtherCAT loop running");
    auto next = std::chrono::steady_clock::now();

    while (running_) {
        next += std::chrono::microseconds(1000);

        {
        std::lock_guard<std::mutex> lock(mutex_);
        slave_->setCommand(tx_);
        }

        slave_->process();

        {
        std::lock_guard<std::mutex> lock(mutex_);
        rx_ = slave_->getState();
        }

        std::this_thread::sleep_until(next);
    }

        RCLCPP_DEBUG(this->get_logger(), "EtherCAT loop exited");
}

        void BridgeNode::toTwinCATCallback(const std_msgs::msg::Int32::SharedPtr msg)
        {
        RCLCPP_DEBUG(this->get_logger(), "Received command: counter=%d", msg->data);
        std::lock_guard<std::mutex> lock(mutex_);
        tx_.counter = msg->data;
        }

void BridgeNode::publishState()
{
        std_msgs::msg::Int32 out;
        {
        std::lock_guard<std::mutex> lock(mutex_);
        out.data = rx_.counter;
        }
        RCLCPP_DEBUG(this->get_logger(), "Publishing state: counter=%d", out.data);
        pub_->publish(out);

        // Publish bool
        std_msgs::msg::Bool out_bool;
        out_bool.data = rx_.flag;  // rx_ is FromTwinCAT struct
        pub_bool_->publish(out_bool);
}

void BridgeNode::toTwinCATBoolCallback(const std_msgs::msg::Bool::SharedPtr msg)
{
    RCLCPP_DEBUG(this->get_logger(), "Received command: bool=%s", msg->data ? "true" : "false");
    std::lock_guard<std::mutex> lock(mutex_);
    tx_.flag = msg->data;  // tx_ is your ToTwinCAT struct
}


} // namespace el6695_bridge