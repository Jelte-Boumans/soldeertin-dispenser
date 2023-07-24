// Soldeertin dispenseer
// Made by Jelte Boumans 2021-2022
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <BasicEncoder.h>

// Rotary encoder pins
#define RE_BUTTON 2
#define RE_A 3
#define RE_B 4

// DRV8825 Step pins
#define STEP1 5
#define STEP2 6
#define STEP3 7

// Servo pin
#define SERVO 8

// Limit switches pins
#define LIM1 9
#define LIM2 10

// Manual motor control buttons
#define BTN1 11
#define BTN2 12
#define BTN3 13

// Min and max length of a dispensed wire
#define MAX_LEN 50
#define MIN_LEN 5

// Values to open or close scissor
#define SERVO_OPEN 180
#define SERVO_CLOSE 0

// Delay when going to another state
#define NEXT_STATE_DELAY 250

// The ammount of steps it takes to extrude 1cm of wire (with microstep en 10µs delay)
#define STEPS_PER_CM 4500L

// Change this from true to false to disable the lidOpen function
// Set this to false when debugging
#define ENABLE_LID_SECURITY true

Servo myServo;
LiquidCrystal_I2C lcd(0x27,16,2);
BasicEncoder encoder(RE_A, RE_B);

enum States {
  mainmenu,
  motorcontrol,
  selectDiameter,
  selectLength,
  dispense,
  cut
};

States state = mainmenu;

byte selectedDiameter = 0;
byte selectedLength = MIN_LEN;

void setup() { 
  pinMode(RE_A, INPUT);
  pinMode(RE_B, INPUT);
  pinMode(RE_BUTTON, INPUT_PULLUP);

  pinMode(STEP1, OUTPUT);
  pinMode(STEP2, OUTPUT);
  pinMode(STEP3, OUTPUT);

  pinMode(SERVO, OUTPUT);

  pinMode(LIM1, INPUT_PULLUP);
  pinMode(LIM2, INPUT_PULLUP);

  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);

  myServo.attach(SERVO);
  myServo.write(SERVO_OPEN);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  setup_encoders(RE_A,RE_B);

  digitalWrite(STEP1, LOW); // Start with all step pins low
  digitalWrite(STEP2, LOW);
  digitalWrite(STEP3, LOW);
}

void loop() {
  switch(state) {
    case mainmenu:
      f_mainmenu();
      break;

    case motorcontrol:
      f_motorcontrol();
      break;

    case selectDiameter:
      f_selectDiameter();
      break;

    case selectLength:
      f_selectLength();
      break;

    case dispense:
      f_dispense();
      break;

    case cut:
      f_cut();
      break;
  }
}

bool yCord = 0;
void f_mainmenu() {
  lcd.setCursor(0,0);
  lcd.print("Dispenser");
  lcd.setCursor(0,1);
  lcd.print("Change roll");

  int encoder_change = encoder.get_change();
  if(encoder_change) {  // Move cursor position when you turn the RE
    yCord = !yCord;
    encoder_change = 0;
  }

  lcd.setCursor(15,yCord);  // Print cursor
  lcd.print("<");
  lcd.setCursor(15,!yCord);
  lcd.print(" ");
    
  
  if(!digitalRead(RE_BUTTON)) { // When the RE is pressed go to the next state depending on the cursor position
    if(yCord == 0)
      state = selectDiameter;
    else
      state = motorcontrol;

    lcd.clear();  // Clear LCD for next state
    delay(NEXT_STATE_DELAY);  // Delay so you dont skip states when RE is pressed
  }
}

byte stepPin = 0;
void f_motorcontrol() {
  lcd.setCursor(0,0);
  lcd.print("    Press RE    ");
  lcd.setCursor(0,1);
  lcd.print("   to go back   ");

  if(!digitalRead(BTN1))  // The step pin that is used will change depending on which button is pressed
    stepPin = STEP1;
  else if(!digitalRead(BTN2))
    stepPin = STEP2;
//  else if(!digitalRead(BTN3))
//    stepPin = STEP3;
  else
    stepPin = 0;

  while(stepPin) { // If one of the buttons is pressed, send pulses to the selected step pin
    digitalWrite(stepPin, !digitalRead(stepPin));
    delayMicroseconds(10);

    if(digitalRead(BTN1) && digitalRead(BTN2))
      stepPin = 0;
    }

  
  if(!digitalRead(RE_BUTTON)) { // When the RE is pressed go back to the main menu
    state = mainmenu;
    lcd.clear();  // Clear LCD for next state
    delay(NEXT_STATE_DELAY);  // Delay so you dont skip states when RE is pressed
  }
}

