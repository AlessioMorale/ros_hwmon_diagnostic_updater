// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Alessio Morale

#ifndef HWMON_DIAGNOSTIC_UPDATER_NODE__HWMON_CONFIG_HPP_
#define HWMON_DIAGNOSTIC_UPDATER_NODE__HWMON_CONFIG_HPP_

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <filesystem>
#include <limits>

namespace hwmon_diagnostic_updater
{

/// @brief Threshold configuration for a single metric
/// @details Defines OK/WARN/ERROR boundaries for metric values.
///          Thresholds must satisfy: error_min < warn_min <= warn_max < error_max
struct ThresholdConfig
{
  // Unset thresholds stay at +/-infinity and never trigger
  double warn_min = -std::numeric_limits<double>::infinity();   ///< Lower WARN threshold
  double warn_max = std::numeric_limits<double>::infinity();    ///< Upper WARN threshold
  double error_min = -std::numeric_limits<double>::infinity();  ///< Lower ERROR threshold
  double error_max = std::numeric_limits<double>::infinity();   ///< Upper ERROR threshold

  /// @brief Validate threshold ranges are logical
  /// @return Error message if invalid, std::nullopt if valid
  [[nodiscard]] std::optional<std::string> validate() const noexcept;
};

/// @brief Configuration for a single hwmon metric
struct MetricConfig
{
  std::string name;                              ///< Metric identifier
  std::string file;                              ///< sysfs filename (relative to sensor base_path)
  double scale = 1.0;                            ///< Multiplier for raw value
  double offset = 0.0;                           ///< Additive offset after scaling
  ThresholdConfig thresholds;                    ///< Threshold configuration
  std::string battery_field;                     ///< BatteryState field: voltage|current|temperature, empty = none
  std::chrono::milliseconds data_timeout_ms{1000};  ///< Max age before marking stale

  /// @brief Validate metric configuration
  /// @return Error message if invalid, std::nullopt if valid
  [[nodiscard]] std::optional<std::string> validate() const noexcept;
};

/// @brief Configuration for a single hwmon sensor/device
struct SensorConfig
{
  std::string name;                                   ///< Sensor identifier
  std::filesystem::path base_path;                    ///< Absolute path to hwmon device directory
  std::filesystem::path configured_path;              ///< base_path as configured (before hwmonN lookup)
  bool enabled = true;                                ///< Whether to monitor this sensor
  std::map<std::string, std::string> init_writes;     ///< Values to write at startup
  std::vector<MetricConfig> metrics;                  ///< Metrics to read from this device

  /// @brief Validate sensor configuration
  /// @return Error message if invalid, std::nullopt if valid
  [[nodiscard]] std::optional<std::string> validate() const noexcept;
};

/// @brief Overall node configuration
struct HwmonNodeConfig
{
  std::vector<SensorConfig> sensors;                  ///< Sensor configurations
  double update_rate_hz = 10.0;                       ///< Update frequency
  std::string battery_topic = "/battery_state";       ///< BatteryState publish topic
  std::string qos_reliability = "reliable";           ///< QoS reliability (reliable|best_effort)
  std::string qos_durability = "volatile";            ///< QoS durability (volatile|transient_local)
  int qos_depth = 10;                                 ///< QoS queue depth

  /// @brief Validate entire configuration
  /// @return Error message if invalid, std::nullopt if valid
  [[nodiscard]] std::optional<std::string> validate() const noexcept;
};

}  // namespace hwmon_diagnostic_updater

#endif  // HWMON_DIAGNOSTIC_UPDATER_NODE__HWMON_CONFIG_HPP_
