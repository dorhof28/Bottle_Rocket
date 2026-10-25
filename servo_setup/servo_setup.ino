#include <Servo.h>

// pins, same as the main sketch
const int servo1Pin = A6;   // RX pad
const int servo2Pin = A7;   // TX pad

Servo servo1;
Servo servo2;

int angle1 = 90;
int angle2 = 90;
int current = 1;   // which servo the keys move

int center1 = 90, center2 = 90;
int left1 = 90, left2 = 90;
bool centerSet = false;
bool leftSet = false;

void showAngles() {
  Serial.print("servo 1: ");
  Serial.print(angle1);
  Serial.print("   servo 2: ");
  Serial.print(angle2);
  Serial.print("   (moving servo ");
  Serial.print(current);
  Serial.println(")");
}

void moveCurrent(int amount) {
  if (current == 1) {
    angle1 = constrain(angle1 + amount, 0, 180);
    servo1.write(angle1);
  } else {
    angle2 = constrain(angle2 + amount, 0, 180);
    servo2.write(angle2);
  }
  showAngles();
}

void showHelp() {
  Serial.println("1 / 2  pick the servo to move");
  Serial.println("a / d  move it 1 degree down / up");
  Serial.println("q / e  move it 5 degrees down / up");
  Serial.println("c      save both servos as CENTER (fins straight)");
  Serial.println("l      save both servos as LEFT (fins where you want left)");
  Serial.println("p      print the numbers to paste into the sketch");
  Serial.println("r      put both servos back to 90");
  Serial.println("h      show this list");
  Serial.println("you can type a few letters at once, like ddd");
}

void printResult() {
  if (!centerSet) {
    Serial.println("set the center first (press c)");
    return;
  }
  Serial.println("---- paste these at the top of bottle_rocket_all ----");
  Serial.print("int center1 = ");
  Serial.print(center1);
  Serial.println(";");
  Serial.print("int center2 = ");
  Serial.print(center2);
  Serial.println(";");

  if (!leftSet) {
    Serial.println("(set left with l to get throwAmt and flip too)");
    return;
  }

  int move1 = abs(left1 - center1);
  int move2 = abs(left2 - center2);
  Serial.print("int throwAmt = ");
  Serial.print((move1 + move2) / 2);
  Serial.println(";");
  Serial.print("bool flip1 = ");
  Serial.print(left1 > center1 ? "true" : "false");
  Serial.println(";");
  Serial.print("bool flip2 = ");
  Serial.print(left2 > center2 ? "true" : "false");
  Serial.println(";");

  if (abs(move1 - move2) > 3) {
    Serial.println("note: the two servos moved different amounts, throwAmt is the average");
  }
  Serial.println("------------------------------------------------------");
}

void setup() {
  Serial.begin(115200);
  int wait = 0;
  while (!Serial && wait < 500) {
    delay(20);
    wait++;
  }
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);
  servo1.write(angle1);
  servo2.write(angle2);

  Serial.println("servo setup");
  showHelp();
  showAngles();
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '1') {
      current = 1;
      showAngles();
    }
    if (c == '2') {
      current = 2;
      showAngles();
    }
    if (c == 'a') moveCurrent(-1);
    if (c == 'd') moveCurrent(1);
    if (c == 'q') moveCurrent(-5);
    if (c == 'e') moveCurrent(5);
    if (c == 'c') {
      center1 = angle1;
      center2 = angle2;
      centerSet = true;
      Serial.println("center saved");
    }
    if (c == 'l') {
      left1 = angle1;
      left2 = angle2;
      leftSet = true;
      Serial.println("left saved");
    }
    if (c == 'p') printResult();
    if (c == 'r') {
      angle1 = 90;
      angle2 = 90;
      servo1.write(angle1);
      servo2.write(angle2);
      showAngles();
    }
    if (c == 'h') showHelp();
  }
}
