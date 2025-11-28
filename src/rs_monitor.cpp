#include "argus_sensors/rs_monitor.hpp"

namespace argus_sensors
{
RsMonitor::RsMonitor(const rclcpp::NodeOptions & options)
: Node("rs_monitor", options)
{
  RCLCPP_INFO(this->get_logger(), "Initializing RsMonitor...\n");

  RCLCPP_INFO(this->get_logger(), "RsMonitor Online.\n");
}
}
