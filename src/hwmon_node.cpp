// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Alessio Morale

#include "hwmon_diagnostic_updater/hwmon_node.hpp"

#include <rclcpp_components/register_node_macro.hpp>
#include <memory>
#include <chrono>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace hwmon_diagnostic_updater
{

namespace
{
// hwmonN numbering is not stable across boots. Accept a device path such as
// /sys/bus/i2c/devices/1-0040 and descend into its single hwmon/hwmonN dir.
std::filesystem::path resolve_hwmon_path(const std::filesystem::path & path)
{
  std::error_code ec;
  if (path.empty()) {
    return path;
  }
  std::filesystem::path found;
  for (const auto & entry : std::filesystem::directory_iterator(path, ec)) {
    if (entry.path().filename().string().rfind("hwmon", 0) == 0) {
      if (!found.empty()) {
        return path;  // ambiguous, let validation/diagnostics report it
      }
      found = entry.path();
    }
  }
  return found.empty() ? path : resolve_hwmon_path(found);
}

std::string metric_key(const std::string & sensor_name, const MetricConfig & metric)
{
  return sensor_name + "/" + metric.name;
}
}  // namespace

HwmonNode::HwmonNode(const rclcpp::NodeOptions & options)
: rclcpp::Node("hwmon_diagnostic_updater_node", options),
  sysfs_reader_(std::make_unique<SysfsReader>())
{
  RCLCPP_INFO(this->get_logger(), "Initializing hwmon diagnostic updater node");

  // Declare parameters
  declare_parameters();

  // Load configuration from parameters
  load_configuration();

  // Validate configuration
  auto config_error = config_.validate();
  if (config_error) {
    RCLCPP_ERROR(this->get_logger(), "Invalid configuration: %s", config_error.value().c_str());
    throw std::runtime_error("Invalid hwmon configuration: " + config_error.value());
  }

  // Initialize diagnostics
  initialize_diagnostics();

  // Initialize sysfs writes
  if (!initialize_sysfs_writes()) {
    RCLCPP_WARN(this->get_logger(), "Some sysfs initialization writes failed");
  }

  // Create publishers
  battery_publisher_ =
    this->create_publisher<sensor_msgs::msg::BatteryState>(config_.battery_topic, 10);

  // Register parameter change callback
  auto param_callback = [this](const std::vector<rclcpp::Parameter> & params) {
    return this->on_parameter_change(params);
  };
  parameter_callback_handle_ = this->add_on_set_parameters_callback(param_callback);

  // Start periodic update timer
  auto timer_callback = [this]() {
    try {
      diagnostic_updater_->force_update();
      publish_battery_state();
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "Error in update callback: %s", e.what());
    }
  };

  auto update_period = std::chrono::duration<double>(1.0 / config_.update_rate_hz);
  timer_ = this->create_wall_timer(
    std::chrono::duration_cast<std::chrono::milliseconds>(update_period), timer_callback);

  RCLCPP_INFO(this->get_logger(), "Hwmon diagnostic updater node initialized successfully");
}

void HwmonNode::declare_parameters()
{
  this->declare_parameter<double>("update_rate_hz", 10.0);
  this->declare_parameter<std::string>("battery_topic", "/battery_state");
  this->declare_parameter<std::string>("qos_reliability", "reliable");
  this->declare_parameter<std::string>("qos_durability", "volatile");
  this->declare_parameter<int64_t>("qos_depth", 10);
  this->declare_parameter<std::vector<std::string>>("sensors", std::vector<std::string>{});
}

