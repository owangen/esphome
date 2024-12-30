#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace mill_gen2 {

class MillGen2 :  public esphome::Component,
                  public esphome::climate::Climate,
                  public esphome::uart::UARTDevice {

public:
  MillGen2();
  ~MillGen2();
  void setup() override;
  void loop() override;
  void control(const climate::ClimateCall &call) override;
  void dump_config() override;

protected:
  esphome::climate::ClimateTraits traits() override;

private:
  void receiveSerialData();
  void sendCommand(char* arrayen, int len, int commando);
  unsigned char calculateChecksum(char *buffer, size_t length);

  static constexpr size_t BUFFER_SIZE = 15;
  char receivedChars[BUFFER_SIZE];
  bool newData = false;

  esphome::climate::ClimateTraits traits_;

  static constexpr size_t COMMAND_TYPE_POS = 4;
  static constexpr size_t TARGET_TEMP_POS = 6;
  static constexpr size_t CURRENT_TEMP_POS = 7;
  static constexpr size_t MODE_POS = 9;
  static constexpr size_t ACTION_POS = 11;

  char powerCommand[12] = {0x00, 0x10, 0x06, 0x00, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  char temperatureCommand[12] = {0x00, 0x10, 0x22, 0x00, 0x46, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00};
};

}  // namespace mill_gen2
}  // namespace esphome
