// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Alessio Morale

#include "hwmon_diagnostic_updater/hwmon_config.hpp"

#include <sstream>

namespace hwmon_diagnostic_updater
{

std::optional<std::string> ThresholdConfig::validate() const noexcept
{
  // Check for logical ordering: error_min <= warn_min <= warn_max <= error_max
  if (warn_min > warn_max) {
    return "warn_min must be <= warn_max";
  }

  if (warn_min < error_min) {
    return "warn_min must be >= error_min";
  }

  if (warn_max > error_max) {
    return "warn_max must be <= error_max";
  }

  return std::nullopt;
}

std::optional<std::string> MetricConfig::validate() const noexcept
{
  if (name.empty()) {
    return "metric name cannot be empty";
  }

  if (file.empty()) {
    return "metric sysfs file cannot be empty";
  }

  if (scale == 0.0) {
    return "metric scale cannot be zero";
  }

  if (!battery_field.empty() && battery_field != "voltage" && battery_field != "current" &&
    battery_field != "temperature")
  {
    return "metric battery_field must be one of: voltage, current, temperature";
  }

  if (data_timeout_ms.count() <= 0) {
    return "data_timeout_ms must be positive";
  }

  auto thresh_error = thresholds.validate();
  if (thresh_error) {
    return thresh_error;
  }

  return std::nullopt;
}

std::optional<std::string> SensorConfig::validate() const noexcept
{
  if (name.empty()) {
    return "sensor name cannot be empty";
  }

  if (base_path.empty()) {
    return "sensor base_path cannot be empty";
  }

  if (!base_path.is_absolute()) {
    return "sensor base_path must be absolute";
  }

  if (metrics.empty()) {
    return "sensor must have at least one metric";
  }

  for (const auto & metric : metrics) {
    auto error = metric.validate();
    if (error) {
      return error;
    }
  }

  return std::nullopt;
}

std::optional<std::string> HwmonNodeConfig::validate() const noexcept
{
  if (sensors.empty()) {
    return "configuration must have at least one sensor";
  }

  if (update_rate_hz <= 0.0) {
    return "update_rate_hz must be positive";
  }

  if (battery_topic.empty()) {
    return "battery_topic cannot be empty";
  }

  if (qos_reliability != "reliable" && qos_reliability != "best_effort") {
    return "qos_reliability must be 'reliable' or 'best_effort'";
  }

  if (qos_durability != "volatile" && qos_durability != "transient_local") {
    return "qos_durability must be 'volatile' or 'transient_local'";
  }

  if (qos_depth <= 0) {
    return "qos_depth must be positive";
  }

  for (const auto & sensor : sensors) {
    auto error = sensor.validate();
    if (error) {
      return error;
    }
  }

  return std::nullopt;
}

}  // namespace hwmon_diagnostic_updater
