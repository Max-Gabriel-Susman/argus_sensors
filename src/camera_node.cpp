#include <rclcpp/rclcpp.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp_components/node_factory.hpp>
#include <class_loader/class_loader.hpp>
#include "argus_sensors/rs_monitor.hpp"

using rclcpp_components::NodeFactory;
using rclcpp_components::NodeInstanceWrapper;

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto logger = rclcpp::get_logger("argus_composed");

  rclcpp::executors::MultiThreadedExecutor exec;

  try {
    auto loader = std::make_shared<class_loader::ClassLoader>("librealsense2_camera.so");

    auto classes = loader->getAvailableClasses<NodeFactory>();

    if (classes.empty()) {
      RCLCPP_FATAL(
        logger,
        "No rclcpp_components::NodeFactory classes found in librealsense2_camera.so");
      rclcpp::shutdown();
      return 1;
    }

    std::string target_class = "realsense2_camera::RealSenseNodeFactory";
    if (std::find(classes.begin(), classes.end(), target_class) == classes.end()) {
      RCLCPP_WARN(
        logger,
        "Class '%s' not found in librealsense2_camera.so. Available classes are:",
        target_class.c_str());
      for (const auto & c : classes) {
        RCLCPP_WARN(logger, " %s", c.c_str());
      }

      target_class = classes.front();
      RCLCPP_WARN(
        logger, "Falling back to first available class: %s",
        target_class.c_str());
    }

    auto rs_factory = loader->createInstance<NodeFactory>(target_class);

    rclcpp::NodeOptions rs_opts;
    rs_opts.use_intra_process_comms(true);

    std::vector<rclcpp::Parameter> rs_params = {
      {"enable_depth", true},
      {"enable_infra1", true},
      {"enable_infra2", true},
      {"enable_color", true},
      {"align_depth", true},
      {"align_depth.enable", true},
      {"pointcloud.enable", true},
      {"pointcloud__neon_.enable", true},
      {"pointcloud__neon_.stream_filter", std::string("color")},
      {"pointcloud__neon_.stream_index_filter", 0},
      {"pointcloud__neon_.ordered_pc", true},
      {"pointcloud__neon_.allow_no_texture_points", true}
    };

    rs_opts.parameter_overrides(rs_params);

    rs_opts.arguments({"--ros-args", "-r", "__node:=realsense2_camera"});
    NodeInstanceWrapper rs_node = rs_factory->create_node_instance(rs_opts);
    exec.add_node(rs_node.get_node_base_interface());

    // rs_monitor
    rclcpp::NodeOptions mon_opts;
    mon_opts.use_intra_process_comms(true);
    auto mon_node = std::make_shared<argus_sensors::RsMonitor>(mon_opts);
    exec.add_node(mon_node);

    RCLCPP_INFO(logger, "Loaded RealSense component and RsMonitor C++ node.");
    exec.spin();

  } catch (const std::exception & e) {
    RCLCPP_FATAL(
      logger,
      "Failed to create components: %s", e.what());
  }

  rclcpp::shutdown();
  return 0;
}
