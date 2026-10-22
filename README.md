# Bottle Rocket Fin Control

Steer two fin servos from the iPhone with arrow buttons, over Bluetooth.

## Wiring
| Servo wire | Goes to |
|---|---|
| Signal (orange/yellow) | servo 1 -> **A3**, servo 2 -> **A2** |
| Power (red) | battery pack +  (not the board's 3.3V pad - servos will brown out the board) |
| Ground (brown/black) | battery pack - **and** a board GND pad (shared ground) |

Power the board separately (USB or its battery connector).

## Install
1. In Arduino IDE, add Adafruit's board package URL under Preferences >
   Additional Boards Manager URLs:
   `https://adafruit.github.io/arduino-board-index/package_adafruit_index.json`
2. Boards Manager: install **Adafruit nRF52**.
3. Select board **Adafruit Circuit Playground Bluefruit**.
4. Open one of the sketches below and upload.

## Sketches
- `bottle_rocket/` - phone arrows plus gyro lean correction you can switch on
  from the app.
- `bottle_rocket_stabilize/` - just the gyro lean correction, no phone.
- `bottle_rocket_demo/` - everything for the demo in one sketch, see below.
- `bottle_rocket_all/` - everything from the phone, upload this one for the
  demo (see below).
- `bottle_rocket_test/` - checks the gyro, both servos and bluetooth one at a
  time and prints PASS or FAIL in the serial monitor (115200).
- `bottle_rocket_launch/` - waits for launch, zigzags the fins, then sweeps
  them 3 times, 30 seconds apart.

The gyro is an Adafruit L3GD20H, see the gyro wiring below.

## Use
1. Open Bluefruit LE Connect, tap "BottleRocket" -> **Connect**.
2. Choose **Controller** -> **Control Pad**.
3. Up/Down: both fins tilt the same way (steers in one plane). Left/Right:
   fins tilt opposite ways (rolls the rocket). Releasing re-centers.
   Button 1: center. Button 2: gyro correction on/off. Button 3: cycles the
   fin throw 10 / 20 / 30. Button 4: sets the current lean as level.

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

## Gyro wiring (L3GD20H)
| Gyro pin | Board pad |
|---|---|
| VIN | 3.3V |
| GND | GND |
| SCL | SCL (A4) |
| SDA | SDA (A5) |

Leave the other pins alone. The code finds the chip at 0x6B or 0x6A on its
own. If the board just sits there after power up, the gyro wasn't found, so
check the wiring.

## Gyro correction
Keep the rocket still when it powers on, it calibrates the gyro for a second
or two. When it is on, the fins push against the lean (arrows still add on
top). Bench test: tilt the nose one way and the fin trailing edges should
swing the other way. If they go the same way, set `STAB_SIGN` to -1. The
gyro X axis is the lean the fins can fix and Z is the spin along the rocket
body, so turn the sensor to match or swap the axes in `readGyro`.

## Launch sketch
Launch is detected by the board's accelerometer (about 2.5 g). Press the left
button on the board to start it on the bench. The three sweeps happen at 30,
60 and 90 seconds after launch.

## All-in-one sketch (bottle_rocket_all)
Connect with Bluefruit LE Connect, then Controller > Control Pad. The Control
Pad has no real switches, so the numbered buttons act as them. Press again to
turn a mode off.

| Button | What it does |
|---|---|
| Arrows | Remote control of the fins |
| 1 | Gyro on/off, fins move opposite to the lean |
| 2 | Zigzag on/off |
| 3 | Push button: once = left, twice = center, three times = right, then repeats |
| 4 | Stop everything and center the fins |

The app also has a UART screen if you want to see "gyro on", "zigzag off" etc.
Keep the rocket still for a second or two at power up. Left and right in
zigzag and push button mean both fins tilting the same way.
