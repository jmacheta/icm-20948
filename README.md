# icm-20948

`icm-20948` is a modern C++ driver for the TDK InvenSense ICM-20948 9-axis IMU.
It provides a clear high-level API for device connection, power-state control, sensor configuration, and reading accelerometer, gyroscope, magnetometer, and die-temperature data through user-supplied transport callbacks.

## Highlights

- Standalone driver library with no external runtime dependencies.
- Built around modern C++23 features, including `std::expected`, `std::span`, strong typing, and explicit error handling.
- Transport-agnostic design that integrates cleanly with platform-specific SPI communication layers.
- Platform-independent source design suitable for embedded and desktop targets with a C++23 toolchain and CMake.
- Consistent physical units for sensor readings and dedicated configuration objects for each sensing block.

## Add to your CMake project

Choose one of the following integration options.

### Option A — `CPM.cmake`

## Installation

This project uses CPM.cmake for source consumption and also supports installable CMake package metadata.

Using CPM.cmake in your project:

```cmake
# Download CPM.cmake (pin version as needed)
file(DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.0/CPM.cmake
  ${CMAKE_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=2020b4fc42dba44817983e06342e682ecfc3d2f484a581f11cc5731fbe4dce8a
)
include(${CMAKE_BINARY_DIR}/cmake/CPM.cmake)

# Add icm-2048 (pin to a released tag or major stream)
CPMAddPackage("gh:jmacheta/icm-20948#v1")

# Link to your library
add_executable(app main.c)
target_link_libraries(app PRIVATE emc::icm-20948)
```

## Build with CMakePresets

The project uses CMake and exposes the `enc::icm-20948` target.
Unit tests are optional and pull GoogleTest only when test support is enabled.

Configure the project:

```sh
cmake --preset native-gcc
```

Build the Debug targets:

```sh
cmake --build --preset native-gcc-all
```

## Porting guide

To integrate the driver on a new platform, only the transport layer needs to be adapted.

- Provide a `chip_select(bool)` callback that asserts CS on `true` and releases it on `false`.
- Provide a `read(std::uint8_t, std::span<std::byte>)` callback that sends the register address first and then reads the requested payload.
- Provide a `write(std::uint8_t, std::span<std::byte const>)` callback that sends the register address first and then writes the payload bytes.
- Translate platform or HAL failures to `std::error_code` values so the API can propagate transport errors without exceptions.
- Keep the SPI transaction ordering compatible with the device timing requirements defined by your target platform.

The driver is platform-independent and does not depend on any operating system, vendor SDK, or runtime framework.

## API example

```cpp
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <system_error>

#include <icm-20948/icm-20948.hpp>

std::error_code platform_spi_read(std::uint8_t reg, std::span<std::byte> data);
std::error_code platform_spi_write(std::uint8_t reg, std::span<std::byte const> data);
void platform_chip_select(bool active);

std::expected<icm20948::Acceleration, std::error_code> read_acceleration_sample() {
  auto driver_result = icm20948::ICM20948::create({
    .chip_select = platform_chip_select,
    .read = platform_spi_read,
    .write = platform_spi_write,
  });

  if(!driver_result) {
    return std::unexpected(driver_result.error());
  }

  auto driver = std::move(*driver_result);

  if(auto ec = driver.connect()) {
    return std::unexpected(ec);
  }

  if(driver.is_sleeping()) {
    if(auto ec = driver.wake()) {
      return std::unexpected(ec);
    }
  }

  if(auto ec = driver.configure_accelerometer({
       .range = icm20948::AccelerometerRange::g_4,
       .output_data_rate_hz = 1125,
       .filter_frequency_hz = 111,
     })) {
    return std::unexpected(ec);
  }

  return driver.read_acceleration();
}
```
