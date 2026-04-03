# Argus Sensors 

This repository contains the sensors package for the Argus perception pipeline

## System Requirements

* Nvidia Orin Nano Super Developer Kit

* A RealSense D455 Camera connected to your Orin Nano

* An installation of ROS 2 Humble 

* An installation of ros-humble-rclcpp-components with apt

* An installation of ros-humble-class-loader with apt

## Prerequisites

* Creation of a ROS 2 workspace named argus_ws in your home directory

* Cloning the ROS 2 package contained in this repository into that workspace

## Usage 

Source ROS 2:
```
source /opt/ros/humble/setup.bash
```

Change directory to your workspace: 
```
cd ~/argus_ws
```

Build the argus_sensors package: 
```
colcon build --packages-select argus_sensors
```

Source the workspace's setup file: 
```
source install/setup.bash
```

View what executables your package has, you should now see camera_node: 
```
ros2 pkg executables argus_sensors
```

Run the camera_node you just created: 
```
ros2 run argus_sensors camera_node
```

### D455 camera

Open a second terminal, ssh into the orin nano and run rqt:
```
rqt
```

Select Image View from Plugins > Visualization. 

Once there select /camera/realsense2_camera/depth/image_rect_raw and you should see camera data from the D455 appear in your rqt image view.

## Development

If you need to format your code for this repo's linter:
```
ament_uncrustify --reformat path/to/file
```

## Nodes

The `argus_sensors` package provides the nodes used to interact with the Argus sensor array; including the camera node and the neural telemetry receiver node.

### Neural Telemetry Receiver Node

The neural Telemetry Receiver Node subscribes to the topics published by the argus neural interface bridge micro ros node.

### Camera Node

The camera node interfaces with a RealSense D455 camera to provide depth, image, and color detection to Argus.
