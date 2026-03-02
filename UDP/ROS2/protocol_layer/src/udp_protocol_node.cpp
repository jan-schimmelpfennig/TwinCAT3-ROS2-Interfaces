#include <rclcpp/rclcpp.hpp>
#include <udp_msgs/msg/udp_packet.hpp>
#include <vector>

#include "protocol_layer/crc_calculator.h"
#include "protocol_layer/message_definitions.h"
#include "protocol_layer/message_serializer.h"

class UdpProtocolNode : public rclcpp::Node {
public:
    UdpProtocolNode() : Node("udp_protocol_node"), counter_(0) {
        // Publisher to UDP write topic
        udp_write_pub_ = this->create_publisher<udp_msgs::msg::UdpPacket>(
        "/udp_write", 10);

        // Subscriber to UDP read topic
        udp_read_sub_ = this->create_subscription<udp_msgs::msg::UdpPacket>(
        "/udp_read", 10,
        std::bind(&UdpProtocolNode::udpReadCallback, this, std::placeholders::_1));

        // Timer to send periodic messages (10 Hz)
        timer_ = this->create_wall_timer(
        std::chrono::milliseconds(100),
        std::bind(&UdpProtocolNode::timerCallback, this));

        RCLCPP_INFO(this->get_logger(), "UDP Protocol Node initialized");
    }

private:
    uint64_t last_received_timestamp_ = 0; // Store the last received timestamp

    void timerCallback() {
        // Create a sample message
        protocol_layer::toTwinCAT msg_toTwinCAT;
        msg_toTwinCAT.a_byte = 0x12;
        msg_toTwinCAT.an_integer = counter_++;
        msg_toTwinCAT.timestamp = last_received_timestamp_;

        // Prepare buffer for serialization
        std::vector<uint8_t> buffer(sizeof(protocol_layer::toTwinCAT));

        // Serialize the message
        std::size_t bytes_written = protocol_layer::serialize(
        msg_toTwinCAT,
        buffer.data(),
        buffer.size()
        );

        if (bytes_written == 0) {
        RCLCPP_ERROR(this->get_logger(), "Failed to serialize message");
        return;
        }

        // Resize the buffer to actual bytes written
        buffer.resize(bytes_written);

        // Prepare ROS UDP message
        udp_msgs::msg::UdpPacket udp_msg;
        udp_msg.address = "50.50.50.50"; // TwinCAT CX IP
        udp_msg.src_port = 10000; // TwinCAT UDP port
        udp_msg.data = buffer;

        // Publish to UDP write topic
        udp_write_pub_->publish(udp_msg);

        RCLCPP_DEBUG(this->get_logger(),
        "Sent message: a_byte=0x%02X counter=%d size=%zu",
        msg_toTwinCAT.a_byte, msg_toTwinCAT.an_integer,  buffer.size());
    }

    void udpReadCallback(const udp_msgs::msg::UdpPacket::SharedPtr msg) {
        // Deserialize received data
        protocol_layer::fromTwinCAT received_msg;

        if (protocol_layer::deserialize(msg->data.data(), msg->data.size(), received_msg)) {
            RCLCPP_INFO(
                this->get_logger(),
                "Received UDP packet from %s:%u keyword=%u integer=%d | timestamp:%ld",
                msg->address.c_str(),
                msg->src_port,
                received_msg.keyword,
                received_msg.an_integer,
                received_msg.timestamp
            );
            // Use the last received timestamp
            last_received_timestamp_=received_msg.timestamp;
        } else {
        RCLCPP_WARN(this->get_logger(),
        "Failed to deserialize message (invalid format or checksum mismatch)");
        }
    }

    rclcpp::Publisher<udp_msgs::msg::UdpPacket>::SharedPtr udp_write_pub_;
    rclcpp::Subscription<udp_msgs::msg::UdpPacket>::SharedPtr udp_read_sub_;
    rclcpp::TimerBase::SharedPtr timer_;
    int32_t counter_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<UdpProtocolNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}