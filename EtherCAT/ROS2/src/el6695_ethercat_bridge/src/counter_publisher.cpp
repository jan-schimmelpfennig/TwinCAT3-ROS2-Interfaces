//
// Created by biromed on 2/18/26.
//

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>
#include <chrono>

namespace el6695_bridge
{

    class CounterPublisher : public rclcpp::Node
    {
    public:
        CounterPublisher()
        : Node("counter_publisher"), counter_(0)
        {
            pub_ = create_publisher<std_msgs::msg::Int32>("to_twincat", 10);

            timer_ = create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&CounterPublisher::publishCounter, this)
            );

            RCLCPP_INFO(this->get_logger(), "Counter publisher started, publishing to 'to_twincat' at 10 Hz");
        }

    private:
        void publishCounter()
        {
            std_msgs::msg::Int32 msg;
            msg.data = counter_++;
            pub_->publish(msg);
            RCLCPP_INFO(this->get_logger(), "Publishing counter: %d", msg.data);
        }

        int32_t counter_;
        rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr pub_;
        rclcpp::TimerBase::SharedPtr timer_;
    };

} // namespace el6695_bridge

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<el6695_bridge::CounterPublisher>());
    rclcpp::shutdown();
    return 0;
}