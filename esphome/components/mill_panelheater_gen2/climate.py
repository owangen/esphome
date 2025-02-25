import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, climate
from esphome.const import (
  CONF_ID,
  CONF_MAX_TEMPERATURE,
  CONF_MIN_TEMPERATURE,
  CONF_SUPPORTED_MODES,
)

from esphome.components.climate import (
    ClimateMode,
    CONF_CURRENT_TEMPERATURE,
)

CODEOWNERS = ["@owangen"]

DEPENDENCIES = ["climate", "uart"]

mill_panelheater_gen2_ns = cg.esphome_ns.namespace("mill_panelheater_gen2")
MillHeater = mill_panelheater_gen2_ns.class_("MillPanelHeaterGen2", uart.UARTDevice, climate.Climate, cg.Component)

CONF_MILL_ID = "mill_id"

CONFIG_SCHEMA = cv.All(
  climate.CLIMATE_SCHEMA.extend(
    {
      cv.GenerateID(): cv.declare_id(MillHeater),
    }
  )
  .extend(uart.UART_DEVICE_SCHEMA)
  .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await uart.register_uart_device(var, config)
  await climate.register_climate(var, config)
