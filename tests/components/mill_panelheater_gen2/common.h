#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "esphome/components/mill_panelheater_gen2/mill_panelheater_gen2.h"
#include "esphome/components/uart/uart_component.h"

namespace esphome::mill_panelheater_gen2::testing {

class MockUARTComponent : public uart::UARTComponent {
 public:
  std::vector<uint8_t> tx;

  void write_array(const uint8_t *data, size_t length) override {
    this->tx.insert(this->tx.end(), data, data + length);
  }

  bool read_array(uint8_t *data, size_t length) override { return false; }
  size_t available() override { return 0; }

  MOCK_METHOD(bool, peek_byte, (uint8_t * data), (override));
  MOCK_METHOD(uart::UARTFlushResult, flush, (), (override));
  MOCK_METHOD(void, check_logger_conflict, (), (override));
#if defined(USE_ESP8266) || defined(USE_ESP32)
  void load_settings(bool dump_config) override {}
#endif
};

class TestableMillPanelHeaterGen2 : public MillPanelHeaterGen2 {
 public:
  void send_power_command(uint8_t command) { this->send_power_command_(command); }
  void send_temperature_command(uint8_t command) { this->send_temperature_command_(command); }
};

}  // namespace esphome::mill_panelheater_gen2::testing
