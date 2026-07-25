import esphome.codegen as cg
from esphome.components import climate, uart
import esphome.config_validation as cv

CODEOWNERS = ["@owangen"]

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["climate"]

mill_panelheater_gen2_ns = cg.esphome_ns.namespace("mill_panelheater_gen2")
MillPanelHeaterGen2 = mill_panelheater_gen2_ns.class_(
    "MillPanelHeaterGen2", climate.Climate, cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = (
    climate.climate_schema(MillPanelHeaterGen2)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "mill_panelheater_gen2",
    baud_rate=9600,
    require_rx=True,
    require_tx=True,
    data_bits=8,
    parity="NONE",
    stop_bits=1,
)


async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
