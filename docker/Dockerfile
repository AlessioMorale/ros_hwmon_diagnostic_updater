FROM ros:lyrical

# Install dependencies
RUN apt-get update && apt-get install -y \
    ros-lyrical-diagnostic-updater \
    ros-lyrical-sensor-msgs \
    i2c-tools \
    && rm -rf /var/lib/apt/lists/*

# Create workspace
WORKDIR /ros_ws

# Copy package source
COPY . /ros_ws/src/hwmon_diagnostic_updater/

# Build package
RUN . /opt/ros/lyrical/setup.sh && \
    colcon build --packages-select hwmon_diagnostic_updater --symlink-install

# Setup entrypoint
COPY docker/ros_entrypoint.sh /
RUN chmod +x /ros_entrypoint.sh
ENTRYPOINT ["/ros_entrypoint.sh"]

# Default command
CMD ["ros2", "launch", "hwmon_diagnostic_updater", "hwmon_diagnostic_updater.launch.py"]

# Build instructions:
# docker build -t hwmon_diagnostic_updater:latest -f docker/Dockerfile .
#
# Run with hardware access (requires --privileged or --device):
# docker run --rm -it \
#   --privileged \
#   --network host \
#   -v /sys/class/hwmon:/sys/class/hwmon:ro \
#   hwmon_diagnostic_updater:latest
#
# Or with specific device:
# docker run --rm -it \
#   --device=/dev/i2c-1 \
#   --network host \
#   -v /sys/class/hwmon:/sys/class/hwmon:ro \
#   hwmon_diagnostic_updater:latest
