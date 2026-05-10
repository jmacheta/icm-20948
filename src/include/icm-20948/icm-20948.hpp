#ifndef ICM_20948_HPP_
#define ICM_20948_HPP_

#include <cstdint>
#include <expected>
#include <functional>
#include <span>
#include <system_error>
namespace icm20948 {

/**
 * @brief Acceleration reading.
 *
 * Values are always returned in micro-g and follow the same contract
 * regardless of the currently selected accelerometer range.
 */
struct Acceleration {
  std::int32_t x{}; ///< Acceleration on the X axis [uG].
  std::int32_t y{}; ///< Acceleration on the Y axis [uG].
  std::int32_t z{}; ///< Acceleration on the Z axis [uG].
};

/**
 * @brief Angular-rate reading.
 *
 * Values are always returned in milli-degrees per second and follow the same
 * contract regardless of the currently selected gyroscope range.
 */
struct AngularRate {
  std::int32_t x{}; ///< Angular rate on the X axis [mdps].
  std::int32_t y{}; ///< Angular rate on the Y axis [mdps].
  std::int32_t z{}; ///< Angular rate on the Z axis [mdps].
};

/**
 * @brief Magnetic-field reading.
 *
 * Values are always returned in nanoTesla, so the result does not require
 * any additional configuration-dependent interpretation.
 */
struct MagneticFlux {
  std::int32_t x{}; ///< Magnetic-field intensity on the X axis [nT].
  std::int32_t y{}; ///< Magnetic-field intensity on the Y axis [nT].
  std::int32_t z{}; ///< Magnetic-field intensity on the Z axis [nT].
};

/**
 * @brief Internal die-temperature reading.
 *
 * The value is always returned in milli-Kelvin and describes the
 * silicon die temperature of the device.
 */
struct DieTemperature {
  std::int32_t value{}; ///< Device die temperature [mK].
};

/** @brief Default accelerometer ODR after synchronizing with the device reset state. */
inline constexpr std::uint16_t default_accelerometer_output_data_rate_hz = 1125;

/** @brief Default gyroscope ODR after synchronizing with the device reset state. */
inline constexpr std::uint16_t default_gyroscope_output_data_rate_hz = 1125;

/** @brief Default accelerometer low-pass filter frequency. */
inline constexpr std::uint16_t default_accelerometer_filter_frequency_hz = 246;

/** @brief Default gyroscope low-pass filter frequency. */
inline constexpr std::uint16_t default_gyroscope_filter_frequency_hz = 197;

/** @brief Default die-temperature low-pass filter frequency. */
inline constexpr std::uint16_t default_die_temperature_filter_frequency_hz = 7932;

/**
 * @brief Set of transport callbacks required by the driver.
 *
 * The struct contract requires all callbacks to be provided before calling
 * @ref ICM20948::create. The driver assumes that `chip_select(true)` asserts the
 * CS line before a transaction and that `chip_select(false)` releases it afterward.
 */
struct Transport {
  /** @brief Callback type used to control the chip-select line. */
  using chip_select_function = std::function<void(bool)>;

  /**
   * @brief Callback type used to read from the device.
   *
   * The first argument is the register address and the span contains only the
   * data bytes to be received. The transport must transmit the address first and
   * then read the requested data into the provided buffer.
   */
  using read_function = std::function<std::error_code(std::uint8_t, std::span<std::byte>)>;

  /**
   * @brief Callback type used to write to the device.
   *
   * The first argument is the register address and the span contains only the
   * data bytes to be written. The transport must transmit the address first and
   * then write the provided data bytes.
   */
  using write_function = std::function<std::error_code(std::uint8_t, std::span<std::byte const>)>;

