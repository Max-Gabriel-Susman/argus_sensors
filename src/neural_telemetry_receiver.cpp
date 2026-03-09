#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float32_multi_array.hpp"

class NeuralTelemetryReceiver : public rclcpp::Node
{
public:
  NeuralTelemetryReceiver()
  : Node("neural_telemetry_receiver")
  {
    input_topic_ = this->declare_parameter<std::string>(
      "input_topic", "/argus/neural_interface/telemetry");

    output_topic_ = this->declare_parameter<std::string>(
      "output_topic", "/argus/sensors/neural_telemetry");

    auto qos = rclcpp::SensorDataQoS();

    publisher_ = this->create_publisher<std_msgs::msg::Float32MultiArray>(
      output_topic_, qos);

    subscription_ = this->create_subscription<std_msgs::msg::Float32MultiArray>(
      input_topic_,
      qos,
      std::bind(
	&NeuralTelemetryReceiver::telemetry_callback,
	this,
	std::placeholders::_1));

    RCLCPP_INFO(
      this->get_logger(),
      "neural_telemetry_receiver listening on '%s' and publishing to '%s'",
      input_topic_.c_str(),
      output_topic_.c_str());
 } 

private:
  void telemetry_callback(const std_msgs::msg::Float32MultiArray::SharedPtr msg)
  {
    if (msg->data.empty()) {
      RCLCPP_WARN_THROTTLE(
        this->get_logger(),
	*this->get_clock(),
	5000,
	"Received empty neural telemetry frame");
      return;
    }

    auto out_msg = std_msgs::msg::Float32MultiArray();
    out_msg.layout = msg->layout;
    out_msg.data = msg->data;

    publisher_->publish(out_msg);

    RCLCPP_DEBUG(
      this->get_logger(),
      "Republished neural telemetry fram with %zu values",
      out_msg.data.size());
  }

  std::string input_topic_;
  std::string output_topic_;

  rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr publisher_;
  rclcpp::Subscription<std_msgs::msg::Float32MultiArray>::SharedPtr subscription_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<NeuralTelemetryReceiver>());
  rclcpp::shutdown();
  return 0;
}
