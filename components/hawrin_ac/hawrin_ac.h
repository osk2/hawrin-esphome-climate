#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome::hawrin_ac {

class HawrinACClimate : public climate_ir::ClimateIR {
 public:
  HawrinACClimate()
      : climate_ir::ClimateIR(
            20, 30, 1.0f, false, false,
            {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
             climate::CLIMATE_FAN_HIGH},
            {}, {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ECO}) {}

  void control(const climate::ClimateCall &call) override;
  void send_display_toggle();

 protected:
  void transmit_state() override;
  void send_power_toggle_();
  void send_eco_(bool enabled);
  void send_frame_(uint8_t byte2, uint8_t byte3, uint8_t byte6, uint8_t byte7, const uint8_t *frame2);
  static uint8_t temp_byte_(float temp);
  static uint8_t fan_byte_(optional<climate::ClimateFanMode> fan);
  static uint8_t seq_for_temp_(float temp);
};

}  // namespace esphome::hawrin_ac
