#include "icm-20948/icm-20948.hpp"

#include "icm-20948/registers.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <utility>
namespace icm20948 {

namespace {

struct RawSample {
  std::int16_t x{};
  std::int16_t y{};
  std::int16_t z{};
};

std::error_code make_error(std::errc error) { return std::make_error_code(error); }

template <typename Value> std::expected<Value, std::error_code> make_unexpected(std::errc error) { return std::unexpected(make_error(error)); }

bool is_enabled(AccelerometerRange range) { return range != AccelerometerRange::disabled; }

bool is_enabled(GyroscopeRange range) { return range != GyroscopeRange::disabled; }

bool is_enabled(MagnetometerMode mode) { return mode != MagnetometerMode::disabled; }

constexpr std::int32_t micro_g_per_g = 1'000'000;
constexpr std::int32_t milli_degrees_per_degree = 1'000;
constexpr std::int32_t accelerometer_sensitivity_g_2 = 16'384;
constexpr std::int32_t accelerometer_sensitivity_g_4 = 8'192;
constexpr std::int32_t accelerometer_sensitivity_g_8 = 4'096;
constexpr std::int32_t accelerometer_sensitivity_g_16 = 2'048;
constexpr std::int32_t gyroscope_scale_numerator_dps_250 = 1'000;
constexpr std::int32_t gyroscope_scale_numerator_high_ranges = 10'000;
constexpr std::int32_t gyroscope_scale_denominator_dps_250 = 131;
constexpr std::int32_t gyroscope_scale_denominator_dps_500 = 655;
constexpr std::int32_t gyroscope_scale_denominator_dps_1000 = 328;
constexpr std::int32_t gyroscope_scale_denominator_dps_2000 = 164;
constexpr std::int32_t magnetometer_sensitivity_nanotesla_per_lsb = 150;
constexpr std::int32_t die_temperature_scale_numerator = 100;
constexpr std::int32_t die_temperature_scale_denominator = 33387;
constexpr std::int32_t die_temperature_offset_value = 294'150;

struct FilterOption {
  std::uint16_t frequency_hz;
  std::uint8_t register_value;
};

constexpr std::array<FilterOption, 7> accelerometer_filter_options{{
    {246, 1},
    {111, 2},
    {50, 3},
    {24, 4},
    {12, 5},
    {6, 6},
    {473, 7},
}};

constexpr std::array<FilterOption, 7> gyroscope_filter_options{{
    {197, 0},
    {152, 1},
    {120, 2},
    {51, 3},
    {24, 4},
    {12, 5},
    {6, 6},
}};

constexpr std::array<FilterOption, 8> die_temperature_filter_options{{
    {7932, 0},
    {218, 1},
    {124, 2},
    {66, 3},
    {34, 4},
    {17, 5},
    {9, 6},
    {7932, 7},
}};

std::int32_t divide_rounding_to_nearest(std::int64_t numerator, std::int64_t denominator) {
  if(numerator >= 0) {
    return static_cast<std::int32_t>((numerator + (denominator / 2)) / denominator);
  }

  return static_cast<std::int32_t>((numerator - (denominator / 2)) / denominator);
}

template <std::size_t Size>
FilterOption find_nearest_filter_option(std::array<FilterOption, Size> const &options, std::uint16_t requested_frequency_hz) {
  auto nearest = options.front();
  auto nearest_distance = static_cast<std::uint32_t>(requested_frequency_hz > nearest.frequency_hz ? requested_frequency_hz - nearest.frequency_hz
                                                                                                   : nearest.frequency_hz - requested_frequency_hz);

  for(auto const &option : options) {
    auto const distance = static_cast<std::uint32_t>(requested_frequency_hz > option.frequency_hz ? requested_frequency_hz - option.frequency_hz
                                                                                                  : option.frequency_hz - requested_frequency_hz);
    if(distance < nearest_distance) {
      nearest = option;
      nearest_distance = distance;
    }
  }

  return nearest;
}

std::uint16_t normalize_accelerometer_filter_frequency(std::uint16_t filter_frequency_hz) {
  if(filter_frequency_hz == 0) {
    return 0;
  }

  return find_nearest_filter_option(accelerometer_filter_options, filter_frequency_hz).frequency_hz;
}

std::uint16_t normalize_gyroscope_filter_frequency(std::uint16_t filter_frequency_hz) {
  if(filter_frequency_hz == 0) {
    return 0;
  }

  return find_nearest_filter_option(gyroscope_filter_options, filter_frequency_hz).frequency_hz;
}

FilterOption normalize_die_temperature_filter(std::uint16_t filter_frequency_hz) {
  if(filter_frequency_hz == 0) {
    return die_temperature_filter_options.front();
  }

  return find_nearest_filter_option(die_temperature_filter_options, filter_frequency_hz);
}

AccelerometerConfiguration normalize_accelerometer_configuration(AccelerometerConfiguration configuration) {
  configuration.filter_frequency_hz = normalize_accelerometer_filter_frequency(configuration.filter_frequency_hz);
  return configuration;
}

GyroscopeConfiguration normalize_gyroscope_configuration(GyroscopeConfiguration configuration) {
  configuration.filter_frequency_hz = normalize_gyroscope_filter_frequency(configuration.filter_frequency_hz);
  return configuration;
}

DieTemperatureConfiguration normalize_die_temperature_configuration(DieTemperatureConfiguration configuration) {
  configuration.filter_frequency_hz = normalize_die_temperature_filter(configuration.filter_frequency_hz).frequency_hz;
  return configuration;
}

std::int32_t accelerometer_sensitivity_lsb_per_g(AccelerometerRange range) {
  switch(range) {
  case AccelerometerRange::g_2:
    return accelerometer_sensitivity_g_2;
  case AccelerometerRange::g_4:
    return accelerometer_sensitivity_g_4;
  case AccelerometerRange::g_8:
    return accelerometer_sensitivity_g_8;
  case AccelerometerRange::g_16:
    return accelerometer_sensitivity_g_16;
  case AccelerometerRange::disabled:
    break;
  }

  return 1;
}

std::int32_t gyroscope_scale_numerator(GyroscopeRange range) {
  switch(range) {
  case GyroscopeRange::dps_250:
    return gyroscope_scale_numerator_dps_250;
  case GyroscopeRange::dps_500:
  case GyroscopeRange::dps_1000:
  case GyroscopeRange::dps_2000:
    return gyroscope_scale_numerator_high_ranges;
  case GyroscopeRange::disabled:
    break;
  }

  return 1;
}

std::int32_t gyroscope_scale_denominator(GyroscopeRange range) {
  switch(range) {
  case GyroscopeRange::dps_250:
    return gyroscope_scale_denominator_dps_250;
  case GyroscopeRange::dps_500:
    return gyroscope_scale_denominator_dps_500;
  case GyroscopeRange::dps_1000:
    return gyroscope_scale_denominator_dps_1000;
  case GyroscopeRange::dps_2000:
    return gyroscope_scale_denominator_dps_2000;
  case GyroscopeRange::disabled:
    break;
  }

  return 1;
}

RawSample decode_sample(std::array<std::uint8_t, registers::vector_sample_size> const &raw_data) {
  return {
      static_cast<std::int16_t>((static_cast<std::uint16_t>(raw_data[registers::x_msb_index]) << registers::byte_shift) |
                                raw_data[registers::x_lsb_index]),
      static_cast<std::int16_t>((static_cast<std::uint16_t>(raw_data[registers::y_msb_index]) << registers::byte_shift) |
                                raw_data[registers::y_lsb_index]),
      static_cast<std::int16_t>((static_cast<std::uint16_t>(raw_data[registers::z_msb_index]) << registers::byte_shift) |
                                raw_data[registers::z_lsb_index]),
  };
}

std::int16_t decode_scalar(std::array<std::uint8_t, registers::scalar_sample_size> const &raw_data) {
  return static_cast<std::int16_t>((static_cast<std::uint16_t>(raw_data[0]) << registers::byte_shift) | raw_data[1]);
}

Acceleration scale_accelerometer_reading(RawSample const &raw, AccelerometerRange range) {
  auto const sensitivity = accelerometer_sensitivity_lsb_per_g(range);
  return {
      .x = divide_rounding_to_nearest(static_cast<std::int64_t>(raw.x) * micro_g_per_g, sensitivity),
      .y = divide_rounding_to_nearest(static_cast<std::int64_t>(raw.y) * micro_g_per_g, sensitivity),
      .z = divide_rounding_to_nearest(static_cast<std::int64_t>(raw.z) * micro_g_per_g, sensitivity),
  };
}

AngularRate scale_gyroscope_reading(RawSample const &raw, GyroscopeRange range) {
  auto const numerator = gyroscope_scale_numerator(range);
  auto const denominator = gyroscope_scale_denominator(range);
  return {
      .x = divide_rounding_to_nearest(static_cast<std::int64_t>(raw.x) * numerator, denominator),
      .y = divide_rounding_to_nearest(static_cast<std::int64_t>(raw.y) * numerator, denominator),
      .z = divide_rounding_to_nearest(static_cast<std::int64_t>(raw.z) * numerator, denominator),
  };
}

MagneticFlux scale_magnetometer_reading(RawSample const &raw) {
  return {
      .x = static_cast<std::int32_t>(raw.x) * magnetometer_sensitivity_nanotesla_per_lsb,
      .y = static_cast<std::int32_t>(raw.y) * magnetometer_sensitivity_nanotesla_per_lsb,
      .z = static_cast<std::int32_t>(raw.z) * magnetometer_sensitivity_nanotesla_per_lsb,
  };
}

DieTemperature scale_die_temperature_reading(std::int16_t raw) {
  return {
      .value = die_temperature_offset_value +
                     divide_rounding_to_nearest(static_cast<std::int64_t>(raw) * die_temperature_scale_numerator * milli_degrees_per_degree,
                                                die_temperature_scale_denominator),
  };
}

class ChipSelectGuard {
public:
  explicit ChipSelectGuard(Transport const &transport) : transport_(transport) {
    if(transport_.chip_select) {
      transport_.chip_select(true);
      selected_ = true;
    }
  }

