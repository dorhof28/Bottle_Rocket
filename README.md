# Bottle Rocket Fin Control

Steer two fin servos from the iPhone with arrow buttons, over Bluetooth.

## What you need
- **Circuit Playground Bluefruit** (the one with Bluetooth - the older Circuit
  Playground Express has no BLE and won't work with a phone).
- 2 hobby servos, plus a separate 4xAA (or similar 4.8-6V) battery pack.
- iPhone app: **Bluefruit LE Connect** (free, by Adafruit).

## Wiring
| Servo wire | Goes to |
|---|---|
| Signal (orange/yellow) | servo 1 -> **A1**, servo 2 -> **A2** |
| Power (red) | battery pack +  (not the board's 3.3V pad - servos will brown out the board) |
| Ground (brown/black) | battery pack - **and** a board GND pad (shared ground) |

Power the board separately (USB or its battery connector).

## Install
1. Install CircuitPython 9.x for Circuit Playground Bluefruit
   (circuitpython.org/board/circuitplayground_bluefruit).
2. From the matching Adafruit CircuitPython Library Bundle, copy these into
   `CIRCUITPY/lib`: `adafruit_ble`, `adafruit_bluefruit_connect`, `adafruit_motor`.
3. Copy `code.py` to `CIRCUITPY`.

## Use
1. Open Bluefruit LE Connect, tap the "CIRCUITPYxxxx" device -> **Connect**.
2. Choose **Controller** -> **Control Pad**.
3. Left/Right moves servo 1, Up/Down moves servo 2. Releasing an arrow
   re-centers that fin. Buttons 1-4: center / 10 / 20 / 30 degree throw.

## Tuning
Edit the top of `code.py`: `CENTER_1/2` (trim so fins are flush), `THROW`,
`INVERT_1/2` (if the rocket steers the wrong way).

## Notes
- Assumes one fin handles left/right and the other up/down (fins on adjacent
  sides). Tell me if your fins are positioned differently.
- The Circuit Playground Bluefruit has an accelerometer built in but **no
  gyroscope**; the gyro would be a separate sensor (e.g. over I2C). Not used
  yet - stabilization/auto-correction could be added later.
- Bluetooth range is only ~10-30 m, and signal from inside a flying rocket is
  unreliable. Test on the ground first, with the rocket held in your hand.
