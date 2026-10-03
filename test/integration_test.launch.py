#!/usr/bin/env python3
"""Integration test launch file for hwmon_diagnostic_updater_node.

This launch file is designed for automated testing with real hardware.
It launches the node and verifies it publishes expected topics.
"""

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import TimerAction
from launch_testing.actions import ReadyToTest
import launch_testing


def generate_launch_description():
    """Generate launch description for integration testing."""

    # Create minimal test configuration
    test_params = {
        'update_rate_hz': 1.0,
        'battery_topic': '/battery_state',
        'qos_reliability': 'reliable',
        'qos_durability': 'volatile',
        'qos_depth': 10,
    }

    hwmon_node = Node(
        package='hwmon_diagnostic_updater',
        executable='hwmon_diagnostic_updater_node',
        name='hwmon_test_node',
        output='screen',
        parameters=[test_params],
    )

    return LaunchDescription([
        hwmon_node,
        # Wait 2 seconds before starting tests
        TimerAction(
            period=2.0,
            actions=[ReadyToTest()]
        ),
    ])


# Note: This is a skeleton integration test launch file.
# Full integration tests would require:
# 1. Mocking hwmon sysfs interface or running on actual hardware
# 2. launch_testing pytest fixtures to verify topics
# 3. Assertions to check diagnostic status and BatteryState messages
#
# Example test (would go in separate test file):
# @launch_testing.post_shutdown_test()
# class TestProcessOutput(unittest.TestCase):
#     def test_exit_code(self, proc_info):
#         launch_testing.asserts.assertExitCodes(proc_info)