  chip_select_function chip_select; ///< Callback that controls the chip-select line.
  read_function read;               ///< Callback that reads data from the device using a register address and destination buffer.
  write_function write;             ///< Callback that writes data to the device using a register address and source buffer.
};

/** @brief Available accelerometer operating ranges. */
enum class AccelerometerRange : std::uint8_t {
  disabled, ///< The accelerometer is disabled.
  g_2,      ///< Measurement range of ±2 g.
  g_4,      /// Measurement range of ±4 g.
  g_8,      ///< Measurement range of ±8 g.
  g_16,     ///< Measurement range of ±16 g.
};

/** @brief Available gyroscope operating ranges. */
enum class GyroscopeRange : std::uint8_t {
  disabled, ///< The gyroscope is disabled.
  dps_250,  ///< Measurement range of ±250 dps.
  dps_500,  ///< Measurement range of ±500 dps.
  dps_1000, ///< Measurement range of ±1000 dps.
  dps_2000, ///< Measurement range of ±2000 dps.
};

/** @brief Available magnetometer operating modes. */
enum class MagnetometerMode : std::uint8_t {
  disabled, ///< The magnetometer is disabled.
  single_measurement,
  continuous_10_hz,  ///< Continuous mode at 10 Hz.
  continuous_20_hz,  ///< Continuous mode at 20 Hz.
  continuous_50_hz,  ///< Continuous mode at 50 Hz.
  continuous_100_hz, ///< Continuous mode at 100 Hz.
  self_test,         ///< Built-in self-test mode.
};

/**
 * @brief Accelerometer configuration stored by the driver.
 *
 * Accelerometer Configuration. The default values matches the sensor reset state.
 */
struct AccelerometerConfiguration {
  AccelerometerRange range{AccelerometerRange::g_2};                            ///< Current accelerometer measurement range.
  std::uint16_t output_data_rate_hz{default_accelerometer_output_data_rate_hz}; /// Current accelerometer ODR in Hz.
  std::uint16_t filter_frequency_hz{default_accelerometer_filter_frequency_hz}; /// Current accelerometer low-pass filter frequency in Hz.
};

/**
 * @brief Gyroscope configuration stored by the driver.
 *
 * Gyroscope Configuration. The default values matches the sensor reset state.
 */
struct GyroscopeConfiguration {
  GyroscopeRange range{GyroscopeRange::dps_250};                            ///< Current gyroscope measurement range.
  std::uint16_t output_data_rate_hz{default_gyroscope_output_data_rate_hz}; ///< Current gyroscope ODR in Hz.
  std::uint16_t filter_frequency_hz{default_gyroscope_filter_frequency_hz}; /// Current gyroscope low-pass filter frequency in Hz.
};

/**
 * @brief Magnetometer configuration stored by the driver.
 *
 * Magnetometer Configuration. The default values matches the sensor reset state.
 */
struct MagnetometerConfiguration {
  MagnetometerMode mode{MagnetometerMode::disabled}; ///< Current magnetometer operating mode.
};

/**
 * @brief Logical die-temperature sensor configuration.
 *
 * Die-temperature Configuration. The default values matches the sensor reset state.
 */
struct DieTemperatureConfiguration {
  bool enabled{true};                                                             ///< Whether die-temperature reading is logically enabled.
  std::uint16_t filter_frequency_hz{default_die_temperature_filter_frequency_hz}; ///< Current die-temperature low-pass filter frequency in Hz.
};

/**
 * @brief High-level API driver for the ICM-20948 device.
 *
 * The object stores the last known logical configuration of each sensor block.
 * Read operations require a prior @ref connect, and while the device is sleeping,
 * all sensors are treated as unavailable for read operations.
 */
class ICM20948 {
public:
  /**
   * @brief Creates a driver instance from a set of transport callbacks.
   *
   * @invariant No communication is performed.
   *
   * @pre `transport.chip_select`, `transport.read`, and `transport.write` are valid callbacks.
   *
   * @post On success, the returned driver stores the provided transport callbacks.
   * @post On failure, the function returns std::errc::invalid_argument.
   *
   * @param transport Set of callbacks used to communicate with the device.
   *
   * @return A constructed driver or an error code describing an invalid transport.
   */
  static std::expected<ICM20948, std::error_code> create(Transport transport);

  ICM20948(ICM20948 &&other) noexcept;
  ICM20948 &operator=(ICM20948 &&other) noexcept;

  ICM20948(ICM20948 const &) = delete;
  ICM20948 &operator=(ICM20948 const &) = delete;

