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

mill_gen2_ns = cg.esphome_ns.namespace("mill_gen2")
MillGen2 = mill_gen2_ns.class_("MillGen2", uart.UARTDevice, climate.Climate, cg.Component)

CONF_MILL_ID = "mill_id"
CONF_CLIMATE_ID = "climate_id"

SUPPORTED_CLIMATE_MODES_OPTIONS = {
    "OFF": ClimateMode.CLIMATE_MODE_OFF,  # always available
    "HEAT": ClimateMode.CLIMATE_MODE_HEAT,
}

CONFIG_SCHEMA = (
  climate.CLIMATE_SCHEMA.extend(
    {
      cv.GenerateID(): cv.declare_id(MillGen2),
      cv.GenerateID(CONF_CLIMATE_ID): cv.use_id(climate.Climate),
      cv.Optional(CONF_SUPPORTED_MODES): cv.ensure_list(
                cv.enum(SUPPORTED_CLIMATE_MODES_OPTIONS, upper=True)
            ),
    }
  )
  .extend(uart.UART_DEVICE_SCHEMA)
  .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await uart.register_uart_device(var, config)
  clim = await cg.get_variable(config[CONF_CLIMATE_ID])
  cg.add(var.set_climate(clim))