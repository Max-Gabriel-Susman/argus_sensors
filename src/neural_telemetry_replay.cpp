#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "ament_index_cpp/get_package_share_directory.hpp"
#include "rclcpp/rclcpp.hpp"
#include "argus_core/msg/neural_frame.hpp"

using namespace std::chrono_literals;

class NeuralTelemetryReplay : public rclcpp::Node
{
public:
  NeuralTelemetryReplay()
  : Node("neural_telemetry_replay"),
    current_index_(0)
  {
    const auto default_csv =
      ament_index_cpp::get_package_share_directory("argus_sensors") +
      "/data/neural_96.csv";

    csv_path_ = this->declare_parameter<std::string>("csv_path", default_csv);
    output_topic_ = this->declare_parameter<std::string>(
      "output_topic", "/argus/neural_interface_bridge/neural_data");
    publish_period_ms_ = this->declare_parameter<int>("publish_period_ms", 500);
    loop_ = this->declare_parameter<bool>("loop", true);
    max_rows_ = this->declare_parameter<int>("max_rows", -1);

    auto qos = rclcpp::SensorDataQoS();

    publisher_ = this->create_publisher<argus_core::msg::NeuralFrame>(
      output_topic_, qos);

    if (!load_csv(csv_path_)) {
      RCLCPP_FATAL(
        this->get_logger(),
        "Failed to load CSV dataset from '%s'",
        csv_path_.c_str());
      throw std::runtime_error("failed to load neural replay CSV");
    }

    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(publish_period_ms_),
      std::bind(&NeuralTelemetryReplay::publish_next_frame, this));

    RCLCPP_INFO(
      this->get_logger(),
      "neural_telemetry_replay publishing %zu frames from '%s' to '%s' every %d ms",
      frames_.size(),
      csv_path_.c_str(),
      output_topic_.c_str(),
      publish_period_ms_);
  }

private:
  bool load_csv(const std::string & path)
  {
    std::ifstream file(path);
    if (!file.is_open()) {
      RCLCPP_ERROR(this->get_logger(), "Could not open CSV file: %s", path.c_str());
      return false;
    }

    std::string line;

    // Skip header
    if (!std::getline(file, line)) {
      RCLCPP_ERROR(this->get_logger(), "CSV file is empty: %s", path.c_str());
      return false;
    }

    int loaded_rows = 0;

    while (std::getline(file, line)) {
      if (line.empty()) {
        continue;
      }

      argus_core::msg::NeuralFrame msg;
      if (!parse_line(line, msg)) {
        RCLCPP_WARN(this->get_logger(), "Skipping malformed CSV row: %s", line.c_str());
        continue;
      }

      frames_.push_back(msg);
      loaded_rows++;

      if (max_rows_ > 0 && loaded_rows >= max_rows_) {
        break;
      }
    }

    return !frames_.empty();
  }

  bool parse_line(const std::string & line, argus_core::msg::NeuralFrame & msg)
  {
    std::stringstream ss(line);
    std::string cell;
    std::vector<std::string> fields;

    while (std::getline(ss, cell, ',')) {
      fields.push_back(cell);
    }

    if (fields.size() < 98) {
      return false;
    }

    msg.sample = static_cast<uint32_t>(std::stoul(fields[0]));
    msg.t = std::stof(fields[1]);
    msg.channel_count = 96;

    for (size_t i = 0; i < 96; ++i) {
      msg.channels[i] = static_cast<uint16_t>(std::stoul(fields[i + 2]));
    }

    return true;
  }

  void publish_next_frame()
  {
    if (frames_.empty()) {
      return;
    }

    if (current_index_ >= frames_.size()) {
      if (loop_) {
        current_index_ = 0;
      } else {
        RCLCPP_INFO_THROTTLE(
          this->get_logger(),
          *this->get_clock(),
          5000,
          "Replay reached end of dataset");
        return;
      }
    }

    publisher_->publish(frames_[current_index_]);

    RCLCPP_DEBUG(
      this->get_logger(),
      "Published replay frame sample=%u index=%zu",
      frames_[current_index_].sample,
      current_index_);

    current_index_++;
  }

  std::string csv_path_;
  std::string output_topic_;
  int publish_period_ms_;
  bool loop_;
  int max_rows_;

  size_t current_index_;
  std::vector<argus_core::msg::NeuralFrame> frames_;

  rclcpp::Publisher<argus_core::msg::NeuralFrame>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<NeuralTelemetryReplay>());
  rclcpp::shutdown();
  return 0;
}