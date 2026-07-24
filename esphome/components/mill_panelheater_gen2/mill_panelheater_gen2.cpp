#include "mill_panelheater_gen2.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include "esphome/core/log.h"

namespace esphome::mill_panelheater_gen2 {

static const char *const TAG = "mill_panelheater_gen2.climate";

static constexpr std::array<uint8_t, 13> POWER_COMMAND{
    0x00, 0x10, 0x06, 0x00, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static constexpr std::array<uint8_t, 13> TEMPERATURE_COMMAND{
    0x00, 0x10, 0x22, 0x00, 0x46, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void MillPanelHeaterGen2::setup() { ESP_LOGI(TAG, "MillPanelHeaterGen2 initialization..."); }

void MillPanelHeaterGen2::dump_config() {
  ESP_LOGCONFIG(TAG, "MillPanelHeaterGen2:");
  LOG_CLIMATE("", "MillPanelHeaterGen2 Climate", this);
  this->check_uart_settings(9600);
}

void MillPanelHeaterGen2::loop() {
  this->receive_byte_();

  if (!this->new_data_) {
    return;
  }
  this->new_data_ = false;

  if (this->received_length_ <= ACTION_POS) {
    ESP_LOGW(TAG, "Received frame is too short: %u bytes", static_cast<unsigned>(this->received_length_));
    return;
  }

  if (this->received_data_[COMMAND_TYPE_POS] != STATUS_COMMAND_TYPE) {
    return;
  }

  if (this->received_data_[TARGET_TEMP_POS] != 0) {
    this->target_temperature = this->received_data_[TARGET_TEMP_POS];
  }
  if (this->received_data_[CURRENT_TEMP_POS] != 0) {
    this->current_temperature = this->received_data_[CURRENT_TEMP_POS];
  }

  if (this->received_data_[MODE_POS] == 0x00) {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
  } else if (this->received_data_[MODE_POS] == 0x01) {
    this->mode = climate::CLIMATE_MODE_HEAT;
  }

  this->action =
      this->received_data_[ACTION_POS] == 0x00 ? climate::CLIMATE_ACTION_IDLE : climate::CLIMATE_ACTION_HEATING;
  this->publish_state();
}

void MillPanelHeaterGen2::receive_byte_() {
  if (this->available() == 0) {
    return;
  }

  uint8_t byte;
  if (!this->read_byte(&byte)) {
    return;
  }

  if (!this->receive_in_progress_) {
    if (byte == START_MARKER) {
      this->receive_in_progress_ = true;
      this->received_length_ = 0;
    }
    return;
  }

  if (byte == END_MARKER || byte == LINE_END_MARKER) {
    this->receive_in_progress_ = false;
    this->new_data_ = true;
    return;
  }

  if (this->received_length_ >= this->received_data_.size()) {
    ESP_LOGW(TAG, "Received frame exceeds buffer size; discarding it");
    this->receive_in_progress_ = false;
    this->received_length_ = 0;
    this->new_data_ = false;
    return;
  }

  this->received_data_[this->received_length_++] = byte;
}

climate::ClimateTraits MillPanelHeaterGen2::traits() {
  climate::ClimateTraits traits;
  traits.set_visual_target_temperature_step(1);
  traits.set_visual_current_temperature_step(1);
  traits.set_visual_min_temperature(5);
  traits.set_visual_max_temperature(35);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE | climate::CLIMATE_SUPPORTS_ACTION);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT,
  });
  return traits;
}

void MillPanelHeaterGen2::control(const climate::ClimateCall &call) {
  ESP_LOGD(TAG, "Climate change requested");

  if (const auto mode = call.get_mode()) {
    switch (*mode) {
      case climate::CLIMATE_MODE_OFF:
        this->send_power_command_(0x00);
        break;
      case climate::CLIMATE_MODE_HEAT:
        this->send_power_command_(0x01);
        break;
      default:
        break;
    }

    this->mode = *mode;
    this->publish_state();
  }

  if (const auto target_temperature = call.get_target_temperature()) {
    const auto temperature = static_cast<uint8_t>(*target_temperature);
    this->send_temperature_command_(temperature);
    this->target_temperature = temperature;
    this->publish_state();
  }
}

void MillPanelHeaterGen2::send_power_command_(uint8_t command) { this->send_command_(POWER_COMMAND, 5, command); }

void MillPanelHeaterGen2::send_temperature_command_(uint8_t command) {
  this->send_command_(TEMPERATURE_COMMAND, 7, command);
}

void MillPanelHeaterGen2::send_command_(std::array<uint8_t, COMMAND_PAYLOAD_SIZE> payload, size_t command_position,
                                        uint8_t command) {
  ESP_LOGD(TAG, "Sending serial command");
  payload[command_position] = command;

  // The original implementation sent 13 payload bytes and attempted to set byte 12 to zero for power commands.
  // This padding byte is retained conservatively, but is not confirmed by manufacturer documentation or UART capture.
  payload[12] = 0x00;

  std::array<uint8_t, COMMAND_PAYLOAD_SIZE + 3> frame{};
  frame[0] = START_MARKER;
  for (size_t i = 0; i < payload.size(); i++) {
    frame[i + 1] = payload[i];
  }
  frame[COMMAND_PAYLOAD_SIZE + 1] = checksum_(payload.data(), payload.size());
  frame[COMMAND_PAYLOAD_SIZE + 2] = END_MARKER;
  this->write_array(frame);
}

uint8_t MillPanelHeaterGen2::checksum_(const uint8_t *data, size_t length) {
  uint8_t checksum = 0;
  for (size_t i = 0; i < length; i++) {
    checksum += data[i];
  }
  return checksum;
}

}  // namespace esphome::mill_panelheater_gen2
