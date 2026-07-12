import esphome.codegen as cg
from esphome.components import climate_ir

AUTO_LOAD = ["climate_ir"]

vizo_ac_ns = cg.esphome_ns.namespace("vizo_ac")
VizoACClimate = vizo_ac_ns.class_("VizoACClimate", climate_ir.ClimateIR)

CONFIG_SCHEMA = climate_ir.climate_ir_schema(VizoACClimate)


async def to_code(config):
    var = await climate_ir.new_climate_ir(config)
    cg.add(var.set_supports_cool(True))
    cg.add(var.set_supports_heat(False))
