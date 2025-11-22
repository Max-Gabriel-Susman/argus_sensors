# Argus Sensors 

This repository contains the sensors package for the Argus perception pipeline

## System Requirements

* Nvidia Orin Nano Super Developer Kit

* A RealSense D455 Camera connected to your Orin Nano

* An installation of ROS 2 Humble 

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

View what executables you package has, you should now see camera_node: 
```
ros2 pkg executables argus_sensors
```

Run the camera_node you just created: 
```
ros2 run argus_sensors camera_node
```



