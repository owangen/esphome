#include "mill_panelheater_gen2.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include "esphome/core/log.h"

namespace esphome::mill_panelheater_gen2 {

static const char *const TAG = "mill_panelheater_gen2.climate";
static const char *const DIAGNOSTIC_BUILD_ID = "mill-gen2-diagnostics-20260725-a";

static constexpr std::array<uint8_t, 13> POWER_COMMAND{
    0x00, 0x10, 0x06, 0x00, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static constexpr std::array<uint8_t, 13> TEMPERATURE_COMMAND{
    0x00, 0x10, 0x22, 0x00, 0x46, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void MillPanelHeaterGen2::setup() {
  ESP_LOGI(TAG, "MillPanelHeaterGen2 initialization...");
  this->publish_diagnostic_counters_();
  ESP_LOGE(TAG, "DIAG build=%s event=setup control_count=%u send_count=%u", DIAGNOSTIC_BUILD_ID,
           static_cast<unsigned>(this->control_call_count_), static_cast<unsigned>(this->send_command_count_));
}

void MillPanelHeaterGen2::dump_config() {
  ESP_LOGCONFIG(TAG, "MillPanelHeaterGen2:");
  LOG_CLIMATE("", "MillPanelHeaterGen2 Climate", this);
  LOG_SENSOR("  ", "Control Call Count", this->control_call_count_sensor_);
  LOG_SENSOR("  ", "Send Command Count", this->send_command_count_sensor_);
  this->check_uart_settings(9600);
}

void MillPanelHeaterGen2::loop() {
  this->receive_byte_();

  if (!this->new_data_) {
    return;
  }
  this->new_data_ = false;

  if (this->received_length_ <= ACTION_POS) {
    ESP_LOGW(TAG, "Rejecting frame: payload is too short (%u bytes; need at least %u bytes through ACTION_POS)",
             static_cast<unsigned>(this->received_length_), static_cast<unsigned>(ACTION_POS + 1));
    return;
  }

  if (this->received_data_[COMMAND_TYPE_POS] != STATUS_COMMAND_TYPE) {
    ESP_LOGD(TAG, "Ignoring frame: type 0x%02X is not status type 0x%02X", this->received_data_[COMMAND_TYPE_POS],
             STATUS_COMMAND_TYPE);
    return;
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
  ESP_LOGD(TAG,
           "C9 fields: UNKNOWN_POS=0x%02X (%u), CURRENT_TEMP_POS=0x%02X (%u), MODE_POS=0x%02X, "
           "ACTION_POS=0x%02X; result: target_temperature=%.1f, current_temperature=%.1f, mode=%s, action=%s",
           this->received_data_[UNKNOWN_POS], this->received_data_[UNKNOWN_POS], this->received_data_[CURRENT_TEMP_POS],
           this->received_data_[CURRENT_TEMP_POS], this->received_data_[MODE_POS], this->received_data_[ACTION_POS],
           this->target_temperature, this->current_temperature,
           LOG_STR_ARG(climate::climate_mode_to_string(this->mode)),
           LOG_STR_ARG(climate::climate_action_to_string(this->action)));
  ESP_LOGD(
      TAG, "publish_state() [C9 status frame]: target_temperature=%.1f, current_temperature=%.1f, mode=%s, action=%s",
      this->target_temperature, this->current_temperature, LOG_STR_ARG(climate::climate_mode_to_string(this->mode)),
      LOG_STR_ARG(climate::climate_action_to_string(this->action)));
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

  ESP_LOGVV(TAG, "RX byte: byte=0x%02X, receive_in_progress=%s, buffer_length=%u", byte,
            YESNO(this->receive_in_progress_), static_cast<unsigned>(this->received_length_));

  if (!this->receive_in_progress_) {
    if (byte == START_MARKER) {
      this->receive_in_progress_ = true;
      this->received_length_ = 0;
    }
    return;
  }

  if (byte == END_MARKER || byte == LINE_END_MARKER) {
    this->log_frame_("Received complete frame", byte);
    this->receive_in_progress_ = false;
    this->new_data_ = true;
    return;
  }

  if (this->received_length_ >= this->received_data_.size()) {
    this->log_frame_("Rejecting overlong frame", byte);
    ESP_LOGW(TAG, "Rejecting frame: payload exceeds %u-byte receive buffer; overflow byte is 0x%02X",
             static_cast<unsigned>(this->received_data_.size()), byte);
    this->receive_in_progress_ = false;
    this->received_length_ = 0;
    this->new_data_ = false;
    return;
  }

  this->received_data_[this->received_length_++] = byte;
}

void MillPanelHeaterGen2::log_frame_(const char *message, uint8_t final_byte) const {
  std::array<uint8_t, RECEIVE_BUFFER_SIZE + 2> frame{};
  size_t frame_length = 0;
  frame[frame_length++] = START_MARKER;
  for (size_t i = 0; i < this->received_length_; i++) {
    frame[frame_length++] = this->received_data_[i];
  }
  frame[frame_length++] = final_byte;

  if (this->received_length_ > COMMAND_TYPE_POS) {
    ESP_LOGD(TAG, "%s: bytes=%s, length=%u, payload_length=%u, type=0x%02X, final_byte=0x%02X", message,
             format_hex_pretty(frame.data(), frame_length).c_str(), static_cast<unsigned>(frame_length),
             static_cast<unsigned>(this->received_length_), this->received_data_[COMMAND_TYPE_POS], final_byte);
  } else {
    ESP_LOGD(TAG, "%s: bytes=%s, length=%u, payload_length=%u, type=unavailable, final_byte=0x%02X", message,
             format_hex_pretty(frame.data(), frame_length).c_str(), static_cast<unsigned>(frame_length),
             static_cast<unsigned>(this->received_length_), final_byte);
  }
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
  this->control_call_count_++;
  this->publish_diagnostic_counters_();
  ESP_LOGE(TAG, "DIAG build=%s event=control control_count=%u send_count=%u", DIAGNOSTIC_BUILD_ID,
           static_cast<unsigned>(this->control_call_count_), static_cast<unsigned>(this->send_command_count_));

  const auto requested_mode = call.get_mode();
  const auto requested_target_temperature = call.get_target_temperature();
  ESP_LOGD(TAG, "control() called: mode_set=%s, target_temperature_set=%s", YESNO(requested_mode.has_value()),
           YESNO(requested_target_temperature.has_value()));
  if (requested_mode.has_value()) {
    ESP_LOGD(TAG, "control() requested mode=%s", LOG_STR_ARG(climate::climate_mode_to_string(*requested_mode)));
  }
  if (requested_target_temperature.has_value()) {
    ESP_LOGD(TAG, "control() requested target_temperature=%.1f", *requested_target_temperature);
  }

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
    ESP_LOGD(
        TAG,
        "publish_state() [control mode request]: target_temperature=%.1f, current_temperature=%.1f, mode=%s, action=%s",
        this->target_temperature, this->current_temperature, LOG_STR_ARG(climate::climate_mode_to_string(this->mode)),
        LOG_STR_ARG(climate::climate_action_to_string(this->action)));
    this->publish_state();
  }

  if (const auto target_temperature = call.get_target_temperature()) {
    const auto temperature = static_cast<uint8_t>(*target_temperature);
    this->send_temperature_command_(temperature);
    ESP_LOGD(TAG, "target_temperature update [control target temperature request]: old=%.1f, new=%.1f",
             this->target_temperature, static_cast<float>(temperature));
    this->target_temperature = temperature;
    ESP_LOGD(TAG,
             "publish_state() [control target temperature request]: target_temperature=%.1f, "
             "current_temperature=%.1f, mode=%s, action=%s",
             this->target_temperature, this->current_temperature,
             LOG_STR_ARG(climate::climate_mode_to_string(this->mode)),
             LOG_STR_ARG(climate::climate_action_to_string(this->action)));
    this->publish_state();
  }
}

void MillPanelHeaterGen2::send_power_command_(uint8_t command) { this->send_command_(POWER_COMMAND, 5, command); }

void MillPanelHeaterGen2::send_temperature_command_(uint8_t command) {
  this->send_command_(TEMPERATURE_COMMAND, 7, command);
}

void MillPanelHeaterGen2::send_command_(std::array<uint8_t, COMMAND_PAYLOAD_SIZE> payload, size_t command_position,
                                        uint8_t command) {
  this->send_command_count_++;
  this->publish_diagnostic_counters_();
  ESP_LOGE(TAG, "DIAG build=%s event=send_command control_count=%u send_count=%u", DIAGNOSTIC_BUILD_ID,
           static_cast<unsigned>(this->control_call_count_), static_cast<unsigned>(this->send_command_count_));
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

void MillPanelHeaterGen2::publish_diagnostic_counters_() {
  if (this->control_call_count_sensor_ != nullptr) {
    this->control_call_count_sensor_->publish_state(static_cast<float>(this->control_call_count_));
  }
  if (this->send_command_count_sensor_ != nullptr) {
    this->send_command_count_sensor_->publish_state(static_cast<float>(this->send_command_count_));
  }
}

uint8_t MillPanelHeaterGen2::checksum_(const uint8_t *data, size_t length) {
  uint8_t checksum = 0;
  for (size_t i = 0; i < length; i++) {
    checksum += data[i];
  }
  return checksum;
}

}  // namespace esphome::mill_panelheater_gen2
