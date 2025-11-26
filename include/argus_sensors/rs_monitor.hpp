#pragma once

#include <rclcpp/rclcpp.hpp>

namespace argus_sensors
{
    class RsMonitor : public rclcpp::Node
    {
        public: 
            explicit RsMonitor(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
    }
}