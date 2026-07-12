#include "hawrin_ac.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::hawrin_ac {
namespace {
const char *const TAG = "hawrin_ac";
const uint8_t FULL_STATE_FRAME2[7] = {0x00, 0x11, 0x00, 0x00, 0x08, 0x00, 0x19};
const uint8_t MODE_FRAME2[7] = {0x00, 0x06, 0x00, 0x00, 0x08, 0x00, 0x0E};
const uint8_t AUTO_MODE_FRAME2[7] = {0x00, 0x17, 0x00, 0x00, 0x08, 0x00, 0x1F};
const uint8_t POWER_FRAME2[7] = {0x00, 0x01, 0x00, 0x00, 0x08, 0x00, 0x09};
const uint8_t ECO_ON_FRAME2[7] = {0x20, 0x0C, 0x00, 0x00, 0x08, 0x00, 0x24};
const uint8_t ECO_OFF_FRAME2[7] = {0x00, 0x0C, 0x00, 0x00, 0x08, 0x00, 0x04};
const uint8_t SLEEP_FRAME2[7] = {0x00, 0x03, 0x00, 0x00, 0x08, 0x00, 0x0B};
const uint8_t BOOST_FRAME2[7] = {0x00, 0x04, 0x00, 0x00, 0x08, 0x00, 0x0C};
const uint8_t DISPLAY_FRAME2[7] = {0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08};
const uint8_t VERTICAL_SWING_FRAME2[7] = {0x00, 0x07, 0x00, 0x00, 0x08, 0x00, 0x0F};
const uint8_t HORIZONTAL_SWING_FRAME2[7] = {0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x00};

void encode_byte(remote_base::RemoteTransmitData *data, uint8_t value) {
  for (uint8_t i = 0; i < 8; i++)
    data->item(580, (value & (1 << i)) ? 1680 : 580);
}
}  // namespace

void HawrinACClimate::control(const climate::ClimateCall &call) {
  const auto old_preset = this->preset.value_or(climate::CLIMATE_PRESET_NONE);

  if (call.get_mode().has_value())
    this->mode = *call.get_mode();
  if (call.get_target_temperature().has_value())
    this->target_temperature = *call.get_target_temperature();
  if (call.get_fan_mode().has_value())
    this->fan_mode = *call.get_fan_mode();
  if (call.get_preset().has_value())
    this->preset = *call.get_preset();

  if (this->mode == climate::CLIMATE_MODE_OFF) {
    this->send_power_toggle_();
  } else if (this->preset.value_or(climate::CLIMATE_PRESET_NONE) != old_preset) {
    const auto new_preset = this->preset.value_or(climate::CLIMATE_PRESET_NONE);
    if (old_preset == climate::CLIMATE_PRESET_ECO && new_preset != climate::CLIMATE_PRESET_ECO)
      this->send_eco_(false);
    if (old_preset == climate::CLIMATE_PRESET_SLEEP && new_preset != climate::CLIMATE_PRESET_SLEEP)
      this->send_sleep_(false);
    if (old_preset == climate::CLIMATE_PRESET_BOOST && new_preset != climate::CLIMATE_PRESET_BOOST)
      this->send_boost_(false);
    if (new_preset == climate::CLIMATE_PRESET_ECO)
      this->send_eco_(true);
    if (new_preset == climate::CLIMATE_PRESET_SLEEP)
      this->send_sleep_(true);
    if (new_preset == climate::CLIMATE_PRESET_BOOST)
      this->send_boost_(true);
  } else {
    this->transmit_state();
  }

  this->publish_state();
}

void HawrinACClimate::transmit_state() {
  if (this->mode == climate::CLIMATE_MODE_OFF) {
    this->send_power_toggle_();
    return;
  }

  switch (this->mode) {
    case climate::CLIMATE_MODE_HEAT_COOL:
      this->send_frame_(0x01, 0x71, 0x91, 0x1E, AUTO_MODE_FRAME2, 0x80);
      return;
    case climate::CLIMATE_MODE_DRY:
      this->send_frame_(0x00, 0x73, 0x91, 0x1E, MODE_FRAME2);
      return;
    case climate::CLIMATE_MODE_FAN_ONLY:
      this->send_frame_(0x01, 0x84, 0x91, 0x20, MODE_FRAME2);
      return;
    default:
      break;
  }

  const uint8_t b2 = fan_byte_(this->fan_mode);
  const uint8_t b3 = temp_byte_(this->target_temperature);
  this->send_frame_(b2, b3, 0x90, seq_for_temp_(this->target_temperature), FULL_STATE_FRAME2);
}

