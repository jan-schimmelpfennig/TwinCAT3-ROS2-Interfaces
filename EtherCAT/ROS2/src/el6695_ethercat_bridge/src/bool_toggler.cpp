//
// Created by biromed on 2/18/26.
//

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/bool.hpp>
#include <chrono>

namespace el6695_bridge
{

    class BoolToggler : public rclcpp::Node
    {
    public:
        BoolToggler()
        : Node("bool_toggler"), flag_(false)
        {
            pub_ = create_publisher<std_msgs::msg::Bool>("to_twincat_bool", 10);

            timer_ = create_wall_timer(
                std::chrono::milliseconds(100),  // 10 Hz
                std::bind(&BoolToggler::publishBool, this)
            );

            RCLCPP_INFO(this->get_logger(), "Bool toggler started, publishing to 'to_twincat_bool' at 10 Hz");
        }

    private:
        void publishBool()
        {
            std_msgs::msg::Bool msg;
            flag_ = !flag_;       // toggle the bool
            msg.data = flag_;
            pub_->publish(msg);

            RCLCPP_INFO(this->get_logger(), "Publishing bool: %s", flag_ ? "true" : "false");
        }

        bool flag_;
        rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr pub_;
        rclcpp::TimerBase::SharedPtr timer_;
    };

} // namespace el6695_bridge

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<el6695_bridge::BoolToggler>());
    rclcpp::shutdown();
    return 0;
}