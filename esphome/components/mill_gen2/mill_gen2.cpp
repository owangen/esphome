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
  this->traits_.set_supports_current_temperature(true);
  this->traits_.set_supports_two_point_target_temperature(false);
  this->traits_.set_visual_min_temperature(5);
  this->traits_.set_visual_max_temperature(35);
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
      if (receivedChars[9] == 0x00) {
        this->mode = climate::CLIMATE_MODE_OFF;
        this->action = climate::CLIMATE_ACTION_OFF;
      } else if (receivedChars[9] == 0x01) {
        this->mode = climate::CLIMATE_MODE_HEAT;
      }

      // Parse action
      if (receivedChars[11] == 0x00) {
        this->action = climate::CLIMATE_ACTION_IDLE;
      } else {
        this->action = climate::CLIMATE_ACTION_HEATING;
      }

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

    while (this->available() > 0) {
        rc = this->read();

        if (recvInProgress) {
            if (rc == END_MARKER || rc == LINE_END_MARKER) {
                recvInProgress = false;
                ndx = 0;
                newData = true;
                ESP_LOGD(TAG, "Data received: %s", receivedChars);
            } else {
                if (ndx < BUFFER_SIZE - 1) {
                    receivedChars[ndx++] = rc;
                } else {
                    ESP_LOGW(TAG, "Buffer overflow, resetting.");
                    recvInProgress = false;
                    ndx = 0;
                }
            }
        } else if (rc == START_MARKER) {
            recvInProgress = true;
        }
    }
}

// void MillGen2::recvWithStartEndMarkers() {
//   static bool recvInProgress = false;
//   static uint8_t ndx = 0;
//   char startMarker = 0x5A;
//   char endMarker = 0x5B;
//   char lineEndMarker = 0x0A;
//   char rc;

//   if (this->available() > 0) {
//     rc = this->read();
//     if (recvInProgress) {
//       if ((rc != endMarker) && (rc != lineEndMarker)) {
//         receivedChars[ndx] = (char) rc;
//         ndx++;
//       } else {
//         recvInProgress = false;
//         ndx = 0;
//         newData = true;
//       }
//     } else if (rc == startMarker) {
//       recvInProgress = true;
//     }
//   }
// }

ClimateTraits MillGen2::traits() { return traits_; }

void MillGen2::control(const climate::ClimateCall &call) {
  ESP_LOGD(TAG, "Climate change requested");

  // if (call.get_mode().has_value()) {

  //     switch (call.get_mode().value()) {
  //             case CLIMATE_MODE_OFF:
  //               sendCommand(setPower, sizeof(setPower), 0x00);
  //                 break;
  //             case CLIMATE_MODE_HEAT:
  //               sendCommand(setPower, sizeof(setPower), 0x01);
  //                 break;
  //             default:
  //                 break;
  //     }

  //   ClimateMode mode = *call.get_mode();

  //   this->mode = mode;
  //   this->publish_state();
  //     }

  // if (call.get_target_temperature().has_value()) {
  //   // User requested target temperature change
  //   int temp = *call.get_target_temperature();
  //   sendCommand(setTemp, sizeof(setTemp), temp);
  //   // ...
  //   this->target_temperature = temp;
  //   this->publish_state();

  // }
}

/* Seriedata ut til mill mikrokontroller ---*/
// void MillGen2::sendCommand(char* arrayen, int len, int commando) {
//   ESP_LOGD(TAG, "Sending serial command");
//   if (arrayen[4] == 0x46) { // Temperatur (OLD  0x43)
//     arrayen[7] = commando;
//   }
//   if (arrayen[4] == 0x47) { // Power av/på
//     arrayen[5] = commando;
//     arrayen[len] = (char)0x00;  // Padding..
//   }
//   char crc = calculateChecksum(arrayen, len + 1);
//   ESP_LOGD(TAG, "writing start byte");
//   Serial.write((char)0x5A); // Startbyte
//   for (int i = 0; i < len + 1; i++) { // Beskjed
//     Serial.write((char)arrayen[i]);
//   }
//   Serial.write((char)crc); // Kontrollbyte
//   Serial.write((char)0x5B); // Stoppbyte
// }

}  // namespace mill_gen2
}  // namespace esphome