void HawrinACClimate::send_power_toggle_() {
  this->send_frame_(0x04, temp_byte_(this->target_temperature), 0x8F, 0x13, POWER_FRAME2);
}

void HawrinACClimate::send_eco_(bool enabled) {
  this->send_frame_(enabled ? 0x03 : 0x00, temp_byte_(this->target_temperature), 0x91, 0x05,
                    enabled ? ECO_ON_FRAME2 : ECO_OFF_FRAME2);
}

void HawrinACClimate::send_sleep_(bool enabled) {
  this->send_frame_(enabled ? 0x08 : 0x00, temp_byte_(this->target_temperature), 0x94, 0x22, SLEEP_FRAME2);
}

void HawrinACClimate::send_boost_(bool enabled) {
  if (enabled) {
    this->send_frame_(0x01, 0x02, 0x97, 0x39, BOOST_FRAME2, 0x00, 0x00, 0x90);
  } else {
    this->send_frame_(0x00, temp_byte_(this->target_temperature), 0x97, 0x34, BOOST_FRAME2);
  }
}

void HawrinACClimate::send_display_toggle() {
  this->send_frame_(0x00, temp_byte_(this->target_temperature), 0xB1, 0x06, DISPLAY_FRAME2);
}

void HawrinACClimate::send_vertical_swing_toggle() {
  this->send_frame_(0x80, temp_byte_(this->target_temperature), 0x94, 0x23, VERTICAL_SWING_FRAME2, 0x00, 0x40);
}

void HawrinACClimate::send_horizontal_swing_toggle() {
  this->send_frame_(0x00, temp_byte_(this->target_temperature), 0x94, 0x24, HORIZONTAL_SWING_FRAME2, 0x00, 0x80);
}

void HawrinACClimate::send_frame_(uint8_t byte2, uint8_t byte3, uint8_t byte6, uint8_t byte7,
                                  const uint8_t *frame2, uint8_t byte4, uint8_t byte8, uint8_t byte5) {
  const uint8_t frame0[6] = {0x83, 0x06, byte2, byte3, byte4, byte5};
  const uint8_t frame1[8] = {byte6, byte7, byte8, 0x00, 0x00, 0x00, 0x00,
                             static_cast<uint8_t>(byte2 ^ byte3 ^ byte4 ^ byte5 ^ byte6 ^ byte7 ^ byte8)};

  auto call = this->transmitter_->transmit();
  auto *data = call.get_data();
  data->set_carrier_frequency(38000);
  data->reserve(344);

  data->item(9070, 4520);
  for (auto b : frame0)
    encode_byte(data, b);
  data->item(580, 8050);
  for (auto b : frame1)
    encode_byte(data, b);
  data->item(580, 8050);
  for (uint8_t i = 0; i < 7; i++)
    encode_byte(data, frame2[i]);
  data->item(580, 10120);

  ESP_LOGD(TAG, "Sending bytes: %02X %02X %02X %02X %02X %02X %02X %02X ... %02X", frame0[0], frame0[1],
           frame0[2], frame0[3], frame0[4], frame0[5], frame1[0], frame1[1], frame1[7]);
  call.perform();
}

uint8_t HawrinACClimate::temp_byte_(float temp) {
  const auto value = static_cast<uint8_t>(roundf(clamp(temp, 20.0f, 30.0f)));
  return static_cast<uint8_t>(((value - 16) << 4) | 0x02);
}

uint8_t HawrinACClimate::fan_byte_(optional<climate::ClimateFanMode> fan) {
  switch (fan.value_or(climate::CLIMATE_FAN_AUTO)) {
    case climate::CLIMATE_FAN_HIGH:
      return 0x01;
    case climate::CLIMATE_FAN_MEDIUM:
      return 0x02;
    case climate::CLIMATE_FAN_LOW:
      return 0x03;
    case climate::CLIMATE_FAN_AUTO:
    default:
      return 0x00;
  }
}

uint8_t HawrinACClimate::seq_for_temp_(float temp) {
  const auto value = static_cast<uint8_t>(roundf(clamp(temp, 20.0f, 30.0f)));
  if (value <= 23)
    return 0x11;
  if (value <= 26)
    return 0x12;
  return 0x13;
}

}  // namespace esphome::hawrin_ac
