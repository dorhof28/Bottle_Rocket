#include <Servo.h>

// one servo on A3, sweeps back and forth forever
Servo myServo;

void setup() {
  Serial.begin(115200);
  myServo.attach(A3);
}

void loop() {
  Serial.println("center");
  myServo.write(90);
  delay(1500);

  Serial.println("left");
  myServo.write(30);
  delay(1500);

  Serial.println("center");
  myServo.write(90);
  delay(1500);

  Serial.println("right");
  myServo.write(150);
  delay(1500);
}
