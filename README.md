![Slamming Works brand lockup](assets/slw-lockup.svg)

# hwmon_diagnostic_updater

Slamming Works' ROS 2 node for reading Linux hwmon sysfs values and publishing diagnostics, with optional battery-state fields.

## Demo

There is no hardware demo video or GIF in this repository yet.

## Hardware

The node reads sensor values from Linux hwmon sysfs, including configured devices such as the INA3221. The repository includes an [INA3221 example configuration](config/ina3221_example.yaml), not a hardware BOM or schematic.

## Build and run

From the ROS 2 workspace root:

```bash
rosdep install --from-paths src --ignore-src -y
colcon build --packages-up-to hwmon_diagnostic_updater
source install/setup.bash
```

```bash
ros2 launch hwmon_diagnostic_updater hwmon_diagnostic_updater.launch.py
ros2 run hwmon_diagnostic_updater hwmon_diagnostic_updater_node --ros-args --params-file my_hwmon.yaml
```

Also available as the component `hwmon_diagnostic_updater::HwmonNode`.

## Interfaces

| Direction | Topic | Type | Notes |
|---|---|---|---|
| Pub | `/diagnostics` | `diagnostic_msgs/DiagnosticArray` | One `hwmon_status` entry with every metric, at `update_rate_hz` |
| Pub | `battery_topic` (`/battery_state`) | `sensor_msgs/BatteryState` | Only fields from metrics with `battery_field`; the rest are NaN |

## Configuration

ROS params files can't hold lists of dicts, so sensors and metrics are listed by name and configured under that name:

```yaml
hwmon_diagnostic_updater_node:
  ros__parameters:
    update_rate_hz: 2.0
    battery_topic: /battery_state
    sensors: [power]                          # names of the sensor blocks below
    power:
      base_path: /sys/bus/i2c/devices/1-0040  # see "base_path" below
      enabled: true
      init_writes: ["shunt1_resistor=20000"]  # optional, written once at startup
      metrics: [battery_voltage]              # names of the metric blocks below
      battery_voltage:
        file: in1_input                       # relative to base_path
        scale: 0.001                          # value = raw * scale + offset
        offset: 0.0
        battery_field: voltage                # voltage | current | temperature | omit
        data_timeout_ms: 1000
        warn_min: 13.6                        # any threshold can be omitted
        error_min: 12.4
        warn_max: 16.9
        error_max: 17.0
```

| Parameter | Default | Description |
|---|---|---|
| `update_rate_hz` | `10.0` | How often metrics are read and `/diagnostics` + `BatteryState` are published |
| `battery_topic` | `/battery_state` | `BatteryState` topic |
| `sensors` | `[]` | Sensor names; at least one is required or the node exits |
| `<sensor>.base_path` | – | Absolute path, see below |
| `<sensor>.enabled` | `true` | Skip the sensor when `false` |
| `<sensor>.init_writes` | `[]` | `"file=value"` strings written to `base_path` at startup |
| `<sensor>.metrics` | `[]` | Metric names; at least one is required |
| `<sensor>.<metric>.file` | – | sysfs file to read |
| `<sensor>.<metric>.scale` / `offset` | `1.0` / `0.0` | Unit conversion (kernel units: mV, mA, m°C, µW) |
| `<sensor>.<metric>.battery_field` | `""` | Copy the value into this `BatteryState` field |
| `<sensor>.<metric>.data_timeout_ms` | `1000` | ERROR if no successful read for this long |
| `<sensor>.<metric>.warn_min/warn_max/error_min/error_max` | unset | WARN/ERROR when outside the range |
| `qos_reliability`, `qos_durability`, `qos_depth` | | Declared and validated, but not applied to the publisher yet |

All parameters are read once at startup.

**`base_path`**: `/sys/class/hwmon/hwmonN` works, but `N` can change between boots. Use the device path instead, e.g. `/sys/bus/i2c/devices/1-0040`: if the path has exactly one `hwmon*` child, the node descends into it (`1-0040/hwmon/hwmon4`). The resolved path is logged at startup.

## Features

- Multiple sensors per node, each with any number of metrics, configured entirely through parameters.
- A failing sensor doesn't stop the others; metrics that can't be read are reported in diagnostics and left as NaN in `BatteryState`.
- Non-finite values (e.g. from scale/offset conversion) are rejected, and repeated errors are log-throttled.
- Parameters and sysfs paths are validated at startup.

## Diagnostic status

- **ERROR**: a metric file is missing, a value is outside `error_min/error_max`, no successful read within `data_timeout_ms`, or 10+ consecutive read failures on a sensor.
- **WARN**: a value is outside `warn_min/warn_max`, or 3+ consecutive read failures.
- Each metric shows up as `<sensor>_<metric>` in the diagnostic values.

## INA3221 notes

The kernel `ina3221` driver exposes `in1-3_input` (bus voltage, mV), `in4-6_input` (shunt voltage, µV), `curr1-3_input` (mA), `curr4_input` (sum of channels, mA) and `shunt1-3_resistor` (µΩ). There is no `power*_input`. If the device isn't bound by the device tree:

```bash
echo ina3221 0x40 | sudo tee /sys/bus/i2c/devices/i2c-1/new_device
```

See [config/ina3221_example.yaml](config/ina3221_example.yaml).

## Status

**Partly works.** Sysfs metrics, diagnostics and optional `BatteryState` mapping are implemented. The QoS parameters are declared and validated but do not yet change publisher QoS.

## License

MIT. See [LICENSE](LICENSE).