void HwmonNode::load_configuration()
{
  config_.update_rate_hz = this->get_parameter("update_rate_hz").as_double();
  config_.battery_topic = this->get_parameter("battery_topic").as_string();
  config_.qos_reliability = this->get_parameter("qos_reliability").as_string();
  config_.qos_durability = this->get_parameter("qos_durability").as_string();
  config_.qos_depth = static_cast<int>(this->get_parameter("qos_depth").as_int());

  // Sensors: `sensors` lists names; each <sensor>.metrics lists metric names,
  // configured under <sensor>.<metric>.*
  config_.sensors.clear();
  // Copy: as_string_array() returns a reference into the temporary Parameter
  const std::vector<std::string> sensor_names = this->get_parameter("sensors").as_string_array();
  for (const auto & sensor_name : sensor_names) {
    const std::string sp = sensor_name + ".";
    SensorConfig sensor;
    sensor.name = sensor_name;
    sensor.configured_path = this->declare_parameter<std::string>(sp + "base_path", "");
    sensor.base_path = resolve_hwmon_path(sensor.configured_path);
    sensor.enabled = this->declare_parameter<bool>(sp + "enabled", true);

    for (const auto & entry :
      this->declare_parameter<std::vector<std::string>>(sp + "init_writes", {}))
    {
      const auto eq = entry.find('=');
      if (eq == std::string::npos) {
        throw std::runtime_error("init_writes entry must be 'file=value': " + entry);
      }
      sensor.init_writes[entry.substr(0, eq)] = entry.substr(eq + 1);
    }

    for (const auto & metric_name :
      this->declare_parameter<std::vector<std::string>>(sp + "metrics", {}))
    {
      const std::string mp = sp + metric_name + ".";
      MetricConfig metric;
      metric.name = metric_name;
      metric.file = this->declare_parameter<std::string>(mp + "file", "");
      metric.scale = this->declare_parameter<double>(mp + "scale", 1.0);
      metric.offset = this->declare_parameter<double>(mp + "offset", 0.0);
      metric.battery_field = this->declare_parameter<std::string>(mp + "battery_field", "");
      metric.data_timeout_ms =
        std::chrono::milliseconds(this->declare_parameter<int64_t>(mp + "data_timeout_ms", 1000));
      auto & t = metric.thresholds;
      t.warn_min = this->declare_parameter<double>(mp + "warn_min", t.warn_min);
      t.warn_max = this->declare_parameter<double>(mp + "warn_max", t.warn_max);
      t.error_min = this->declare_parameter<double>(mp + "error_min", t.error_min);
      t.error_max = this->declare_parameter<double>(mp + "error_max", t.error_max);
      sensor.metrics.push_back(std::move(metric));
    }

    RCLCPP_INFO(
      this->get_logger(), "Sensor '%s' at %s with %zu metrics", sensor.name.c_str(),
      sensor.base_path.c_str(), sensor.metrics.size());
    config_.sensors.push_back(std::move(sensor));
  }
}

void HwmonNode::initialize_diagnostics() noexcept
{
  diagnostic_updater_ = std::make_unique<diagnostic_updater::Updater>(this);

  // Set hardware ID from first sensor
  if (!config_.sensors.empty()) {
    diagnostic_updater_->setHardwareID(config_.sensors[0].name);
  } else {
    diagnostic_updater_->setHardwareID("unknown");
  }

  // Add diagnostic task
  diagnostic_updater_->add(
    "hwmon_status", this, &HwmonNode::update_diagnostics);
}

bool HwmonNode::initialize_sysfs_writes() noexcept
{
  bool all_success = true;

  for (const auto & sensor : config_.sensors) {
    if (!sensor.enabled) {
      continue;
    }

    for (const auto & [key, value] : sensor.init_writes) {
      auto path = sensor.base_path / key;
      if (!sysfs_reader_->write_value(path, value)) {
        RCLCPP_WARN(
          this->get_logger(), "Failed to write %s = %s to %s", key.c_str(), value.c_str(),
          path.c_str());
        all_success = false;
      }
    }
  }

  return all_success;
}

rcl_interfaces::msg::SetParametersResult HwmonNode::on_parameter_change(
  const std::vector<rclcpp::Parameter> & parameters) noexcept
{
  auto result = rcl_interfaces::msg::SetParametersResult();
  result.successful = true;

  std::lock_guard<std::mutex> lock(config_mutex_);

  HwmonNodeConfig temp_config = config_;

  for (const auto & param : parameters) {
    auto error = apply_parameter_to_config(param, temp_config);
    if (error) {
      result.successful = false;
      result.reason = error.value();
      RCLCPP_ERROR(this->get_logger(), "Parameter validation failed: %s", result.reason.c_str());
      return result;
    }
  }

  // Validate entire config
  auto config_error = temp_config.validate();
  if (config_error) {
    result.successful = false;
    result.reason = config_error.value();
    RCLCPP_ERROR(this->get_logger(), "Configuration invalid: %s", result.reason.c_str());
    return result;
  }

  config_ = temp_config;
  RCLCPP_INFO(this->get_logger(), "Parameters updated successfully");

  return result;
}

