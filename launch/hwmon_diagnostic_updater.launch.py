#!/usr/bin/env python3

from launch import LaunchDescription
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import PathJoinSubstitution
import os


def generate_launch_description():
    """Generate launch description for hwmon_diagnostic_updater_node."""

    # Get package share directory
    package_share = FindPackageShare('hwmon_diagnostic_updater')
    
    # Path to config file
    config_file = PathJoinSubstitution([
        package_share,
        'config',
        'ina3221_example.yaml'
    ])

    hwmon_node = Node(
        package='hwmon_diagnostic_updater',
        executable='hwmon_diagnostic_updater_node',
        output='screen',
        parameters=[config_file],
    )

    ld = LaunchDescription([
        hwmon_node,
    ])

    return ld


if __name__ == '__main__':
    generate_launch_description()
