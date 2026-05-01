#include <Servo.h>

Servo base;      // pin 2
Servo shoulder;  // pin 3
Servo elbow;     // pin 4
Servo gripper;   // pin 5

// current angles
int baseAngle = 90;
int shoulderAngle = 90;
int elbowAngle = 45;
int gripperAngle = 90;

// speed control
int speedDelay = 15;

// base positions
int baseHome = 90;
int bin1 = 40;
int bin2 = 70;
int bin3 = 120;
int bin4 = 150;

void setup() {

  Serial.begin(9600);

  base.attach(2);
  shoulder.attach(3);
  elbow.attach(4);
  gripper.attach(5);

  homePosition();
}

void loop() {

  if (Serial.available()) {

    int command = Serial.parseInt();
    Serial.read();

    if(command >= 0 && command <= 4)
    {
      executeCycle(command);
    }
  }
}


// ===== MAIN SEQUENCE =====
void executeCycle(int command)
{
  pickObject();

  // Move to bin
  switch(command)
  {
    case 0: moveServoSlow(base, baseAngle, baseHome); break;
    case 1: moveServoSlow(base, baseAngle, bin1); break;
    case 2: moveServoSlow(base, baseAngle, bin2); break;
    case 3: moveServoSlow(base, baseAngle, bin3); break;
    case 4: moveServoSlow(base, baseAngle, bin4); break;
  }

  delay(300);

  dropObject();

  delay(300);

  // Reset arm
  moveServoSlow(shoulder, shoulderAngle, 90);
  moveServoSlow(elbow, elbowAngle, 45);

  delay(200);

  // Return base to home
  moveServoSlow(base, baseAngle, baseHome);
}


// ===== Smooth Movement =====
void moveServoSlow(Servo &servo, int &currentAngle, int targetAngle)
{
  if(currentAngle < targetAngle)
  {
    for(int pos = currentAngle; pos <= targetAngle; pos++)
    {
      servo.write(pos);
      delay(speedDelay);
    }
  }
  else
  {
    for(int pos = currentAngle; pos >= targetAngle; pos--)
    {
      servo.write(pos);
      delay(speedDelay);
    }
  }
  currentAngle = targetAngle;
}


// ===== PICK OBJECT =====
void pickObject()
{
  moveServoSlow(shoulder, shoulderAngle, 135);
  delay(200);

  moveServoSlow(gripper, gripperAngle, 90);
  delay(200);

  moveServoSlow(gripper, gripperAngle, 45);
  delay(150);

  moveServoSlow(shoulder, shoulderAngle, 90);
  moveServoSlow(elbow, elbowAngle, 45);
}


// ===== DROP OBJECT (UPDATED) =====
void dropObject()
{
  // Shoulder only to 100°
  moveServoSlow(shoulder, shoulderAngle, 100);
  delay(200);

  // Elbow stays at 45°

  // Open gripper
  moveServoSlow(gripper, gripperAngle, 90);
}


// ===== HOME =====
void homePosition()
{
  moveServoSlow(base, baseAngle, baseHome);
  moveServoSlow(shoulder, shoulderAngle, 90);
  moveServoSlow(elbow, elbowAngle, 45);
  moveServoSlow(gripper, gripperAngle, 90);
}