std::optional<std::string> HwmonNode::apply_parameter_to_config(
  const rclcpp::Parameter & param, HwmonNodeConfig & config) noexcept
{
  const auto & name = param.get_name();

  if (name == "update_rate_hz") {
    config.update_rate_hz = param.as_double();
  } else if (name == "battery_topic") {
    config.battery_topic = param.as_string();
  } else if (name == "qos_reliability") {
    config.qos_reliability = param.as_string();
  } else if (name == "qos_durability") {
    config.qos_durability = param.as_string();
  } else if (name == "qos_depth") {
    config.qos_depth = static_cast<int>(param.as_int());
  }

  return std::nullopt;
}

void HwmonNode::update_diagnostics(diagnostic_updater::DiagnosticStatusWrapper & stat) noexcept
{
  std::lock_guard<std::mutex> lock(config_mutex_);

  int overall_status = diagnostic_msgs::msg::DiagnosticStatus::OK;
  int healthy_sensors = 0;
  int error_metrics = 0;
  int total_metrics = 0;

  for (auto & sensor : config_.sensors) {
    if (!sensor.enabled) {
      continue;
    }

    // The device may be bound after startup (e.g. by a boot service): retry the hwmonN lookup
    if (!sensor.metrics.empty() &&
      !sysfs_reader_->path_exists(sensor.base_path / sensor.metrics.front().file))
    {
      auto resolved = resolve_hwmon_path(sensor.configured_path);
      if (resolved != sensor.base_path) {
        RCLCPP_INFO(
          this->get_logger(), "Sensor '%s' now at %s", sensor.name.c_str(), resolved.c_str());
        sensor.base_path = resolved;
      }
    }

    for (const auto & metric : sensor.metrics) {
      total_metrics++;

      // Check if path exists
      auto metric_path = sensor.base_path / metric.file;
      if (!sysfs_reader_->path_exists(metric_path)) {
        stat.add("error_missing_path_" + sensor.name + "_" + metric.name, metric_path.string());
        overall_status = std::max(overall_status, (int)diagnostic_msgs::msg::DiagnosticStatus::ERROR);
        error_metrics++;
        continue;
      }

      // Read metric value
      auto value = read_metric(sensor.name, metric);
      if (!value) {
        // No successful read within the timeout
        if (!is_data_fresh(sensor.name, metric)) {
          stat.add("error_stale_data_" + sensor.name + "_" + metric.name,
                   std::to_string(metric.data_timeout_ms.count()) + "ms");
          overall_status = std::max(overall_status, (int)diagnostic_msgs::msg::DiagnosticStatus::ERROR);
        }

        consecutive_read_failures_[sensor.name]++;
        int status = diagnostic_msgs::msg::DiagnosticStatus::OK;
        if (consecutive_read_failures_[sensor.name] >= ERROR_FAILURE_THRESHOLD) {
          status = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
        } else if (consecutive_read_failures_[sensor.name] >= WARN_FAILURE_THRESHOLD) {
          status = diagnostic_msgs::msg::DiagnosticStatus::WARN;
        }
        overall_status = std::max(overall_status, status);
        error_metrics++;
        continue;
      }

      // Reset failure counter on success
      consecutive_read_failures_[sensor.name] = 0;

      // Check thresholds
      int metric_status = determine_metric_status(value.value(), metric.thresholds);
      overall_status = std::max(overall_status, metric_status);

      stat.add(sensor.name + "_" + metric.name, value.value());
      if (metric_status > diagnostic_msgs::msg::DiagnosticStatus::OK) {
        error_metrics++;
      }
    }

    if (consecutive_read_failures_[sensor.name] < WARN_FAILURE_THRESHOLD) {
      healthy_sensors++;
    }
  }

  // Set overall status
  if (overall_status == diagnostic_msgs::msg::DiagnosticStatus::ERROR) {
    stat.summary(diagnostic_msgs::msg::DiagnosticStatus::ERROR, "Hwmon sensor error");
  } else if (overall_status == diagnostic_msgs::msg::DiagnosticStatus::WARN) {
    stat.summary(diagnostic_msgs::msg::DiagnosticStatus::WARN, "Hwmon sensor warning");
  } else {
    stat.summary(diagnostic_msgs::msg::DiagnosticStatus::OK, "Hwmon sensors OK");
  }

  stat.add("healthy_sensors", healthy_sensors);
  stat.add("error_metrics", error_metrics);
  stat.add("total_metrics", total_metrics);
}

