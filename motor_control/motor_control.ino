#include <Wire.h>

#include <Servo.h>


byte motordata; /* motor data code:


0x42- "B" raise head - high value
0x43- "C" lower head- low value
0x44- "D" rotate rubik's claw
0x45- "E" depress left-front button for Simon
0x46- "F" depress left-rear button
0x47- "G" depress right-front button 75
0x48- "H" depress right-rear button 123
0x49- "I" open front claw
0x4A- "J" close front claw
0x61- "a" start stepper sequence from beginning
0x62- "b" abort stepper motor attempt
0x63- "c" pause stepper motor writing
0x64- "d" resume stepper motor writing
*/
//pin definitions
#define RAISE_HEAD 0x42
#define LOWER_HEAD 0x43
#define ROTATE_CUBE 0x44
#define LEFT_FRONT 0x45
#define LEFT_REAR 0x46
#define RIGHT_FRONT 0x47
#define RIGHT_REAR 0x48
#define OPEN_CLAW 0x49
#define CLOSE_CLAW 0x4A
#define STEPPER_START 0x61
#define STEPPER_ABORT 0x62
#define STEPPER_PAUSE 0x63
#define STEPPER_RESUME 0x64

#define ENABLE_PIN 13
#define STEP1_PIN 7
#define STEP2_PIN 4
#define DIR1_PIN 9
#define DIR2_PIN 8
#define PWM1_PIN 3 //pin assignments to the on-board headers
#define PWM2_PIN 5
#define PWM3_PIN 6
////////////////// ARDUINO PINS 10 AND 11 ALSO CONTROL SERVOS

//Defines stepper motor related methods
#define STEP_DELAY 100

//stepper motor prototypes
void stepper_init();
void stepper_enable();
void stepper_disable();
void step_ch1();
void step_ch2();
void setdir_ch1(boolean d);
void setdir_ch2(boolean d);
void stepper_up();
void stepper_down();
void stepper_right();
void stepper_left();

//instructions (generate from matlab)
int instr[] = {2,1,1,4,1,1,4,7,8,8,8,8,8,8,8,8,8,8,9,8,9,6,6,6,6,6,3,3,3,3,3,3,3,2,1,2,2,1,2,2,2,2,2,2,2,2,2,3,2,2,2,3,2,2,3,2,3,2,3,2,3,3,3,6,3,6,9,8,8,8,8,7,8,8,8,8,7,8,8,8,7,8,8,8,7,8,7,8,8,7,8,7,8,7,8,7,8,8,8,9,9,6,9,6,6,6,6,6,3,6,6,2,2,2,2,2,2,2,2,2,2,2,3,3,2,3,3,3,6,6,6,6,8,6,9,8,8,8,8,7,8,7,8,7,7,7,7,7,4,7,4,8,8,6,8,9,6,6,9,3,6,6,6,6,3,6,3,6,6,6,2,2,2,2,2,2,2,2,2,2,2,2,3,2,3,2,3,3,3,3,6,6,6,6,9,9,9,9,8,8,7,8,7,7,7,7,8,7,4,7,7,7,4,7,7,8,9,9,9,6,6,6,6,6,6,6,6,3,6,3,6,3,2,2,2,2,2,2,2,2,2,2,2,3,2,2,2,3,3,3,6,3,6,6,6,6,9,9,9,8,8,7,8,8,7,8,7,8,7,4,8,4,7,7,7,7,4,4,8,9,9,6,9,6,6,6,6,6,6,3,6,6,6,6,6};
int instrlen = 295;

boolean stepper_enabled = false;
boolean leftWrittenRear = false;
boolean rightWrittenRear = false;
boolean rightActive = false;
boolean leftActive = false;
boolean rubiksActive = false;

//misc global variable
long lasttime = 0; //last time an operation has occured. May need to be changed to last time of stepper operation 
int instri = 0; //index of current instruction

Servo frontClaw;
Servo armLift;
Servo rubiksTwist;
Servo leftSimon;
Servo rightSimon;
long rubiksTimer = 0;
long simonLeftTimer = 0;
long simonRightTimer = 0;

void setup() {
 
  frontClaw.attach(11);
  armLift.attach(PWM1_PIN); // PWM 1
  
  leftSimon.attach(PWM2_PIN);// PWM 2
  rightSimon.attach(PWM3_PIN); // PWM 3
  stepper_init();
  Serial.begin(9600);
  setdir_ch1(false);
  setdir_ch2(false);
  stepper_disable();
}

