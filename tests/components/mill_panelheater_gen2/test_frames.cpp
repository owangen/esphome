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

}  // namespace esphome::mill_panelheater_gen2::testing