  /**
   * @brief Establishes a logical connection to the device and synchronizes driver state.
   *
   * The function verifies the `WHO_AM_I` identifier and reads the sleep state.
   *
   * @post On success, the driver is marked as connected and the stored logical configuration is reset to values matching the reset state.
   * @post On failure, the object remains disconnected.
   *
   * @return An empty error code on success or the reason the connection failed.
   */
  std::error_code connect();

  /**
   * @brief Reports whether the driver is logically connected to the device.
   *
   * @invariant No communication is performed.
   *
   * @return true if @ref connect completed successfully and the state was not lost.
   */
  [[nodiscard]] bool is_connected() const noexcept;

  /**
   * @brief Puts the device to sleep and updates the driver's local state.
   *
   * @pre The device is logically connected.
   *
   * @post On success, the device is marked as sleeping.
   * @post On success, sensor reads are logically forbidden until @ref wake is called.
   * @post On success, sensor reads are logically forbidden until @ref wake is called.
   *
   * @return An empty error code on success or the reason the operation failed.
   */
  std::error_code sleep();

  /**
   * @brief Wakes the device up and updates the driver's local state.
   *
   * @pre The device is logically connected.
   *
   * @post On success, the device is marked as awake.
   * @post On success, sensor reads are allowed again according to their logical configuration.
   *
   * @return An empty error code on success or the reason the operation failed.
   */
  std::error_code wake();

  /**
   * @brief Reports whether the device is currently marked as sleeping.
   *
   * @invariant No communication is performed.
   *
   * @return true if the device is currently in the sleeping state.
   */
  [[nodiscard]] bool is_sleeping() const noexcept;

  /**
   * @brief Stores a new logical accelerometer configuration.
   *
   * The function stores the configuration in the object after normalizing input values.
   *
   * @pre The device is logically connected.
   *
   * @post On success, the stored accelerometer configuration is updated with normalized values.
   *
   * @param configuration New logical accelerometer configuration.
   *
   * @return An empty error code on success or the reason the operation was rejected.
   */
  std::error_code configure_accelerometer(AccelerometerConfiguration configuration);

  /**
   * @brief Stores a new logical gyroscope configuration.
   *
   * The function stores the configuration in the object after normalizing input values.
   *
   * @pre The device is logically connected.
   *
   * @post On success, the stored gyroscope configuration is updated with normalized values.
   *
   * @param configuration New logical gyroscope configuration.
   *
   * @return An empty error code on success or the reason the operation was rejected.
   */
  std::error_code configure_gyroscope(GyroscopeConfiguration configuration);

  /**
   * @brief Stores a new logical magnetometer configuration.
   *
   * The function updates only the configuration maintained by the driver.
   *
   * @pre The device is logically connected.
   *
   * @post On success, the stored magnetometer configuration is updated.
   *
   * @param configuration New logical magnetometer configuration.
   *
   * @return An empty error code on success or the reason the operation was rejected.
   */
  std::error_code configure_magnetometer(MagnetometerConfiguration configuration);

  /**
   * @brief Stores a new logical die-temperature configuration.
   *
   * The function updates the local state and performs the required register writes.
   *
   * @pre The device is logically connected.
   *
   * @post On success, the stored die-temperature configuration is updated.
   * @post On success, the die-temperature-related register state is synchronized with the stored configuration.
   *
   * @param configuration New logical die-temperature configuration.
   *
   * @return An empty error code on success or the reason the operation was rejected.
   */
  std::error_code configure_die_temperature(DieTemperatureConfiguration configuration);

  /**
   * @brief Returns the currently stored logical accelerometer configuration.
   *
   * @invariant No communication is performed.
   *
   * @post Returns a reference to the configuration stored inside the object.
   * @post The returned reference remains valid until the object is destroyed or the configuration is overwritten.
   *
   * @return A reference to the stored accelerometer configuration.
   */
  [[nodiscard]] constexpr AccelerometerConfiguration const &accelerometer_configuration() const noexcept;

  /**
   * @brief Returns the currently stored logical gyroscope configuration.
   *
   * @invariant No communication is performed.
   *
   * @post Returns a reference to the configuration stored inside the object.
   * @post The returned reference remains valid until the object is destroyed or the configuration is overwritten.
   *
   * @return A reference to the stored gyroscope configuration.
   */
  [[nodiscard]] constexpr GyroscopeConfiguration const &gyroscope_configuration() const noexcept;

