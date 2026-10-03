// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Alessio Morale

#include "hwmon_diagnostic_updater/sysfs_reader.hpp"

#include <fstream>
#include <sstream>
#include <cmath>
#include <limits>

namespace hwmon_diagnostic_updater
{

std::optional<double> SysfsReader::read_value(const std::filesystem::path & path) const
  noexcept
{
  std::lock_guard<std::mutex> lock(mutex_);

  if (!std::filesystem::exists(path)) {
    return std::nullopt;
  }

  try {
    std::ifstream file(path);
    if (!file.is_open()) {
      return std::nullopt;
    }

    std::string line;
    if (!std::getline(file, line)) {
      return std::nullopt;
    }

    return parse_double(line);
  } catch (...) {
    return std::nullopt;
  }
}

std::optional<std::string> SysfsReader::read_string(const std::filesystem::path & path) const
  noexcept
{
  std::lock_guard<std::mutex> lock(mutex_);

  if (!std::filesystem::exists(path)) {
    return std::nullopt;
  }

  try {
    std::ifstream file(path);
    if (!file.is_open()) {
      return std::nullopt;
    }

    std::string line;
    if (!std::getline(file, line)) {
      return std::nullopt;
    }

    return trim(line);
  } catch (...) {
    return std::nullopt;
  }
}

bool SysfsReader::write_value(const std::filesystem::path & path,
                               std::string_view value) const noexcept
{
  std::lock_guard<std::mutex> lock(mutex_);

  if (!std::filesystem::exists(path)) {
    return false;
  }

  try {
    std::ofstream file(path);
    if (!file.is_open()) {
      return false;
    }

    file << value;
    return file.good();
  } catch (...) {
    return false;
  }
}

bool SysfsReader::path_exists(const std::filesystem::path & path) const noexcept
{
  std::lock_guard<std::mutex> lock(mutex_);
  return std::filesystem::exists(path);
}

std::optional<double> SysfsReader::parse_double(std::string_view str) noexcept
{
  try {
    std::string s(str);
    size_t idx = 0;
    double value = std::stod(s, &idx);

    if (!std::isfinite(value)) {
      return std::nullopt;
    }

    return value;
  } catch (...) {
    return std::nullopt;
  }
}

std::string SysfsReader::trim(std::string_view str) noexcept
{
  const auto start = str.find_first_not_of(" \t\n\r\f\v");
  if (start == std::string_view::npos) {
    return "";
  }

  const auto end = str.find_last_not_of(" \t\n\r\f\v");
  return std::string(str.substr(start, end - start + 1));
}

}  // namespace hwmon_diagnostic_updater
