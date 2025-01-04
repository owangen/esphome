#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/log.h"
#include "mill_gen2.h"

using namespace esphome::climate;
using namespace esphome::uart;

namespace esphome {
namespace mill_gen2 {

static const char *TAG = "millgen2.climate";

char receivedChars[15];
bool newData;
// mill commandos
char setPower[] = {0x00, 0x10, 0x06, 0x00, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // Powertoggle er pos 5
char setTemp[] = {0x00, 0x10, 0x22, 0x00, 0x46, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00};

MillGen2::MillGen2() {
  this->traits_ = climate::ClimateTraits();
  this->traits_.set_visual_target_temperature_step(1);
  this->traits_.set_visual_current_temperature_step(1);
  this->traits_.set_visual_min_temperature(5);
  this->traits_.set_visual_max_temperature(35);
  this->traits_.set_supports_current_temperature(true);
  this->traits_.set_supports_two_point_target_temperature(false);
  this->traits_.set_supports_action(true);
  this->traits_.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT,
  });
}

MillGen2::~MillGen2() {}

void MillGen2::setup() { ESP_LOGI(TAG, "MillGen2 initialization..."); }

void MillGen2::dump_config() {
  ESP_LOGCONFIG(TAG, "MillGen2:");
  LOG_CLIMATE("", "MillGen2 Climate", this);
  this->check_uart_settings(9600);
}

void MillGen2::loop() {
  recvWithStartEndMarkers();

  if (newData == true) {
    newData = false;
    if (receivedChars[4] == 0xC9) {  // Filter out unnecessary information
      // Parse target temperature
      if (receivedChars[6] != 0) {
        this->target_temperature = receivedChars[6];
      }
      // Parse current temperature
      if (receivedChars[7] != 0) {
        this->current_temperature = receivedChars[7];
      }
      // Parse climate mode
      // TODO bruke TARGET_TEMP_POS istede
      if (receivedChars[9] == 0x00) {
        this->mode = climate::CLIMATE_MODE_OFF;
        this->action = climate::CLIMATE_ACTION_OFF;
      } else if (receivedChars[9] == 0x01) {
        this->mode = climate::CLIMATE_MODE_HEAT;
      }

      // Parse action
      this->action = (receivedChars[11] == 0x00) ? climate::CLIMATE_ACTION_IDLE : climate::CLIMATE_ACTION_HEATING;

      this->publish_state();
    }
  }
}

void MillGen2::recvWithStartEndMarkers() {
  static bool recvInProgress = false;
  static uint8_t ndx = 0;
  const char START_MARKER = 0x5A;
  const char END_MARKER = 0x5B;
  const char LINE_END_MARKER = 0x0A;
  char rc;

  if (this->available() > 0) {
    rc = this->read();
    if (recvInProgress) {
      if ((rc != END_MARKER) && (rc != LINE_END_MARKER)) {
        receivedChars[ndx] = (char) rc;
        ndx++;
      } else {
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    } else if (rc == START_MARKER) {
      recvInProgress = true;
    }
  }
}

ClimateTraits MillGen2::traits() { return traits_; }

void MillGen2::control(const climate::ClimateCall &call) {
  ESP_LOGD(TAG, "Climate change requested");

  if (call.get_mode().has_value()) {
    switch (call.get_mode().value()) {
      case CLIMATE_MODE_OFF:
        sendCommand(setPower, sizeof(setPower), 0x00);
        break;
      case CLIMATE_MODE_HEAT:
        sendCommand(setPower, sizeof(setPower), 0x01);
        break;
      default:
        break;
    }

    ClimateMode mode = *call.get_mode();
    this->mode = mode;
    this->publish_state();
  }

  if (call.get_target_temperature().has_value()) {
    // User requested target temperature change
    int temp = *call.get_target_temperature();
    sendCommand(setTemp, sizeof(setTemp), temp);
    this->target_temperature = temp;
    this->publish_state();
  }
}

/* Send serial data to the microcontroller */
void MillGen2::sendCommand(char *commandArray, int len, int command) {
  ESP_LOGD(TAG, "Sending serial command");
  if (commandArray[4] == 0x46) {  // Temperature
    commandArray[7] = command;
  }
  if (commandArray[4] == 0x47) {  // Power on/off
    commandArray[5] = command;
    commandArray[len] = (char) 0x00;  // Padding
  }
  char crc = checksum(commandArray, len + 1);
  ESP_LOGD(TAG, "writing start byte");
  write((char) 0x5A);                  // Start byte
  for (int i = 0; i < len + 1; i++) {  // Message
    write((char) commandArray[i]);
  }
  write((char) crc);   // Control byte
  write((char) 0x5B);  // Stop byte
}

/*--- Function for calculating control byte checksum ---*/
unsigned char MillGen2::checksum(char *buf, int len) {
  unsigned char chk = 0;
  for (; len != 0; len--) {
    chk += *buf++;
  }
  return chk;
}

}  // namespace mill_gen2
}  // namespace esphome