byte xCord = 3;
void f_selectDiameter() {
  lcd.setCursor(0,0);
  lcd.print("  Diameter(mm)  ");
  lcd.setCursor(0,1);
  lcd.print("0.3   0.6   1.0 ");

  // If RE is turned CW
  int encoder_change = encoder.get_change();
  if(encoder_change >= 1) {
    if(xCord >= 15)  // Move cursor to the right
      xCord = 3;
    else
      xCord += 6; 
    encoder_change = 0;
  }

  // If RE is turned CCW
  else if(encoder_change <= -1) {
    if(xCord <= 3)  // Move cursor to the left
      xCord = 15;
    else
      xCord -= 6; 
    encoder_change = 0;
  }

  lcd.setCursor(xCord,1); // Print cursor
  lcd.print("<");

  if(!digitalRead(RE_BUTTON)) { // When the RE is pressed save the selected diameter the cursor was on
    if(xCord == 3)
      selectedDiameter = 1;
    else if (xCord == 9)
      selectedDiameter = 2;
    else if (xCord == 15)
      selectedDiameter = 3;

    xCord = 3;  // Reset variable for next time
    state = selectLength;
    lcd.clear();  // Clear LCD for next state
    delay(NEXT_STATE_DELAY);  // Delay so you dont skip states when RE is pressed
  }
}

void f_selectLength() {
  lcd.setCursor(3,0);
  lcd.print("Length(cm)");
  lcd.setCursor(7,1);
  lcd.print("=  ");

  // If RE is turned
  int encoder_change = encoder.get_change();
  if(encoder_change) {
    if(encoder_change >= 1) // If RE is turned CW
      selectedLength++;
    else if(encoder_change <= -1) // If RE is turned CCW
      selectedLength--;

    if(selectedLength > MAX_LEN) // Limit length value
      selectedLength = MIN_LEN;
    else if(selectedLength < MIN_LEN)
      selectedLength = MAX_LEN;
  }

  lcd.setCursor(8,1); // Print selected length
  lcd.print(selectedLength);

  if(!digitalRead(RE_BUTTON)) { // When the RE is pressed start dispensing the selected length
    state = dispense;
    lcd.clear();  // Clear LCD for next state
    delay(NEXT_STATE_DELAY);  // Delay so you dont skip states when RE is pressed
  }
}

void f_dispense() {
  lcd.setCursor(2,0);
  lcd.print("Dispensing...");
  
  if(selectedDiameter == 1)
    stepPin = STEP1;
  else if (selectedDiameter == 2)
    stepPin = STEP2;
  else if (selectedDiameter == 3)
    stepPin = STEP3;

  for(unsigned long i = 0; i < selectedLength*STEPS_PER_CM; i++) {
    if((!digitalRead(LIM1) || !digitalRead(LIM2)) && ENABLE_LID_SECURITY) {  // If lid is opened, stop dispending
      lidOpen();
      lcd.setCursor(2,0);
      lcd.print("Dispensing...");
    }    
              
    digitalWrite(stepPin, !digitalRead(stepPin));
    delayMicroseconds(10);
  }

  selectedLength = MIN_LEN; // Reset variables for next time

  state = cut;
  lcd.clear();  // Clear LCD for next state
  delay(NEXT_STATE_DELAY);  // Delay so you dont skip states when RE is pressed
}

void f_cut() {
  lcd.setCursor(3,0);
  lcd.print("Cutting...");
  
  for(byte i = 0; i < 3; i++) { // Open and close scissor 3 times
    if((!digitalRead(LIM1) || !digitalRead(LIM2)) && ENABLE_LID_SECURITY) {  // If lid is opened, stop cutting
      lidOpen();
      lcd.setCursor(3,0);
      lcd.print("Cutting...");
    }   
      
    myServo.write(SERVO_CLOSE);
    delay(600);
    myServo.write(SERVO_OPEN);
    delay(600);
  }

  for(unsigned long i = 0; i < 3*STEPS_PER_CM; i++) {  // Dispense a little bit of wire to set it up for the next time
    if((!digitalRead(LIM1) || !digitalRead(LIM2)) && ENABLE_LID_SECURITY) {  // If lid is opened, stop dispending
      lidOpen();
      lcd.setCursor(3,0);
      lcd.print("Cutting...");
    }    
              
    digitalWrite(stepPin, !digitalRead(stepPin));
    delayMicroseconds(10);
  }

  state = mainmenu;
  lcd.clear();  // Clear LCD for next state
  delay(NEXT_STATE_DELAY);  // Delay so you dont skip states when RE is pressed
}

void lidOpen() {
  lcd.clear();
  while(!digitalRead(LIM1) || !digitalRead(LIM2)) { // Dont exit this function until the lid is closed
    lcd.setCursor(4,0);
    lcd.print("Close lid");
    lcd.setCursor(3,1);
    lcd.print("to continue");
  }
  
  lcd.clear();  // Clear LCD for next state
  delay(NEXT_STATE_DELAY);  // Delay so you dont skip states when RE is pressed
}

void pciSetup(byte pin) { // Setup pin change interupt on pin
  *digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR |= bit(digitalPinToPCICRbit(pin));  // clear outstanding interrupt
  PCICR |= bit(digitalPinToPCICRbit(pin));  // enable interrupt for group
}

void setup_encoders(int a, int b) {
  uint8_t old_sreg = SREG;  // save the current interrupt enable flag
  noInterrupts();
  pciSetup(a);
  pciSetup(b);
  encoder.reset();
  SREG = old_sreg;  // restore the previous interrupt enable flag state
}

ISR(PCINT2_vect) {  // pin change interrupt for D0 to D7
  encoder.service();
}
