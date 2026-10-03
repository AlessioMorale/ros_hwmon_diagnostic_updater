#!/bin/bash
set -e

# Setup ROS 2 environment
source "/opt/ros/lyrical/setup.bash"

# Source workspace if built
if [ -f "/ros_ws/install/setup.bash" ]; then
    source "/ros_ws/install/setup.bash"
fi

# Execute command
exec "$@"
