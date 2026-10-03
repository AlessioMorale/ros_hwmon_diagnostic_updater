// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Alessio Morale

#include <rclcpp/rclcpp.hpp>
#include "hwmon_diagnostic_updater/hwmon_node.hpp"

int main(int argc, char * argv[])
{
  // Initialize ROS 2
  rclcpp::init(argc, argv);

  try {
    // Create node with default options
    auto node = std::make_shared<hwmon_diagnostic_updater::HwmonNode>();

    // Spin node in single-threaded executor
    rclcpp::spin(node);
  } catch (const std::exception & e) {
    RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Fatal error: %s", e.what());
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}
