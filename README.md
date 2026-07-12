# Vizo AC ESPHome external component

Minimal IR climate encoder derived from captured Pronto/raw codes.

Supported:

- `cool`
- temperature `20-30`
- fan `auto`, `low`, `medium`, `high`
- preset `eco`
- display toggle button via lambda

Known limitation:

- `off` uses the original remote's power toggle code. Without a real AC state sensor it can get out of sync.
- `display` is also a toggle; the captured on/off codes are identical.

Example:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/osk2/hawrin-esphome-climate.git

remote_transmitter:
  id: ir_tx
  pin: GPIO4
  carrier_duty_percent: 50%

climate:
  - platform: vizo_ac
    id: bedroom_ac
    name: "Bedroom AC"
    transmitter_id: ir_tx
    supports_heat: false

button:
  - platform: template
    name: "Bedroom AC Display Toggle"
    on_press:
      - lambda: |-
          id(bedroom_ac).send_display_toggle();
```
