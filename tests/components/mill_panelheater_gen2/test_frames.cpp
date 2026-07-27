#include "common.h"

#include "esphome/core/hal.h"

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

TEST(MillPanelHeaterGen2Test, UpdatesStateFromStatusFrame) {
  MockUARTComponent uart;
  TestableMillPanelHeaterGen2 heater;
  heater.set_uart_parent(&uart);
  heater.target_temperature = 22.0f;
  uart.rx = {
      0x5A, 0x00, 0x11, 0x00, 0x00, 0xC9, 0x00, 0x05, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF4, 0x5B,
  };

  while (uart.available() != 0) {
    heater.loop();
  }

  EXPECT_FLOAT_EQ(heater.target_temperature, 5.0f);
  EXPECT_FLOAT_EQ(heater.current_temperature, 20.0f);
  EXPECT_EQ(heater.mode, climate::CLIMATE_MODE_HEAT);
  EXPECT_EQ(heater.action, climate::CLIMATE_ACTION_IDLE);
}

TEST(MillPanelHeaterGen2Test, ParsesTenDegreeTargetAsData) {
  MockUARTComponent uart;
  TestableMillPanelHeaterGen2 heater;
  heater.set_uart_parent(&uart);
  heater.target_temperature = 22.0f;
  uart.rx = {
      0x5A, 0x00, 0x11, 0x00, 0x00, 0xC9, 0x00, 0x0A, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF9, 0x5B,
  };

  while (uart.available() != 0) {
    heater.loop();
  }

  EXPECT_FLOAT_EQ(heater.target_temperature, 10.0f);
  EXPECT_FLOAT_EQ(heater.current_temperature, 20.0f);
  EXPECT_EQ(heater.mode, climate::CLIMATE_MODE_HEAT);
  EXPECT_EQ(heater.action, climate::CLIMATE_ACTION_IDLE);
}

TEST(MillPanelHeaterGen2Test, RejectsInvalidChecksum) {
  MockUARTComponent uart;
  TestableMillPanelHeaterGen2 heater;
  heater.set_uart_parent(&uart);
  heater.target_temperature = 22.0f;
  heater.current_temperature = 21.0f;
  uart.rx = {
      0x5A, 0x00, 0x11, 0x00, 0x00, 0xC9, 0x00, 0x05, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF5, 0x5B,
  };

  while (uart.available() != 0) {
    heater.loop();
  }

  EXPECT_FLOAT_EQ(heater.target_temperature, 22.0f);
  EXPECT_FLOAT_EQ(heater.current_temperature, 21.0f);
}

TEST(MillPanelHeaterGen2Test, RejectsLineFeedAsFrameTerminator) {
  MockUARTComponent uart;
  TestableMillPanelHeaterGen2 heater;
  heater.set_uart_parent(&uart);
  heater.target_temperature = 22.0f;
  heater.current_temperature = 21.0f;
  uart.rx = {
      0x5A, 0x00, 0x11, 0x00, 0x00, 0xC9, 0x00, 0x05, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF4, 0x0A,
  };

  while (uart.available() != 0) {
    heater.loop();
  }

  EXPECT_FLOAT_EQ(heater.target_temperature, 22.0f);
  EXPECT_FLOAT_EQ(heater.current_temperature, 21.0f);
}

TEST(MillPanelHeaterGen2Test, RecoversFromIncompleteWifiButtonSequence) {
  MockUARTComponent uart;
  TestableMillPanelHeaterGen2 heater;
  heater.set_uart_parent(&uart);
  heater.target_temperature = 22.0f;
  heater.current_temperature = 21.0f;
  uart.rx = {0x5A, 0x00, 0x0A};

  while (uart.available() != 0) {
    heater.loop();
  }

  delay(101);
  const std::vector<uint8_t> status_frame{
      0x5A, 0x00, 0x11, 0x00, 0x00, 0xC9, 0x00, 0x05, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF4, 0x5B,
  };
  uart.rx.insert(uart.rx.end(), status_frame.begin(), status_frame.end());

  while (uart.available() != 0) {
    heater.loop();
  }

  EXPECT_FLOAT_EQ(heater.target_temperature, 5.0f);
  EXPECT_FLOAT_EQ(heater.current_temperature, 20.0f);
}

TEST(MillPanelHeaterGen2Test, KeepsOffModeAndActionConsistent) {
  MockUARTComponent uart;
  TestableMillPanelHeaterGen2 heater;
  heater.set_uart_parent(&uart);
  heater.mode = climate::CLIMATE_MODE_HEAT;
  heater.action = climate::CLIMATE_ACTION_HEATING;
  uart.rx = {
      0x5A, 0x00, 0x11, 0x00, 0x00, 0xC9, 0x00, 0x05, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF3, 0x5B,
  };

  while (uart.available() != 0) {
    heater.loop();
  }

  EXPECT_EQ(heater.mode, climate::CLIMATE_MODE_OFF);
  EXPECT_EQ(heater.action, climate::CLIMATE_ACTION_OFF);
}

}  // namespace esphome::mill_panelheater_gen2::testing
