// Bottle rocket fin control - Circuit Playground Bluefruit
// Control with the Adafruit "Bluefruit LE Connect" iPhone app > Controller > Control Pad.
//
// Two OPPOSITE fins are on servos (servo 1 = fin A, servo 2 = fin B).
//   UP / DOWN    -> both fins tilt the SAME way  -> pushes the tail sideways
//                   (steers the nose in the one plane perpendicular to the fins)
//   LEFT / RIGHT -> fins tilt OPPOSITE ways      -> rolls (spins) the rocket
// Roll lets you turn the steering plane to point where you want, so together
// the arrows can aim the rocket any direction.
// Button 1: re-center.  Button 2/3/4: fin throw 10 / 20 / 30 degrees.
// Release an arrow = those fins return to center.

#include <bluefruit.h>
#include <Servo.h>

int CENTER_1 = 90;        // servo 1 angle when fin is straight (trim so fin looks flush)
int CENTER_2 = 90;        // servo 2 angle when fin is straight
int THROW = 20;           // degrees of fin deflection from center
bool INVERT_1 = false;    // flip a servo if its fin tilts the wrong way
bool INVERT_2 = true;     // fins face opposite ways, so servo 2 is usually mirrored.
                          // CALIBRATE: press UP - both fins' trailing edges must
                          // move toward the same side. If not, flip this.
bool LINK_LOST_CENTER = true;   // center fins if the phone disconnects

// Servo signal wires: A1 and A2 pads. Power servos from a separate 4xAA pack
// (common ground with the board), NOT the board's 3.3V pad.
Servo servo1;
Servo servo2;
BLEUart bleuart;

int rollCmd = 0;
int pitchCmd = 0;

int clampInt(int v, int lo, int hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

void setFins(int roll, int pitch) {
  int a = clampInt(pitch + roll, -1, 1);
  int b = clampInt(pitch - roll, -1, 1);
  if (INVERT_1) a = -a;
  if (INVERT_2) b = -b;
  servo1.write(clampInt(CENTER_1 + a * THROW, 0, 180));
  servo2.write(clampInt(CENTER_2 + b * THROW, 0, 180));
}

void handleButton(char b, bool down) {
  switch (b) {
    case '7': rollCmd = down ? -1 : (rollCmd == -1 ? 0 : rollCmd); break;
    case '8': rollCmd = down ? 1 : (rollCmd == 1 ? 0 : rollCmd); break;
    case '6': pitchCmd = down ? -1 : (pitchCmd == -1 ? 0 : pitchCmd); break;
    case '5': pitchCmd = down ? 1 : (pitchCmd == 1 ? 0 : pitchCmd); break;
    case '1': if (down) rollCmd = pitchCmd = 0; break;
    case '2': if (down) THROW = 10; break;
    case '3': if (down) THROW = 20; break;
    case '4': if (down) THROW = 30; break;
  }
  setFins(rollCmd, pitchCmd);
}

void startAdv() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

void disconnectCallback(uint16_t conn_handle, uint8_t reason) {
  rollCmd = pitchCmd = 0;
  if (LINK_LOST_CENTER) setFins(0, 0);
}

void setup() {
  servo1.attach(A1);
  servo2.attach(A2);
  setFins(0, 0);

  Bluefruit.begin();
  Bluefruit.setName("BottleRocket");
  Bluefruit.Periph.setDisconnectCallback(disconnectCallback);
  bleuart.begin();
  startAdv();
}

void loop() {
  while (bleuart.available() >= 5) {
    if (bleuart.peek() != '!') {
      bleuart.read();
      continue;
    }
    uint8_t p[5];
    for (int i = 0; i < 5; i++) p[i] = bleuart.read();
    if (p[1] != 'B') continue;
    uint8_t sum = 0;
    for (int i = 0; i < 4; i++) sum += p[i];
    if ((uint8_t)~sum != p[4]) continue;
    handleButton((char)p[2], p[3] == '1');
  }
}
