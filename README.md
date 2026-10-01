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
1. In Arduino IDE, add Adafruit's board package URL under Preferences >
   Additional Boards Manager URLs:
   `https://adafruit.github.io/arduino-board-index/package_adafruit_index.json`
2. Boards Manager: install **Adafruit nRF52**.
3. Select board **Adafruit Circuit Playground Bluefruit**.
4. Open `bottle_rocket/bottle_rocket.ino` and upload.

## Use
1. Open Bluefruit LE Connect, tap "BottleRocket" -> **Connect**.
2. Choose **Controller** -> **Control Pad**.
3. Up/Down: both fins tilt the same way (steers in one plane). Left/Right:
   fins tilt opposite ways (rolls the rocket). Releasing re-centers.
   Buttons 1-4: center / 10 / 20 / 30 degree throw.

## How one pair of fins steers
A fin pair can only push the tail sideways in the plane perpendicular to the
fins, so Up/Down steers just one axis. To go "left" or "right" you first roll
the rocket (Left/Right arrows) until that steering plane points the way you
want, then use Up/Down. Fins on servos: mount them opposite each other.
**Calibrate on the bench:** press Up - both fins' trailing edges should move to
the same side; press Left - they should move opposite. If Up looks wrong, flip
`INVERT_2` (or `INVERT_1`).

## Tuning
Edit the top of the sketch: `CENTER_1/2` (trim so fins are flush), `THROW`,
`INVERT_1/2`.

## Notes
- Because only one pair moves, the other pair stays fixed as stabilizers.
- The Circuit Playground Bluefruit has an accelerometer built in but **no
  gyroscope**; the gyro would be a separate sensor (e.g. over I2C). Not used
  yet - stabilization/auto-correction could be added later.
- Bluetooth range is only ~10-30 m, and signal from inside a flying rocket is
  unreliable. Test on the ground first, with the rocket held in your hand.
