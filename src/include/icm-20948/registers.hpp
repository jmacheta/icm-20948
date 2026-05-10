#ifndef ICM_20948_REGISTERS_HPP_
#define ICM_20948_REGISTERS_HPP_

#include <cstddef>
#include <cstdint>

namespace icm20948::registers {

inline constexpr std::uint8_t read_mask = 0x80;
inline constexpr std::uint8_t who_am_i_register = 0x00;
inline constexpr std::uint8_t who_am_i_value = 0xEA;
inline constexpr std::uint8_t pwr_mgmt_1_register = 0x06;
inline constexpr std::uint8_t pwr_mgmt_1_sleep_mask = 0x40;
inline constexpr std::uint8_t pwr_mgmt_1_temp_disable_mask = 0x08;
inline constexpr std::uint8_t accelerometer_data_register = 0x2D;
inline constexpr std::uint8_t gyroscope_data_register = 0x33;
inline constexpr std::uint8_t magnetometer_data_register = 0x3B;
inline constexpr std::uint8_t die_temperature_data_register = 0x39;
inline constexpr std::uint8_t reg_bank_sel_register = 0x7F;
inline constexpr std::uint8_t user_bank_0 = 0x00;
inline constexpr std::uint8_t user_bank_2 = 0x20;
inline constexpr std::uint8_t temp_config_register = 0x53;
inline constexpr std::uint8_t temp_config_digital_low_pass_filter_mask = 0x07;
inline constexpr std::uint32_t wakeup_delay_us = 10;
inline constexpr std::size_t vector_sample_size = 6;
inline constexpr std::size_t scalar_sample_size = 2;
inline constexpr unsigned byte_shift = 8U;
inline constexpr std::size_t x_msb_index = 0;
inline constexpr std::size_t x_lsb_index = 1;
inline constexpr std::size_t y_msb_index = 2;
inline constexpr std::size_t y_lsb_index = 3;
inline constexpr std::size_t z_msb_index = 4;
inline constexpr std::size_t z_lsb_index = 5;

} // namespace icm20948::registers

#endif
