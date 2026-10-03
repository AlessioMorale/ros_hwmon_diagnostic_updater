cmake_minimum_required(VERSION 3.16)
project(hwmon_diagnostic_updater)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(
    -Wall -Wextra -Wpedantic
    -Werror=conversion
    -Werror=unused-but-set-variable
    -Werror=return-type
    -Werror=shadow
    -Wno-psabi
  )
endif()

set(NODE_NAME hwmon_diagnostic_updater_node)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(diagnostic_msgs REQUIRED)
find_package(diagnostic_updater REQUIRED)
find_package(rcl_interfaces REQUIRED)
find_package(ament_cmake_gtest QUIET)
find_package(ament_cmake_gmock QUIET)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
)

# Build library for testing
add_library(${PROJECT_NAME}_lib
  src/sysfs_reader.cpp
  src/hwmon_config.cpp
  src/hwmon_node.cpp
)

target_include_directories(${PROJECT_NAME}_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Enable position independent code for shared library linking
set_target_properties(${PROJECT_NAME}_lib PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(${PROJECT_NAME}_lib PUBLIC
  rclcpp::rclcpp
  rclcpp_components::component
  ${sensor_msgs_TARGETS}
  ${diagnostic_msgs_TARGETS}
  diagnostic_updater::diagnostic_updater
  ${rcl_interfaces_TARGETS}
)

# Build shared library component for composable nodes
add_library(${PROJECT_NAME}_component SHARED
  src/hwmon_component.cpp
)

target_link_libraries(${PROJECT_NAME}_component
  ${PROJECT_NAME}_lib
)

# Register the component
rclcpp_components_register_nodes(${PROJECT_NAME}_component
  "hwmon_diagnostic_updater::HwmonNode"
)

# Build executable
add_executable(${NODE_NAME}
  src/hwmon_main.cpp
)

target_link_libraries(${NODE_NAME}
  ${PROJECT_NAME}_lib
)

# Install executable
install(TARGETS ${NODE_NAME}
  DESTINATION lib/${PROJECT_NAME}
)

# Install component library
install(TARGETS ${PROJECT_NAME}_component
  LIBRARY DESTINATION lib
)

# Install headers
install(DIRECTORY include/
  DESTINATION include
)

# Install launch files
install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

# Install config files
install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

if(BUILD_TESTING)
  if(ament_cmake_gtest_FOUND AND ament_cmake_gmock_FOUND)
    ament_add_gtest(${PROJECT_NAME}_test
      test/test_hwmon_node.cpp
      test/test_sysfs_reader.cpp
    )
    if(TARGET ${PROJECT_NAME}_test)
      target_link_libraries(${PROJECT_NAME}_test
        ${PROJECT_NAME}_lib
      )
    endif()
  endif()
endif()

ament_package()
