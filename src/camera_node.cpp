// src/argus_composed_main.cpp
#include <rclcpp/rclcpp.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp_components/node_factory.hpp>
#include <pluginlib/class_loader.hpp>

using rclcpp_components::NodeFactory;
using rclcpp_components::NodeInstanceWrapper;

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  auto exec = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();

  // Loader for composable nodes
  pluginlib::ClassLoader<NodeFactory> loader("rclcpp_components", "rclcpp_components::NodeFactory");

  try {
    // --- 1) Load RealSense camera component ---
    // Class name provided by the realsense2_camera package
    auto rs_factory = loader.createSharedInstance("realsense2_camera::RealSenseNodeFactory");

    // Set the parameters to mirror your Python launch
    rclcpp::NodeOptions rs_opts;
    rs_opts.use_intra_process_comms(true);

    std::vector<rclcpp::Parameter> rs_params = {
      {"enable_depth", true},
      {"enable_infra1", true},
      {"enable_infra2", true},
      {"enable_color", true},
      {"align_depth", true}, // top-level (legacy) switch; also keep filter-specific one
      {"align_depth.enable", true},
      {"pointcloud.enable", true},
      // If you really are using the custom "pointcloud__neon_" namespace, keep these as-is.
      // If not, typical upstream keys are "pointcloud.stream_filter", etc.
      {"pointcloud__neon_.enable", true},
      {"pointcloud__neon_.stream_filter", std::string("color")},
      {"pointcloud__neon_.stream_index_filter", 0},
      {"pointcloud__neon_.ordered_pc", true},
      {"pointcloud__neon_.allow_no_texture_points", true}
    };
    rs_opts.parameter_overrides(rs_params);

    rs_opts.arguments({"--ros-args", "-r", "__node:=realsense2_camera"}); // set node name

    NodeInstanceWrapper rs_node = rs_factory->create_node_instance(rs_opts);
    exec->add_node(rs_node.get_node_base_interface());

    // --- 2) Load your rs_monitor component (if available as a component) ---
    // Replace with your actual component class name if different:
    // e.g., "argus_perception::RsMonitor" or "argus_perception::RsMonitorNode"
    try {
      auto mon_factory = loader.createSharedInstance("argus_perception::RsMonitor");
      rclcpp::NodeOptions mon_opts;
      mon_opts.use_intra_process_comms(true);
      mon_opts.arguments({"--ros-args", "-r", "__node:=rs_monitor"});

      NodeInstanceWrapper mon_node = mon_factory->create_node_instance(mon_opts);
      exec->add_node(mon_node.get_node_base_interface());
      RCLCPP_INFO(rclcpp::get_logger("argus_composed"), "Loaded argus_perception::RsMonitor component.");
    } catch (const std::exception &e) {
      RCLCPP_WARN(rclcpp::get_logger("argus_composed"),
        "Could not load argus_perception::RsMonitor as a component (%s). "
        "Build rs_monitor as a composable node or run it as a separate process.",
        e.what());
    }

    RCLCPP_INFO(rclcpp::get_logger("argus_composed"), "Argus composed container running.");
    exec->spin();

  } catch (const std::exception &e) {
    RCLCPP_FATAL(rclcpp::get_logger("argus_composed"), "Failed to create components: %s", e.what());
  }

  rclcpp::shutdown();
  return 0;
}

