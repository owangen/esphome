#include "mill_gen2.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mill_gen2 {

static const char *TAG = "MillGen2";

MillGen2::~MillGen2() {}

void MillGen2::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MillGen2...");
}

void MillGen2::loop() {
  this->receiveSerialData();
  if (this->newData) {
    this->newData = false;
    this->processReceivedData();
  }
}

climate::ClimateTraits MillGen2::traits() {
  climate::ClimateTraits traits;
  traits.set_supports_current_temperature(true);
  traits.set_supports_action(true);
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT});
  traits.set_visual_min_temperature(5);
  traits.set_visual_max_temperature(30);
  traits.set_visual_temperature_step(1);
  return traits;
}

void MillGen2::control(const climate::ClimateCall &call) {
  ESP_LOGD(TAG, "Control called.");
  if (call.get_mode().has_value()) {
    this->handleModeChange(*call.get_mode());
    this->mode = *call.get_mode();
    this->publish_state();
  }
  if (call.get_target_temperature().has_value()) {
    this->sendTemperatureCommand(*call.get_target_temperature());
    this->target_temperature = *call.get_target_temperature();
    this->publish_state();
  }
}

// Implement the rest of the methods (receiveSerialData, processReceivedData, etc.)...

}  // namespace mill_gen2
}  // namespace esphome
