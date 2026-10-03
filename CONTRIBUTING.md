# Contributing to hwmon_diagnostic_updater

Thank you for your interest in contributing! This document provides guidelines for contributing code, reporting bugs, and suggesting improvements.

## Code of Conduct

This project adheres to the Contributor Covenant Code of Conduct (see [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)). By participating, you are expected to uphold this code.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/YOUR_USERNAME/ros_elrs_joy.git`
3. Create a feature branch: `git checkout -b feature/your-feature-name`
4. Set up development environment: See README.md Installation section

## Development Workflow

### Code Style

- Follow Google C++ style guide with modifications in `.clang-format`
- Run `clang-format` before committing:
  ```bash
  clang-format -i src/**/*.cpp include/**/*.hpp test/**/*.cpp
  ```
- Run `clang-tidy` for static analysis:
  ```bash
  clang-tidy src/*.cpp -- -I include -std=c++20
  ```

### Testing

All code changes must include tests:
- Unit tests cover new functions/classes
- Safety tests cover error conditions
- Test coverage goal: >80% for core logic

Run tests before committing:
```bash
colcon test --packages-select hwmon_diagnostic_updater --event-handlers console_direct+
```

### Commit Messages

Follow conventional commit format:
- `feat:` A new feature
- `fix:` A bug fix
- `docs:` Documentation changes
- `test:` Test additions or modifications
- `refactor:` Code refactoring without behavior changes
- `perf:` Performance improvements
- `chore:` Build system, dependencies, etc.

Example:
```
feat: add stale data detection for sensor metrics

- Implement configurable per-metric timeout
- Track last successful read timestamp
- Escalate to ERROR diagnostic on timeout
- Add corresponding unit tests

Closes #123
```

### Pull Request Process

1. Create a feature branch from main
2. Make your changes with clear commit messages
3. Add tests for new functionality
4. Ensure all tests pass: `colcon test`
5. Check for compiler warnings with strict flags
6. Update documentation if needed
7. Create a pull request with detailed description
8. Link any related issues

## Reporting Bugs

Use GitHub Issues to report bugs. Provide:

1. **Description**: Clear summary of the problem
2. **Reproduction**: Steps to reproduce the issue
3. **Expected behavior**: What should happen
4. **Actual behavior**: What actually happens
5. **Environment**: 
   - ROS 2 version (e.g., Lyrical)
   - Hardware (e.g., INA3221, I2C bus)
   - OS (Ubuntu version)
6. **Logs**: Relevant error messages or diagnostics

Example:
```
**Title**: Node crashes when sensor I2C disconnected

**Description**
When the INA3221 I2C device is disconnected during operation, 
the node crashes with a segfault instead of reporting ERROR diagnostics.

**Reproduction**
1. Launch node with INA3221 configured
2. Disconnect I2C cable from device
3. Observe crash in logs

**Expected**
Node should report ERROR diagnostic status for missing sensor and continue running.

**Actual**
Node exits with segmentation fault.

**Environment**
- ROS 2 Lyrical
- Ubuntu 24.04 LTS
- INA3221 on /sys/class/hwmon/hwmon5

**Logs**
[ERROR] [hwmon_node]: Segmentation fault (core dumped)
```

## Feature Requests

Suggest new features through GitHub Issues with:
1. Clear description of the feature
2. Use case and motivation
3. Possible implementation approach
4. Any potential concerns

## Code Review Guidelines

When reviewing pull requests:

1. **Functionality**: Does it work as intended?
2. **Safety**: Are error conditions handled? Are there potential crashes?
3. **Design**: Is the approach sound? Does it follow ROS 2 best practices?
4. **Testing**: Are tests comprehensive? Do they cover error cases?
5. **Performance**: Are there efficiency concerns?
6. **Documentation**: Are public APIs documented? Is usage clear?
7. **Style**: Does code follow conventions and style guidelines?

Constructive feedback should be:
- Specific: Point to exact code locations
- Actionable: Suggest concrete improvements
- Respectful: Assume good intent
- Educational: Explain the reasoning

## Documentation

Contributing to documentation is valued equally with code:

- Update README.md for user-facing changes
- Add docstring comments to public APIs
- Add troubleshooting entries for known issues

## Architecture Notes

Key components:
- **SysfsReader**: Low-level sysfs I/O abstraction
- **HwmonConfig structs**: Parameter validation and storage
- **HwmonNode**: Main ROS 2 node with timer callback
- **Diagnostic publishing**: diagnostic_updater integration

When making changes:
- Keep SysfsReader independent and testable
- Maintain separation between data models and ROS logic
- Use dependency injection for testability (mock SysfsReader)
- Follow RAII patterns for resource cleanup

## Testing Strategy

### Unit Tests
- Mock SysfsReader with gmock
- Test parameter validation
- Test threshold logic with boundary values
- Test error handling paths

### Integration Tests
- Use real hwmon interface in docker/test environment
- Test sensor detection and recovery
- Test dynamic reconfiguration
- Test composable node loading

### Hardware Tests (Optional)
- Verify with actual INA3221 or similar sensor
- Test sensor failure scenarios
- Validate published values against sysfs reads

## Building Documentation Locally

Build the README and view locally:
```bash
# View README in default browser
open README.md  # macOS
xdg-open README.md  # Linux
```

## Questions?

- Check existing GitHub Issues
- Review [README.md](README.md) troubleshooting section
- Ask in discussions on GitHub

## Recognition

Thank you for helping improve this project!
