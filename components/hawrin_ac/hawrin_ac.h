#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome::hawrin_ac {

class HawrinACClimate : public climate_ir::ClimateIR {
 public:
  HawrinACClimate()
      : climate_ir::ClimateIR(
            20, 30, 1.0f, true, true,
            {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
             climate::CLIMATE_FAN_HIGH},
            {},
            {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ECO, climate::CLIMATE_PRESET_SLEEP,
             climate::CLIMATE_PRESET_BOOST}) {}

  void control(const climate::ClimateCall &call) override;
  void send_display_toggle();
  void send_vertical_swing_toggle();
  void send_horizontal_swing_toggle();

 protected:
  void transmit_state() override;
  void send_power_toggle_();
  void send_eco_(bool enabled);
  void send_sleep_(bool enabled);
  void send_boost_(bool enabled);
  void send_frame_(uint8_t byte2, uint8_t byte3, uint8_t byte6, uint8_t byte7, const uint8_t *frame2,
                   uint8_t byte4 = 0x00, uint8_t byte8 = 0x00, uint8_t byte5 = 0x00);
  static uint8_t temp_byte_(float temp);
  static uint8_t fan_byte_(optional<climate::ClimateFanMode> fan);
  static uint8_t seq_for_temp_(float temp);
};

}  // namespace esphome::hawrin_ac
