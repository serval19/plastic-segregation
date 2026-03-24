#include <Servo.h>

// Create servo objects for each motor
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;
Servo servo5;
Servo servo6;

// Define the pins for each servo
const int servoPin1 = 2;
const int servoPin2 = 3;
const int servoPin3 = 4;
const int servoPin4 = 5;
const int servoPin5 = 6;
const int servoPin6 = 7;

void setup() {
  // Attach each servo to its pin
  servo1.attach(servoPin1);
  servo2.attach(servoPin2);
  servo3.attach(servoPin3);
  servo4.attach(servoPin4);
  servo5.attach(servoPin5);
  servo6.attach(servoPin6);
  
  // Move all servos to 90 degrees
  servo1.write(90);
  servo2.write(90);
  servo3.write(90);
  servo4.write(90);
  servo5.write(90);
  servo6.write(90);
  
  // Optional: Add a small delay to ensure all servos reach position
  delay(1000);
}

void loop() {
  // Nothing needed in loop since we just want them at 90 degrees
  // The servos will maintain their position
}