  /**
   * @brief Returns the currently stored logical magnetometer configuration.
   *
   * @invariant No communication is performed.
   *
   * @post Returns a reference to the configuration stored inside the object.
   * @post The returned reference remains valid until the object is destroyed or the configuration is overwritten.
   *
   * @return A reference to the stored magnetometer configuration.
   */
  [[nodiscard]] constexpr MagnetometerConfiguration const &magnetometer_configuration() const noexcept;

  /**
   * @brief Returns the currently stored logical die-temperature configuration.
   *
   * @invariant No communication is performed.
   *
   * @post Returns a reference to the configuration stored inside the object.
   * @post The returned reference remains valid until the object is destroyed or the configuration is overwritten.
   *
   * @return A reference to the stored die-temperature configuration.
   */
  [[nodiscard]] constexpr DieTemperatureConfiguration const &die_temperature_configuration() const noexcept;

  /**
   * @brief Reads the current acceleration from the accelerometer.
   *
   * @pre The device is logically connected.
   * @pre The device is not sleeping.
   * @pre The accelerometer is logically enabled in the stored configuration.
   *
   * @post On success, the returned value is expressed in micro-g.
   *
   * @return The acceleration reading or an error code describing why data is unavailable.
   */
  [[nodiscard]] std::expected<Acceleration, std::error_code> read_acceleration();

  /**
   * @brief Reads the current angular rate from the gyroscope.
   *
   *
   * @pre The device is logically connected.
   * @pre The device is not sleeping.
   * @pre The gyroscope is logically enabled in the stored configuration.
   *
   * @post On success, the returned value is expressed in mdps.
   *
   * @return The angular-rate reading or an error code describing why data is unavailable.
   */
  [[nodiscard]] std::expected<AngularRate, std::error_code> read_angular_rate();

  /**
   * @brief Reads the current magnetic field from the magnetometer.
   *
   *
   * @pre The device is logically connected.
   * @pre The device is not sleeping.
   * @pre The magnetometer is logically enabled in the stored configuration.
   *
   * @post On success, the returned value is expressed in nT.
   *
   * @return The magnetic-field reading or an error code describing why data is unavailable.
   */
  [[nodiscard]] std::expected<MagneticFlux, std::error_code> read_magnetometer();

  /**
   * @brief Reads the current die temperature.
   *
   *
   * @pre The device is logically connected.
   * @pre The device is not sleeping.
   * @pre Die-temperature measurement is logically enabled in the stored configuration.
   *
   * @post On success, the returned value is expressed in milli-Kelvin.
   *
   * @return The die-temperature reading or an error code describing why data is unavailable.
   */
  [[nodiscard]] std::expected<DieTemperature, std::error_code> read_die_temperature();

protected:
  /**
   * @brief Constructs an object without validating the transport.
   *
   * The constructor is intended for internal use by @ref create.
   *
   * @param transport Set of transport callbacks moved into the object.
   */
  explicit ICM20948(Transport transport) noexcept;

  Transport transport{}; ///< Set of transport callbacks.

  bool connected{false}; ///< Logical connection state. Set by @ref connect().

  bool sleeping{true}; ///< Sleep state. Set by @ref sleep() and @ref wake().

  AccelerometerConfiguration accelerometer_config{};    ///< Current accelerometer configuration.
  GyroscopeConfiguration gyroscope_config{};            ///< Current gyroscope configuration.
  MagnetometerConfiguration magnetometer_config{};      ///< Current magnetometer configuration.
  DieTemperatureConfiguration die_temperature_config{}; ///< Current die-temperature configuration.
};

/* ----------------- Inline Implementations ----------------- */

constexpr AccelerometerConfiguration const &ICM20948::accelerometer_configuration() const noexcept { return accelerometer_config; }

constexpr GyroscopeConfiguration const &ICM20948::gyroscope_configuration() const noexcept { return gyroscope_config; }

constexpr MagnetometerConfiguration const &ICM20948::magnetometer_configuration() const noexcept { return magnetometer_config; }

constexpr DieTemperatureConfiguration const &ICM20948::die_temperature_configuration() const noexcept { return die_temperature_config; }

} // namespace icm20948

#endif
