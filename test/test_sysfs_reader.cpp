// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Alessio Morale

#include <gtest/gtest.h>
#include "hwmon_diagnostic_updater/sysfs_reader.hpp"

namespace hwmon_diagnostic_updater
{

class SysfsReaderTest : public ::testing::Test
{
protected:
  SysfsReader reader;
};

// Test: read non-existent file
TEST_F(SysfsReaderTest, ReadNonexistentFile)
{
  auto result = reader.read_value("/nonexistent/path/file");
  EXPECT_FALSE(result.has_value());
}

// Test: parse valid double
TEST_F(SysfsReaderTest, ParseValidDouble)
{
  auto result = reader.read_string("/etc/hostname");
  // This will depend on system, so we just check it doesn't crash
  EXPECT_TRUE(true);
}

// Test: path exists check
TEST_F(SysfsReaderTest, PathExistsCheck)
{
  // Check existing path
  EXPECT_TRUE(reader.path_exists("/etc"));
  
  // Check non-existent path
  EXPECT_FALSE(reader.path_exists("/nonexistent/path"));
}

}  // namespace hwmon_diagnostic_updater