void loop() {

  motordata = 0;
 while(Serial.available()) {
   motordata = Serial.read();
 }
 switch (motordata) {
   case RAISE_HEAD:
     armLift.write(96);
     Serial.write(0x42);
     break;
   case LOWER_HEAD:
     armLift.write(84);
     Serial.write(0x43);
     break;
   case ROTATE_CUBE:
     rubiksTwist.attach(10);
     rubiksTimer = millis() + 700;
     rubiksTwist.write(100);
     rubiksActive = true;
     break;
   case LEFT_FRONT:
     simonLeftTimer = millis() + 1000;
     leftSimon.write(97);//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     leftWrittenRear = false;
     leftActive = true;
     break;
   case LEFT_REAR:// left rear
     simonLeftTimer = millis() + 1000;
     leftSimon.write(55);//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     leftWrittenRear = true;
     leftActive = true;
     break;     
   case RIGHT_FRONT:
     simonRightTimer = millis() + 1000;
     rightSimon.write(75);
     rightWrittenRear = false;
     rightActive = true;
     break;     
   case RIGHT_REAR:// right rear
     simonRightTimer = millis() + 1000;
     rightSimon.write(123);
     rightWrittenRear = true;
     rightActive = true;
     break;     
   case OPEN_CLAW: // high open, low close
     frontClaw.attach(11);
     frontClaw.write(100);
     Serial.write(OPEN_CLAW);
     break;     
   case CLOSE_CLAW:
     frontClaw.attach(11);
     frontClaw.write(85);
     Serial.write(CLOSE_CLAW);
     break;     
   case 0x4B:
       frontClaw.detach();
      Serial.write(0x4B);
   case STEPPER_START: // begin
     stepper_enabled = true;
     stepper_enable();
     instri = 0;
     break;     
   case STEPPER_ABORT: // abort
     stepper_enabled = false;
     stepper_disable();
     instri = 0;
     break;     
   case STEPPER_PAUSE: // pause
     stepper_enabled = false;
     stepper_disable();
     break;     
   case STEPPER_RESUME: // resume
     instri = 0;
     stepper_enable();
     break;   
   default:
     break;
     
 }
 if ((millis() >= rubiksTimer) && rubiksActive) {
   rubiksTwist.write(90);
   rubiksTwist.detach();
   rubiksActive = false;
   Serial.write(0x44);
  
 }
 if ((millis() >= simonLeftTimer) && leftActive) {
   leftSimon.write(76);
   leftActive = false;
   if(leftWrittenRear) {
     Serial.write(0x46);
   }
   else {
     Serial.write(0x45);
   }
 }
 if ((millis() >= simonRightTimer) && rightActive) {
   rightSimon.write(99);
   rightActive = false;
   if(rightWrittenRear) {
     Serial.write(0x48);
   }
   else {
     Serial.write(0x47);
   }
 }
 if (lasttime + STEP_DELAY < millis() && stepper_enabled) {
    
    switch(instr[instri]) {
      
      /*
		the codes are shown by the following:
        1 2 3
        4 5 6
        7 8 9
		8 is directly down, 2 is directly up, and so on
      */
      
      case 1:
        stepper_up();
        stepper_left();
        break;
      case 2:
        stepper_up();
        break;
      case 3:
        stepper_right();
        stepper_up();
        break;
      case 4:
        stepper_left();
        break;
      case 6:
        stepper_right();
        break;
      case 7:
        stepper_down();
        stepper_left();
        break;
      case 8:
        stepper_down();
        break;
      case 9:
        stepper_down();
        stepper_right();
        break;
      case 0:
        stepper_disable();
        break;
      default:
        break;
      
    }
    instri++;
    if (instri >= instrlen) { //if out of bounds
      if(stepper_enabled == true) {
        Serial.write(0x61); // echo the initiate command
      }
      stepper_enabled = false;
      instri = 0;
      stepper_disable(); //stop everything
    }

    lasttime = millis(); //update last time the stepper motor has ran
  }
 
 
 
 
 
 
}// end main loop~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



//begin stepper motor code (this should be in a seperate file)
void stepper_init() {
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(STEP1_PIN, OUTPUT);
  pinMode(STEP2_PIN, OUTPUT);
  pinMode(DIR1_PIN, OUTPUT);
  pinMode(DIR2_PIN, OUTPUT);
  stepper_disable();
  setdir_ch1(true);
  setdir_ch2(true);
}


//begin stepper motor functions
void stepper_enable() {
  digitalWrite(ENABLE_PIN, LOW);
}

void stepper_disable() {
  digitalWrite(ENABLE_PIN, HIGH);
}

void step_ch1() {
  digitalWrite(STEP1_PIN, HIGH);
  digitalWrite(STEP1_PIN, LOW); 
}

void step_ch2() {
  digitalWrite(STEP2_PIN, HIGH);
  digitalWrite(STEP2_PIN, LOW);
}

void setdir_ch1(boolean d) {
  if (d) {
    digitalWrite(DIR1_PIN, HIGH);
    digitalWrite(DIR1_PIN, LOW);
  }
  else {
    digitalWrite(DIR1_PIN, LOW);
    digitalWrite(DIR1_PIN, HIGH);
  }
}

void setdir_ch2(boolean d) {
  if (d) {
    digitalWrite(DIR2_PIN, HIGH);
    digitalWrite(DIR2_PIN, LOW);
  }
  else {
    digitalWrite(DIR2_PIN, LOW);
    digitalWrite(DIR2_PIN, HIGH);
  }
}

void stepper_up() {
   setdir_ch1(true);
   step_ch1();
}
void stepper_down() {
   setdir_ch1(false);
   step_ch1();
}
void stepper_right() {
   setdir_ch2(true);
   step_ch2();
}
void stepper_left() {
   setdir_ch2(false);
   step_ch2();
}
     
     
     
     
     
     
     
