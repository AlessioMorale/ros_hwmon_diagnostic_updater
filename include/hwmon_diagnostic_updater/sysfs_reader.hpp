// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Alessio Morale

#ifndef HWMON_DIAGNOSTIC_UPDATER_NODE__SYSFS_READER_HPP_
#define HWMON_DIAGNOSTIC_UPDATER_NODE__SYSFS_READER_HPP_

#include <optional>
#include <string>
#include <string_view>
#include <filesystem>
#include <mutex>

namespace hwmon_diagnostic_updater
{

/// @brief Safe wrapper for reading/writing sysfs files
///
/// Provides thread-safe access to hwmon sysfs interface with comprehensive
/// error handling. All operations check for file existence, permission errors,
/// parse errors, and I/O issues. Operations that fail return std::nullopt or false.
class SysfsReader
{
public:
  SysfsReader() = default;
  ~SysfsReader() = default;

  // Non-copyable, non-movable (could be enhanced to movable later)
  SysfsReader(const SysfsReader &) = delete;
  SysfsReader & operator=(const SysfsReader &) = delete;
  SysfsReader(SysfsReader &&) = delete;
  SysfsReader & operator=(SysfsReader &&) = delete;

  /// @brief Read a numeric value from sysfs file
  /// @param path Absolute path to sysfs file
  /// @return Parsed double value, or std::nullopt on any error
  /// @details Handles file not found, permission errors, parse errors gracefully.
  ///          Validates that result is finite (not NaN or Inf).
  [[nodiscard]] std::optional<double> read_value(const std::filesystem::path & path) const
    noexcept;

  /// @brief Read raw string from sysfs file
  /// @param path Absolute path to sysfs file
  /// @return File contents trimmed of whitespace, or std::nullopt on error
  [[nodiscard]] std::optional<std::string> read_string(const std::filesystem::path & path) const
    noexcept;

  /// @brief Write a value to sysfs file
  /// @param path Absolute path to sysfs file
  /// @param value Value to write
  /// @return true if write succeeded, false otherwise
  /// @details Handles file not found, permission errors, I/O errors gracefully.
  [[nodiscard]] bool write_value(const std::filesystem::path & path,
                                  std::string_view value) const noexcept;

  /// @brief Check if path exists and is readable
  /// @param path Absolute path to check
  /// @return true if path exists, false otherwise
  [[nodiscard]] bool path_exists(const std::filesystem::path & path) const noexcept;

private:
  mutable std::mutex mutex_;  ///< Protect file operations

  /// @brief Helper to safely parse string to double
  /// @param str String to parse
  /// @return Parsed value or std::nullopt on parse error
  [[nodiscard]] static std::optional<double> parse_double(std::string_view str) noexcept;

  /// @brief Trim whitespace from both ends of string
  /// @param str String to trim
  /// @return Trimmed string
  [[nodiscard]] static std::string trim(std::string_view str) noexcept;
};

}  // namespace hwmon_diagnostic_updater

#endif  // HWMON_DIAGNOSTIC_UPDATER_NODE__SYSFS_READER_HPP_
