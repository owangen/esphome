#include "common.h"

namespace esphome::mill_panelheater_gen2::testing {

TEST(MillPanelHeaterGen2Test, SendsPowerOnFrame) {
  MockUARTComponent uart;
  TestableMillPanelHeaterGen2 heater;
  heater.set_uart_parent(&uart);

  heater.send_power_command(0x01);

  const std::vector<uint8_t> expected{
      0x5A, 0x00, 0x10, 0x06, 0x00, 0x47, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5E, 0x5B,
  };
  EXPECT_EQ(uart.tx, expected);
}

TEST(MillPanelHeaterGen2Test, SendsTemperatureSixDegreeFrame) {
  MockUARTComponent uart;
  TestableMillPanelHeaterGen2 heater;
  heater.set_uart_parent(&uart);

  heater.send_temperature_command(0x06);

  const std::vector<uint8_t> expected{
      0x5A, 0x00, 0x10, 0x22, 0x00, 0x46, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x5B,
  };
  EXPECT_EQ(uart.tx, expected);
}

class MillPanelHeaterGen2ReceiveTest : public ::testing::TestWithParam<uint8_t> {};

TEST_P(MillPanelHeaterGen2ReceiveTest, PreservesStatusFrameHandling) {
  MockUARTComponent uart;
  TestableMillPanelHeaterGen2 heater;
  heater.set_uart_parent(&uart);
  heater.target_temperature = 22.0f;
  uart.rx = {
      0x5A, 0x00, 0x11, 0x00, 0x00, 0xC9, 0x00, 0x05, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF4, GetParam(),
  };

  while (uart.available() != 0) {
    heater.loop();
  }

  EXPECT_FLOAT_EQ(heater.target_temperature, 22.0f);
  EXPECT_FLOAT_EQ(heater.current_temperature, 20.0f);
  EXPECT_EQ(heater.mode, climate::CLIMATE_MODE_HEAT);
  EXPECT_EQ(heater.action, climate::CLIMATE_ACTION_IDLE);
}

INSTANTIATE_TEST_SUITE_P(FrameTerminators, MillPanelHeaterGen2ReceiveTest, ::testing::Values<uint8_t>(0x5B, 0x0A));

}  // namespace esphome::mill_panelheater_gen2::testing
