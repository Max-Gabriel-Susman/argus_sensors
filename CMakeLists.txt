cmake_minimum_required(VERSION 3.8)
project(argus_sensors)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(class_loader REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(argus_core REQUIRED)
find_package(ament_index_cpp REQUIRED)

add_executable(camera_node src/camera_node.cpp)
add_executable(neural_telemetry_receiver_node src/neural_telemetry_receiver.cpp)
add_executable(neural_telemetry_replay src/neural_telemetry_replay.cpp)
add_executable(neural_udp_receiver src/neural_udp_receiver.cpp)

add_library(argus_sensors_rs_monitor src/rs_monitor.cpp)

target_include_directories(argus_sensors_rs_monitor PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

ament_target_dependencies(argus_sensors_rs_monitor
  rclcpp
)

ament_target_dependencies(camera_node
  rclcpp
  rclcpp_components
  class_loader
  sensor_msgs
)

ament_target_dependencies(neural_telemetry_receiver_node
  rclcpp
  std_msgs
  argus_core
)

ament_target_dependencies(neural_telemetry_replay
  rclcpp
  argus_core
  ament_index_cpp
)

ament_target_dependencies(neural_udp_receiver
  rclcpp
  argus_core
)

# ament_target_dependencies does not propagate all exported include
# directories from argus_core; add them explicilitly so argus_wire.h 
# and the generated NeuralFrame headers resolve.
target_include_directories(neural_telemetry_receiver_node PRIVATE
  ${argus_core_INCLUDE_DIRS}
)
target_include_directories(neural_telemetry_receiver PRIVATE
  ${argus_core_INCLUDE_DIRS}
)
target_include_directories(neural_udp_receiver PRIVATE
  ${argus_core_INCLUDE_DIRS}
)

target_link_libraries(camera_node
  argus_sensors_rs_monitor
)

target_include_directories(camera_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(camera_node PUBLIC c_std_99 cxx_std_17)

install(DIRECTORY include/ DESTINATION include)
ament_export_include_directories(include)

install(TARGETS
  camera_node
  neural_telemetry_receiver_node
  neural_telemetry_replay
  neural_udp_receiver
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
