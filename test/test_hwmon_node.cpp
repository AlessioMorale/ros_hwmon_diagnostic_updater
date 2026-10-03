// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Alessio Morale

#include <gtest/gtest.h>
#include "hwmon_diagnostic_updater/hwmon_config.hpp"

namespace hwmon_diagnostic_updater
{

class HwmonConfigTest : public ::testing::Test
{
protected:
};

// Test: valid threshold configuration
TEST_F(HwmonConfigTest, ValidThresholds)
{
  ThresholdConfig config;
  config.warn_min = 10.0;
  config.warn_max = 20.0;
  config.error_min = 5.0;
  config.error_max = 25.0;
  
  auto error = config.validate();
  EXPECT_FALSE(error.has_value());
}

// Test: invalid threshold ordering
TEST_F(HwmonConfigTest, InvalidThresholdOrdering)
{
  ThresholdConfig config;
  config.warn_min = 20.0;
  config.warn_max = 10.0;  // Invalid: min > max
  config.error_min = 5.0;
  config.error_max = 25.0;
  
  auto error = config.validate();
  EXPECT_TRUE(error.has_value());
}

// Test: metric validation with zero scale
TEST_F(HwmonConfigTest, MetricZeroScale)
{
  MetricConfig metric;
  metric.name = "test";
  metric.file = "test_file";
  metric.scale = 0.0;  // Invalid
  
  auto error = metric.validate();
  EXPECT_TRUE(error.has_value());
}

// Test: sensor validation requires metrics
TEST_F(HwmonConfigTest, SensorNoMetrics)
{
  SensorConfig sensor;
  sensor.name = "test_sensor";
  sensor.base_path = "/sys/class/hwmon/hwmon0";
  // No metrics
  
  auto error = sensor.validate();
  EXPECT_TRUE(error.has_value());
}

}  // namespace hwmon_diagnostic_updater
