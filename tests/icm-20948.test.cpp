#include "icm-20948/icm-20948.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace {

constexpr std::uint8_t read_mask = 0x80;
constexpr std::uint8_t who_am_i_register = 0x00;
constexpr std::uint8_t who_am_i_value = 0xEA;
constexpr std::uint8_t pwr_mgmt_1_register = 0x06;
constexpr std::uint8_t pwr_mgmt_1_sleep_mask = 0x40;
constexpr std::uint8_t reg_bank_sel_register = 0x7F;
constexpr std::uint8_t temp_config_register = 0x53;
constexpr std::uint8_t accelerometer_data_register = 0x2D;
constexpr std::uint8_t gyroscope_data_register = 0x33;
constexpr std::uint8_t magnetometer_data_register = 0x3B;
constexpr std::uint8_t die_temperature_data_register = 0x39;
constexpr std::size_t register_count = 128;
constexpr std::uint16_t custom_accelerometer_odr_hz = 225;
constexpr std::uint16_t custom_gyroscope_odr_hz = 400;
constexpr std::array<std::uint8_t, 6> accelerometer_raw_bytes{0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
constexpr std::array<std::uint8_t, 6> gyroscope_raw_bytes{0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
constexpr std::array<std::uint8_t, 6> magnetometer_raw_bytes{0x10, 0x11, 0x12, 0x13, 0x14, 0x15};
constexpr std::array<std::uint8_t, 2> die_temperature_raw_bytes{0x01, 0x20};

struct FakeTransportState {
  std::array<std::uint8_t, register_count> registers{};
  bool chip_select_active{false};
  std::size_t chip_select_enable_count{0};
  std::size_t chip_select_disable_count{0};
};

FakeTransportState state;

void set_chip_select(bool active) {
  state.chip_select_active = active;
  if(active) {
    ++state.chip_select_enable_count;
  } else {
    ++state.chip_select_disable_count;
  }
}

std::error_code read_register(std::uint8_t address, std::span<std::byte> data) {
  if(!state.chip_select_active) {
    return std::make_error_code(std::errc::operation_not_permitted);
  }

  auto const register_address = static_cast<std::uint8_t>(address & ~read_mask);
  for(std::size_t index = 0; index < data.size(); ++index) {
    data[index] = static_cast<std::byte>(state.registers[register_address + index]);
  }
  return {};
}

std::error_code write_register(std::uint8_t address, std::span<std::byte const> data) {
  if(!state.chip_select_active) {
    return std::make_error_code(std::errc::operation_not_permitted);
  }

  for(std::size_t index = 0; index < data.size(); ++index) {
    state.registers[address + index] = static_cast<std::uint8_t>(data[index]);
  }
  return {};
}

void reset_state() {
  state = {};
  state.registers[who_am_i_register] = who_am_i_value;
  state.registers[pwr_mgmt_1_register] = 0x00;
  for(std::size_t index = 0; index < accelerometer_raw_bytes.size(); ++index) {
    state.registers[accelerometer_data_register + index] = accelerometer_raw_bytes[index];
    state.registers[gyroscope_data_register + index] = gyroscope_raw_bytes[index];
    state.registers[magnetometer_data_register + index] = magnetometer_raw_bytes[index];
  }

  for(std::size_t index = 0; index < die_temperature_raw_bytes.size(); ++index) {
    state.registers[die_temperature_data_register + index] = die_temperature_raw_bytes[index];
  }
}

} // namespace

TEST(icm20948_api, create_rejects_incomplete_transport) {
  auto sensor = icm20948::ICM20948::create({});

  ASSERT_FALSE(sensor.has_value());
  EXPECT_EQ(sensor.error(), std::make_error_code(std::errc::invalid_argument));
}

TEST(icm20948_api, connect_sets_reset_default_configuration) {
  reset_state();

  auto sensor = icm20948::ICM20948::create({
      .chip_select = set_chip_select,
      .read = read_register,
      .write = write_register,
  });

  ASSERT_TRUE(sensor.has_value());
  auto &driver = sensor.value();

  ASSERT_EQ(driver.connect(), std::error_code{});
  EXPECT_TRUE(driver.is_connected());
  EXPECT_FALSE(state.chip_select_active);
  EXPECT_GT(state.chip_select_enable_count, 0U);
  EXPECT_EQ(state.chip_select_enable_count, state.chip_select_disable_count);

  EXPECT_TRUE(driver.read_acceleration().has_value());
  EXPECT_TRUE(driver.read_angular_rate().has_value());
  EXPECT_FALSE(driver.read_magnetometer().has_value());
  EXPECT_TRUE(driver.read_die_temperature().has_value());
}

TEST(icm20948_api, sensor_configuration_is_independent_per_block) {
  reset_state();

  auto sensor = icm20948::ICM20948::create({
      .chip_select = set_chip_select,
      .read = read_register,
      .write = write_register,
  });

  ASSERT_TRUE(sensor.has_value());
  auto &driver = sensor.value();

  ASSERT_EQ(driver.connect(), std::error_code{});

  EXPECT_EQ(driver.configure_accelerometer({
                .range = icm20948::AccelerometerRange::g_8,
                .output_data_rate_hz = custom_accelerometer_odr_hz,
                .filter_frequency_hz = 49,
            }),
            std::error_code{});

  EXPECT_EQ(driver.configure_gyroscope({
                .range = icm20948::GyroscopeRange::dps_1000,
                .output_data_rate_hz = custom_gyroscope_odr_hz,
                .filter_frequency_hz = 55,
            }),
            std::error_code{});

  EXPECT_EQ(driver.configure_magnetometer({
                .mode = icm20948::MagnetometerMode::continuous_20_hz,
            }),
            std::error_code{});

  EXPECT_EQ(driver.configure_die_temperature({
                .enabled = false,
                .filter_frequency_hz = 18,
            }),
            std::error_code{});

  EXPECT_TRUE(driver.read_acceleration().has_value());
  EXPECT_TRUE(driver.read_angular_rate().has_value());
  EXPECT_TRUE(driver.read_magnetometer().has_value());
  EXPECT_FALSE(driver.read_die_temperature().has_value());
}

TEST(icm20948_api, configure_requires_connected_device) {
  reset_state();

  auto sensor = icm20948::ICM20948::create({
      .chip_select = set_chip_select,
      .read = read_register,
      .write = write_register,
  });

  ASSERT_TRUE(sensor.has_value());

  auto result = sensor->configure_accelerometer({});
  EXPECT_EQ(result, std::make_error_code(std::errc::not_connected));
}

TEST(icm20948_api, readings_return_fixed_unit_values) {
  reset_state();

  auto sensor = icm20948::ICM20948::create({
      .chip_select = set_chip_select,
      .read = read_register,
      .write = write_register,
  });

  ASSERT_TRUE(sensor.has_value());
  auto &driver = sensor.value();
  ASSERT_EQ(driver.connect(), std::error_code{});

  ASSERT_EQ(driver.configure_accelerometer(icm20948::AccelerometerConfiguration{}), std::error_code{});
  ASSERT_EQ(driver.configure_gyroscope(icm20948::GyroscopeConfiguration{}), std::error_code{});
  ASSERT_EQ(driver.configure_magnetometer({.mode = icm20948::MagnetometerMode::continuous_20_hz}), std::error_code{});
  ASSERT_EQ(driver.configure_die_temperature({
                .enabled = true,
                .filter_frequency_hz = 33,
            }),
            std::error_code{});

  auto accelerometer = driver.read_acceleration();
  auto gyroscope = driver.read_angular_rate();
  auto magnetometer = driver.read_magnetometer();
  auto temperature = driver.read_die_temperature();

  ASSERT_TRUE(accelerometer.has_value());
  ASSERT_TRUE(gyroscope.has_value());
  ASSERT_TRUE(magnetometer.has_value());
  ASSERT_TRUE(temperature.has_value());
  EXPECT_FALSE(state.chip_select_active);
  EXPECT_EQ(state.chip_select_enable_count, state.chip_select_disable_count);

  EXPECT_EQ(accelerometer->x, 15747);
  EXPECT_EQ(accelerometer->y, 47119);
  EXPECT_EQ(accelerometer->z, 78491);
  EXPECT_EQ(gyroscope->x, 19626);
  EXPECT_EQ(magnetometer->x, 616950);
  EXPECT_EQ(temperature->value, 295013);
}

TEST(icm20948_api, sleep_and_wake_update_sensor_state) {
  reset_state();

  auto sensor = icm20948::ICM20948::create({
      .chip_select = set_chip_select,
      .read = read_register,
      .write = write_register,
  });

  ASSERT_TRUE(sensor.has_value());
  auto &driver = sensor.value();
  ASSERT_EQ(driver.connect(), std::error_code{});
  EXPECT_FALSE(driver.is_sleeping());

  ASSERT_EQ(driver.sleep(), std::error_code{});
  EXPECT_TRUE(driver.is_sleeping());
  EXPECT_NE((state.registers[pwr_mgmt_1_register] & pwr_mgmt_1_sleep_mask), 0);
  EXPECT_FALSE(driver.read_acceleration().has_value());

  ASSERT_EQ(driver.wake(), std::error_code{});
  EXPECT_FALSE(driver.is_sleeping());
  EXPECT_EQ((state.registers[pwr_mgmt_1_register] & pwr_mgmt_1_sleep_mask), 0);
  EXPECT_TRUE(driver.read_acceleration().has_value());
}

TEST(icm20948_api, connect_reflects_sleep_state_from_register) {
  reset_state();
  state.registers[pwr_mgmt_1_register] = pwr_mgmt_1_sleep_mask;

  auto sensor = icm20948::ICM20948::create({
      .chip_select = set_chip_select,
      .read = read_register,
      .write = write_register,
  });

  ASSERT_TRUE(sensor.has_value());
  ASSERT_EQ(sensor->connect(), std::error_code{});
  EXPECT_TRUE(sensor->is_sleeping());
}

TEST(icm20948_api, die_temperature_filter_maps_to_nearest_supported_frequency) {
  reset_state();

  auto sensor = icm20948::ICM20948::create({
      .chip_select = set_chip_select,
      .read = read_register,
      .write = write_register,
  });

  ASSERT_TRUE(sensor.has_value());
  auto &driver = sensor.value();
  ASSERT_EQ(driver.connect(), std::error_code{});

  ASSERT_EQ(driver.configure_die_temperature({
                .enabled = true,
                .filter_frequency_hz = 18,
            }),
            std::error_code{});

  EXPECT_EQ(state.registers[reg_bank_sel_register], 0x00);
  EXPECT_EQ(state.registers[temp_config_register], 0x05);
}

TEST(icm20948_api, connect_failure_leaves_device_disconnected) {
  reset_state();
  state.registers[who_am_i_register] = 0x00;

  auto sensor = icm20948::ICM20948::create({
      .chip_select = set_chip_select,
      .read = read_register,
      .write = write_register,
  });

  ASSERT_TRUE(sensor.has_value());
  auto connect_result = sensor->connect();
  EXPECT_EQ(connect_result, std::make_error_code(std::errc::no_such_device_or_address));
  EXPECT_FALSE(sensor->is_connected());
}