  ChipSelectGuard(ChipSelectGuard const &) = delete;
  ChipSelectGuard &operator=(ChipSelectGuard const &) = delete;

  ~ChipSelectGuard() {
    if(selected_) {
      transport_.chip_select(false);
    }
  }

private:
  Transport const &transport_;
  bool selected_{false};
};

std::error_code read_register(Transport const &transport, std::uint8_t address, std::uint8_t *buffer, std::uint32_t length) {
  if(!transport.chip_select || !transport.read) {
    return make_error(std::errc::bad_address);
  }

  ChipSelectGuard chip_select_guard(transport);
  auto const result = transport.read(static_cast<std::uint8_t>(address | registers::read_mask),
                                     std::as_writable_bytes(std::span{buffer, static_cast<std::size_t>(length)}));
  return result;
}

std::error_code write_register(Transport const &transport, std::uint8_t address, std::uint8_t const *buffer, std::uint32_t length) {
  if(!transport.chip_select || !transport.write) {
    return make_error(std::errc::bad_address);
  }

  ChipSelectGuard chip_select_guard(transport);
  auto const result = transport.write(address, std::as_bytes(std::span{buffer, static_cast<std::size_t>(length)}));
  return result;
}

std::error_code ensure_connected(bool connected) {
  if(!connected) {
    return make_error(std::errc::not_connected);
  }

  return {};
}

std::error_code ensure_awake_and_connected(bool connected, bool sleeping) {
  auto connected_result = ensure_connected(connected);
  if(connected_result) {
    return connected_result;
  }

  if(sleeping) {
    return make_error(std::errc::operation_not_permitted);
  }

  return {};
}

template <typename Configuration> std::error_code store_configuration(bool connected, Configuration &target, Configuration const &source) {
  auto connected_result = ensure_connected(connected);
  if(connected_result) {
    return connected_result;
  }

  target = source;
  return {};
}

std::error_code select_user_bank(Transport const &transport, std::uint8_t bank) {
  return write_register(transport, registers::reg_bank_sel_register, &bank, 1);
}

std::expected<std::uint8_t, std::error_code> read_power_management_1(Transport const &transport) {
  std::uint8_t power_management_1{};
  auto read_result = read_register(transport, registers::pwr_mgmt_1_register, &power_management_1, 1);
  if(read_result) {
    return std::unexpected(read_result);
  }

  return power_management_1;
}

std::error_code set_sleep_state(Transport const &transport, bool sleeping) {
  auto power_management_1 = read_power_management_1(transport);
  if(!power_management_1) {
    return power_management_1.error();
  }

  auto value = power_management_1.value();
  if(sleeping) {
    value = static_cast<std::uint8_t>(value | registers::pwr_mgmt_1_sleep_mask);
  } else {
    value = static_cast<std::uint8_t>(value & ~registers::pwr_mgmt_1_sleep_mask);
  }

  return write_register(transport, registers::pwr_mgmt_1_register, &value, 1);
}

std::error_code apply_die_temperature_configuration(Transport const &transport, DieTemperatureConfiguration const &configuration) {
  std::uint8_t power_management_1{};
  auto read_power_management_result = read_register(transport, registers::pwr_mgmt_1_register, &power_management_1, 1);
  if(read_power_management_result) {
    return read_power_management_result;
  }

  if(configuration.enabled) {
    power_management_1 = static_cast<std::uint8_t>(power_management_1 & ~registers::pwr_mgmt_1_temp_disable_mask);
  } else {
    power_management_1 = static_cast<std::uint8_t>(power_management_1 | registers::pwr_mgmt_1_temp_disable_mask);
  }

  auto write_power_management_result = write_register(transport, registers::pwr_mgmt_1_register, &power_management_1, 1);
  if(write_power_management_result) {
    return write_power_management_result;
  }

  auto select_bank_2_result = select_user_bank(transport, registers::user_bank_2);
  if(select_bank_2_result) {
    return select_bank_2_result;
  }

  auto const normalized_filter = normalize_die_temperature_filter(configuration.filter_frequency_hz);
  std::uint8_t temp_config_value = static_cast<std::uint8_t>(normalized_filter.register_value & registers::temp_config_digital_low_pass_filter_mask);
  auto write_temp_config_result = write_register(transport, registers::temp_config_register, &temp_config_value, 1);
  auto restore_bank_0_result = select_user_bank(transport, registers::user_bank_0);

  if(write_temp_config_result) {
    return write_temp_config_result;
  }

  if(restore_bank_0_result) {
    return restore_bank_0_result;
  }

  return {};
}

} // namespace

ICM20948::ICM20948(Transport transport) noexcept : transport(std::move(transport)) {}

ICM20948::ICM20948(ICM20948 &&other) noexcept
    : transport(std::exchange(other.transport, {})), connected(std::exchange(other.connected, false)), sleeping(std::exchange(other.sleeping, true)),
      accelerometer_config(std::exchange(other.accelerometer_config, {})), gyroscope_config(std::exchange(other.gyroscope_config, {})),
      magnetometer_config(std::exchange(other.magnetometer_config, {})), die_temperature_config(std::exchange(other.die_temperature_config, {})) {}

ICM20948 &ICM20948::operator=(ICM20948 &&other) noexcept {
  if(this == &other) {
    return *this;
  }

  transport = std::exchange(other.transport, {});
  connected = std::exchange(other.connected, false);
  sleeping = std::exchange(other.sleeping, true);
  accelerometer_config = std::exchange(other.accelerometer_config, {});
  gyroscope_config = std::exchange(other.gyroscope_config, {});
  magnetometer_config = std::exchange(other.magnetometer_config, {});
  die_temperature_config = std::exchange(other.die_temperature_config, {});

  return *this;
}

std::expected<ICM20948, std::error_code> ICM20948::create(Transport transport) {
  if(!transport.chip_select || !transport.read || !transport.write) {
    return make_unexpected<ICM20948>(std::errc::invalid_argument);
  }
  transport.chip_select(false);
  return ICM20948(std::move(transport));
}

std::error_code ICM20948::connect() {
  std::uint8_t who_am_i{};
  auto read_result = read_register(transport, registers::who_am_i_register, &who_am_i, 1);

  if(read_result) {
    connected = false;
    sleeping = false;
    return read_result;
  }

  if(who_am_i != registers::who_am_i_value) {
    connected = false;
    sleeping = false;
    return make_error(std::errc::no_such_device_or_address);
  }

  auto power_management_1 = read_power_management_1(transport);
  if(!power_management_1) {
    connected = false;
    sleeping = false;
    return power_management_1.error();
  }

  connected = true;
  sleeping = (power_management_1.value() & registers::pwr_mgmt_1_sleep_mask) != 0;
  accelerometer_config = {};
  gyroscope_config = {};
  magnetometer_config = {};
  die_temperature_config = {};
  return {};
}

bool ICM20948::is_connected() const noexcept { return connected; }

std::error_code ICM20948::sleep() {
  auto not_operational = ensure_connected(connected);
  if(not_operational) {
    return not_operational;
  }

  auto sleep_result = set_sleep_state(transport, true);
  if(sleep_result) {
    return sleep_result;
  }

  sleeping = true;
  return {};
}

std::error_code ICM20948::wake() {
  auto not_operational = ensure_connected(connected);
  if(not_operational) {
    return not_operational;
  }

  auto wake_result = set_sleep_state(transport, false);
  if(wake_result) {
    return wake_result;
  }

  sleeping = false;
  return {};
}

bool ICM20948::is_sleeping() const noexcept { return sleeping; }

std::error_code ICM20948::configure_accelerometer(AccelerometerConfiguration configuration) {
  return store_configuration(connected, accelerometer_config, normalize_accelerometer_configuration(configuration));
}

std::error_code ICM20948::configure_gyroscope(GyroscopeConfiguration configuration) {
  return store_configuration(connected, gyroscope_config, normalize_gyroscope_configuration(configuration));
}

std::error_code ICM20948::configure_magnetometer(MagnetometerConfiguration configuration) {
  return store_configuration(connected, magnetometer_config, configuration);
}

std::error_code ICM20948::configure_die_temperature(DieTemperatureConfiguration configuration) {
  auto not_operational = ensure_connected(connected);
  if(not_operational) {
    return not_operational;
  }

  auto normalized_configuration = normalize_die_temperature_configuration(configuration);

  auto apply_result = apply_die_temperature_configuration(transport, normalized_configuration);
  if(apply_result) {
    return apply_result;
  }

  die_temperature_config = normalized_configuration;
  return {};
}

std::expected<Acceleration, std::error_code> ICM20948::read_acceleration() {
  auto not_operational = ensure_awake_and_connected(connected, sleeping);
  if(not_operational) {
    return std::unexpected(not_operational);
  }

  if(!is_enabled(accelerometer_config.range)) {
    return make_unexpected<Acceleration>(std::errc::operation_not_permitted);
  }

  std::array<std::uint8_t, registers::vector_sample_size> raw_data{};
  auto read_result = read_register(transport, registers::accelerometer_data_register, raw_data.data(), raw_data.size());
  if(read_result) {
    return std::unexpected(read_result);
  }

  return scale_accelerometer_reading(decode_sample(raw_data), accelerometer_config.range);
}

std::expected<AngularRate, std::error_code> ICM20948::read_angular_rate() {
  auto not_operational = ensure_awake_and_connected(connected, sleeping);
  if(not_operational) {
    return std::unexpected(not_operational);
  }

  if(!is_enabled(gyroscope_config.range)) {
    return make_unexpected<AngularRate>(std::errc::operation_not_permitted);
  }

  std::array<std::uint8_t, registers::vector_sample_size> raw_data{};
  auto read_result = read_register(transport, registers::gyroscope_data_register, raw_data.data(), raw_data.size());
  if(read_result) {
    return std::unexpected(read_result);
  }

  return scale_gyroscope_reading(decode_sample(raw_data), gyroscope_config.range);
}

std::expected<MagneticFlux, std::error_code> ICM20948::read_magnetometer() {
  auto not_operational = ensure_awake_and_connected(connected, sleeping);
  if(not_operational) {
    return std::unexpected(not_operational);
  }

  if(!is_enabled(magnetometer_config.mode)) {
    return make_unexpected<MagneticFlux>(std::errc::operation_not_permitted);
  }

  std::array<std::uint8_t, registers::vector_sample_size> raw_data{};
  auto read_result = read_register(transport, registers::magnetometer_data_register, raw_data.data(), raw_data.size());
  if(read_result) {
    return std::unexpected(read_result);
  }

  return scale_magnetometer_reading(decode_sample(raw_data));
}

std::expected<DieTemperature, std::error_code> ICM20948::read_die_temperature() {
  auto not_operational = ensure_awake_and_connected(connected, sleeping);
  if(not_operational) {
    return std::unexpected(not_operational);
  }

  if(!die_temperature_config.enabled) {
    return make_unexpected<DieTemperature>(std::errc::operation_not_permitted);
  }

  std::array<std::uint8_t, registers::scalar_sample_size> raw_data{};
  auto read_result = read_register(transport, registers::die_temperature_data_register, raw_data.data(), raw_data.size());
  if(read_result) {
    return std::unexpected(read_result);
  }

  return scale_die_temperature_reading(decode_scalar(raw_data));
}

} // namespace icm20948