void HwmonNode::publish_battery_state() noexcept
{
  std::lock_guard<std::mutex> lock(config_mutex_);

  auto msg = std::make_unique<sensor_msgs::msg::BatteryState>();
  msg->header.stamp = this->now();
  msg->power_supply_technology = sensor_msgs::msg::BatteryState::POWER_SUPPLY_TECHNOLOGY_UNKNOWN;
  msg->power_supply_health = sensor_msgs::msg::BatteryState::POWER_SUPPLY_HEALTH_UNKNOWN;
  msg->power_supply_status = sensor_msgs::msg::BatteryState::POWER_SUPPLY_STATUS_UNKNOWN;

  // Initialize with NaN for unavailable fields
  msg->voltage = std::numeric_limits<float>::quiet_NaN();
  msg->temperature = std::numeric_limits<float>::quiet_NaN();
  msg->current = std::numeric_limits<float>::quiet_NaN();
  msg->charge = std::numeric_limits<float>::quiet_NaN();
  msg->capacity = std::numeric_limits<float>::quiet_NaN();
  msg->design_capacity = std::numeric_limits<float>::quiet_NaN();
  msg->percentage = std::numeric_limits<float>::quiet_NaN();
  msg->present = false;

  bool has_valid_reading = false;

  for (const auto & sensor : config_.sensors) {
    if (!sensor.enabled) {
      continue;
    }

    for (const auto & metric : sensor.metrics) {
      if (metric.battery_field.empty()) {
        continue;
      }

      auto value = read_metric(sensor.name, metric);
      if (!value) {
        continue;
      }

      has_valid_reading = true;
      msg->header.frame_id = sensor.name;

      if (metric.battery_field == "voltage") {
        msg->voltage = static_cast<float>(value.value());
        msg->present = true;
      } else if (metric.battery_field == "current") {
        msg->current = static_cast<float>(value.value());
      } else if (metric.battery_field == "temperature") {
        msg->temperature = static_cast<float>(value.value());
      }
    }
  }

  if (has_valid_reading) {
    battery_publisher_->publish(std::move(msg));
  }
}

std::optional<double> HwmonNode::read_metric(const std::string & sensor_name,
                                              const MetricConfig & metric) noexcept
{
  auto metric_path = config_.sensors[0].base_path / metric.file;

  // Find actual sensor in config
  for (const auto & sensor : config_.sensors) {
    if (sensor.name == sensor_name) {
      metric_path = sensor.base_path / metric.file;
      break;
    }
  }

  auto raw_value = sysfs_reader_->read_value(metric_path);
  if (!raw_value) {
    return std::nullopt;
  }

  double scaled = raw_value.value() * metric.scale + metric.offset;

  if (!std::isfinite(scaled)) {
    RCLCPP_WARN_THROTTLE(
      this->get_logger(), *this->get_clock(), 5000,
      "Sensor %s metric %s calculation resulted in non-finite value", sensor_name.c_str(),
      metric.name.c_str());
    return std::nullopt;
  }

  last_successful_read_[metric_key(sensor_name, metric)] = std::chrono::steady_clock::now();

  return scaled;
}

bool HwmonNode::is_data_fresh(const std::string & sensor_name,
                               const MetricConfig & metric) const noexcept
{
  auto it = last_successful_read_.find(metric_key(sensor_name, metric));
  if (it == last_successful_read_.end()) {
    return false;  // Never read before
  }

  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);

  return elapsed <= metric.data_timeout_ms;
}

int HwmonNode::determine_metric_status(double value,
                                        const ThresholdConfig & thresholds) const noexcept
{
  // Check error bounds first
  if (value < thresholds.error_min || value > thresholds.error_max) {
    return diagnostic_msgs::msg::DiagnosticStatus::ERROR;
  }

  // Check warning bounds
  if (value < thresholds.warn_min || value > thresholds.warn_max) {
    return diagnostic_msgs::msg::DiagnosticStatus::WARN;
  }

  return diagnostic_msgs::msg::DiagnosticStatus::OK;
}

}  // namespace hwmon_diagnostic_updater
