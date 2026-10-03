// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Alessio Morale

#ifndef HWMON_DIAGNOSTIC_UPDATER_NODE__HWMON_NODE_HPP_
#define HWMON_DIAGNOSTIC_UPDATER_NODE__HWMON_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/battery_state.hpp>
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <diagnostic_updater/diagnostic_updater.hpp>

#include <memory>
#include <string>
#include <map>
#include <chrono>
#include <vector>

#include "hwmon_diagnostic_updater/hwmon_config.hpp"
#include "hwmon_diagnostic_updater/sysfs_reader.hpp"

namespace hwmon_diagnostic_updater
{

/// @brief ROS 2 node for reading hwmon metrics and publishing diagnostics
///
/// This composable node reads Linux hwmon interface files via sysfs and publishes:
/// - Diagnostics to /diagnostics topic with status aggregation
/// - BatteryState messages to configurable topic
///
/// Key features:
/// - Multiple sensors per node with independent thresholds
/// - Dynamic parameter reconfiguration with validation
/// - Stale data detection with configurable timeouts
/// - Watchdog pattern for consecutive failure tracking
/// - Graceful degradation: continues with available sensors
/// - Thread-safe parameter updates
class HwmonNode : public rclcpp::Node
{
public:
  /// @brief Constructor for composable node or standalone
  /// @param options ROS 2 node options
  explicit HwmonNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  // Initialization
  /// @brief Load configuration from ROS parameters
  /// @throws rclcpp exceptions or std::runtime_error on malformed parameters
  void load_configuration();

  /// @brief Initialize diagnostic updater with hardware ID
  void initialize_diagnostics() noexcept;

  /// @brief Initialize sysfs write operations (e.g., shunt resistor config)
  /// @return true if all writes succeeded, false if any failed
  [[nodiscard]] bool initialize_sysfs_writes() noexcept;

  /// @brief Declare all ROS parameters with descriptors and defaults
  void declare_parameters();

  // Parameter management
  /// @brief Handle parameter changes with validation
  /// @param parameters Vector of parameters that changed
  /// @return SetParametersResult with success status and error message
  rcl_interfaces::msg::SetParametersResult on_parameter_change(
    const std::vector<rclcpp::Parameter> & parameters) noexcept;

  /// @brief Apply parameter changes to configuration
  /// @param param Parameter to apply
  /// @param config Configuration to update
  /// @return Error message if invalid, std::nullopt if valid and applied
  [[nodiscard]] std::optional<std::string> apply_parameter_to_config(
    const rclcpp::Parameter & param, HwmonNodeConfig & config) noexcept;

  // Diagnostics & publishing
  /// @brief Callback for periodic diagnostic update (called by diagnostic_updater)
  /// @param stat Diagnostic status wrapper to populate
  void update_diagnostics(diagnostic_updater::DiagnosticStatusWrapper & stat) noexcept;

  /// @brief Read all metrics and publish BatteryState message
  void publish_battery_state() noexcept;

  /// @brief Read a single metric value with safety checks
  /// @param sensor_name Name of sensor (for error reporting and watchdog)
  /// @param metric Metric configuration
  /// @return Value if read succeeded, std::nullopt otherwise
  /// @details Updates watchdog counters and last_successful_read timestamps
  [[nodiscard]] std::optional<double> read_metric(const std::string & sensor_name,
                                                    const MetricConfig & metric) noexcept;

  /// @brief Check if sensor data is fresh (not older than timeout)
  /// @param sensor_name Sensor identifier
  /// @param metric Metric configuration
  /// @return true if data is within timeout, false if stale
  [[nodiscard]] bool is_data_fresh(const std::string & sensor_name,
                                     const MetricConfig & metric) const noexcept;

  /// @brief Determine metric status based on value and thresholds
  /// @param value Metric value
  /// @param thresholds Threshold configuration
  /// @return 0=OK, 1=WARN, 2=ERROR
  [[nodiscard]] int determine_metric_status(double value,
                                             const ThresholdConfig & thresholds) const
    noexcept;

  // Members
  std::unique_ptr<SysfsReader> sysfs_reader_;                       // File I/O abstraction
  HwmonNodeConfig config_;                                          // Current configuration
  rclcpp::Publisher<sensor_msgs::msg::BatteryState>::SharedPtr
    battery_publisher_;                                             // BatteryState publisher
  std::unique_ptr<diagnostic_updater::Updater> diagnostic_updater_; // Diagnostic updater
  rclcpp::TimerBase::SharedPtr timer_;                              // Periodic update timer
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr
    parameter_callback_handle_;                                      // Parameter callback handle

  // Watchdog tracking: timestamp of last successful read per "sensor/metric"
  std::map<std::string, std::chrono::steady_clock::time_point> last_successful_read_;

  // Watchdog tracking: consecutive read failures per sensor
  std::map<std::string, size_t> consecutive_read_failures_;

  // Configuration protection
  mutable std::mutex config_mutex_;

  static constexpr size_t WARN_FAILURE_THRESHOLD = 3;     // Failures before WARN
  static constexpr size_t ERROR_FAILURE_THRESHOLD = 10;   // Failures before ERROR
};

}  // namespace hwmon_diagnostic_updater

#endif  // HWMON_DIAGNOSTIC_UPDATER_NODE__HWMON_NODE_HPP_
