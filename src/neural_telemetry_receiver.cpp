// neural_telemetry_receiver.cpp
#include <memory>
#include <string>
#include <utility>

#include "rclcpp/rclcpp.hpp"
#include "argus_core/msg/neural_frame.hpp"

class NeuralTelemetryReceiver : public rclcpp::Node
{
public:
  NeuralTelemetryReceiver()
  : Node("neural_telemetry_receiver")
  {
    input_topic_ = this->declare_parameter<std::string>(
      "input_topic", "/argus/neural_interface_bridge/neural_data");

    output_topic_ = this->declare_parameter<std::string>(
      "output_topic", "/argus/sensors/neural_telemetry");

    auto qos = rclcpp::SensorDataQoS();

    publisher_ = this->create_publisher<argus_core::msg::NeuralFrame>(
      output_topic_, qos);

    subscription_ = this->create_subscription<argus_core::msg::NeuralFrame>(
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
  void telemetry_callback(const argus_core::msg::NeuralFrame::SharedPtr msg)
  {
    if (msg->channel_count == 0) {
      RCLCPP_WARN_THROTTLE(
        this->get_logger(),
        *this->get_clock(),
        5000,
        "Received empty neural telemetry frame");
      return;
    }

    if (msg->channel_count > 96) {
      RCLCPP_WARN_THROTTLE(
        this->get_logger(),
        *this->get_clock(),
        5000,
        "Received invalid neural telemetry frame: channel_count=%u",
        msg->channel_count);
      return;
    }

    publisher_->publish(*msg);

    RCLCPP_DEBUG(
      this->get_logger(),
      "Republished neural telemetry frame sample=%u with %u channels",
      msg->sample,
      msg->channel_count);
  }

  std::string input_topic_;
  std::string output_topic_;

  rclcpp::Publisher<argus_core::msg::NeuralFrame>::SharedPtr publisher_;
  rclcpp::Subscription<argus_core::msg::NeuralFrame>::SharedPtr subscription_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<NeuralTelemetryReceiver>());
  rclcpp::shutdown();
  return 0;
}