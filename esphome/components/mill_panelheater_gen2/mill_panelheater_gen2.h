#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome::mill_panelheater_gen2 {

class MillPanelHeaterGen2 : public climate::Climate, public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void control(const climate::ClimateCall &call) override;
  void dump_config() override;

  void set_power_sensor(sensor::Sensor *power_sensor) { this->power_sensor_ = power_sensor; }
  void set_rated_power(float rated_power) { this->rated_power_ = rated_power; }

 protected:
  climate::ClimateTraits traits() override;

  void send_power_command_(uint8_t command);
  void send_temperature_command_(uint8_t command);

 private:
  void receive_byte_();
  void log_frame_(const char *message, uint8_t final_byte) const;
  void publish_power_state_();
  void reset_communication_timeout_();
  void send_command_(std::array<uint8_t, 13> payload, size_t command_position, uint8_t command);
  static uint8_t checksum_(const uint8_t *data, size_t length);

  static constexpr size_t RECEIVE_BUFFER_SIZE = 15;
  static constexpr size_t COMMAND_PAYLOAD_SIZE = 13;

  static constexpr size_t COMMAND_TYPE_POS = 4;
  static constexpr size_t TARGET_TEMP_POS = 6;
  static constexpr size_t CURRENT_TEMP_POS = 7;
  static constexpr size_t MODE_POS = 9;
  static constexpr size_t ACTION_POS = 11;

  static constexpr uint8_t START_MARKER = 0x5A;
  static constexpr uint8_t END_MARKER = 0x5B;
  static constexpr uint8_t LINE_END_MARKER = 0x0A;
  static constexpr uint8_t STATUS_COMMAND_TYPE = 0xC9;

  static constexpr uint32_t COMMUNICATION_TIMEOUT = 60000;

  std::array<uint8_t, RECEIVE_BUFFER_SIZE> received_data_{};
  size_t received_length_{0};
  bool receive_in_progress_{false};
  bool new_data_{false};

  sensor::Sensor *power_sensor_{nullptr};
  float rated_power_{0.0f};
};

}  // namespace esphome::mill_panelheater_gen2
