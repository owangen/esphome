import esphome.codegen as cg
from esphome.components import climate, sensor, uart
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC, STATE_CLASS_TOTAL_INCREASING

CODEOWNERS = ["@owangen"]

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["climate", "sensor"]

CONF_CONTROL_CALL_COUNT = "control_call_count"
CONF_SEND_COMMAND_COUNT = "send_command_count"

mill_panelheater_gen2_ns = cg.esphome_ns.namespace("mill_panelheater_gen2")
MillPanelHeaterGen2 = mill_panelheater_gen2_ns.class_(
    "MillPanelHeaterGen2", climate.Climate, cg.Component, uart.UARTDevice
)

DIAGNOSTIC_COUNTER_SCHEMA = sensor.sensor_schema(
    accuracy_decimals=0,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    icon="mdi:counter",
    state_class=STATE_CLASS_TOTAL_INCREASING,
)

CONFIG_SCHEMA = (
    climate.climate_schema(MillPanelHeaterGen2)
    .extend(
        {
            cv.Optional(CONF_CONTROL_CALL_COUNT): DIAGNOSTIC_COUNTER_SCHEMA,
            cv.Optional(CONF_SEND_COMMAND_COUNT): DIAGNOSTIC_COUNTER_SCHEMA,
        }
    )
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

    if control_call_count_config := config.get(CONF_CONTROL_CALL_COUNT):
        control_call_count_sensor = await sensor.new_sensor(control_call_count_config)
        cg.add(var.set_control_call_count_sensor(control_call_count_sensor))

    if send_command_count_config := config.get(CONF_SEND_COMMAND_COUNT):
        send_command_count_sensor = await sensor.new_sensor(send_command_count_config)
        cg.add(var.set_send_command_count_sensor(send_command_count_sensor))
