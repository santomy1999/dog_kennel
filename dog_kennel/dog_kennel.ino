
// This is developed by santo
#include <Servo.h>

#include "HX711.h"

#define calibration_factor 9800.0 //This value is obtained using the SparkFun_HX711_Calibration sketch

#define LOADCELL_DOUT_PIN  3
#define LOADCELL_SCK_PIN  2

#define SPEAKER  5

#define MOTOR_1 11
#define MOTOR_2 12

#define STATE_IDLE                  0
#define STATE_DOG_INSIDE_KENNEL     1
#define STATE_DOG_EXITED            2
#define STATE_POOP_URINE_DETECTED   3

#define SERV 10
//Objects declarations
Servo serv;
HX711 scale;

int current_state = STATE_IDLE;
bool is_urine_present = false;
bool button_pressed = false;
bool food_dispensed = false;

void setup() {

  Serial.begin(9600);
  //  Scale init
  Serial.println("HX711 online");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); // pins set for HX711
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0

  //Motor init
  pinMode(MOTOR_1, OUTPUT);
  pinMode(MOTOR_2, OUTPUT);

  //servo init
  serv.attach(SERV);
  //speaker init
  pinMode(SPEAKER, OUTPUT);
  //Adjust servo position
  serv.write(0);
}

void loop() {

  switch (current_state) {
    case STATE_IDLE :
      check_dog_is_present();
      break;

    case STATE_DOG_INSIDE_KENNEL :
      check_dog_urine();
      if (is_urine_present && !food_dispensed) {
        impl_food_dispense();
        impl_sound_msg();
        food_dispensed = true;
      }
      check_dog_exited();
      break;

    case STATE_DOG_EXITED :
      check_dog_poop_urine();
      break;

    case STATE_POOP_URINE_DETECTED :
      impl_cleaning();
      break;
  }
  if (Serial.available()) //Caliberate to zero if input entered Y
  {
    char temp = Serial.read();
    if (temp == 'y' || temp == 'Y')
    {
      Serial.println("Calibrating");
      scale.tare();
    }
    if (temp == 'f' || temp == 'F') {
      impl_food_dispense();
    }
    if (temp == 'c' || temp == 'C') {
      \
      button_pressed = true;
      impl_cleaning();
    }
  }
  check_input();
}

float get_weight(void)
{
  float weight;
  float avg = 0;
  for (int i = 0; i < 10; i++) {
    avg += (scale.get_units() / 2.2);
  }
  weight = avg / 10;
  //  Serial.print("Current weight on sensor is : ");
  //  Serial.println(weight);
  return weight;
}

void check_dog_is_present(void)
{
  if (get_weight() > 1) {
    Serial.println("Dog detected");
    current_state = STATE_DOG_INSIDE_KENNEL;
  }
}

void check_dog_exited(void)
{
  if (get_weight() < 1) {
    Serial.println("Dog Exited the cage");
    current_state = STATE_DOG_EXITED;
  }
}

void check_dog_poop_urine(void)
{
  delay(5000);
  float current_weight = get_weight();
  if ( (current_weight > .150) && (current_weight < 1) ) {
    Serial.print("Dog poop detected, Weight is : ");
    Serial.println(get_weight());
    impl_food_dispense();
    impl_sound_msg();
    current_state = STATE_POOP_URINE_DETECTED;
  } else if (is_urine_present) {
    current_state = STATE_POOP_URINE_DETECTED;
  } else {
    current_state = STATE_IDLE;
  }
}

void impl_food_dispense(void)
{
  delay(500);
  serv.write(180);
  delay(500);
  serv.write(0);
  Serial.println("Food dispensed");
}

void impl_sound_msg(void)
{
  Serial.println("Playing pre-recorded message");
  digitalWrite(SPEAKER, HIGH);//Pre-recorded message played
  delay(100);
  digitalWrite(SPEAKER, LOW);
  Serial.println("Played pre-recorded message");
  current_state = STATE_IDLE;
}

int get_moisture() {
  int moist_val;
  int avg_moist = 0;
  for (int i = 0; i < 10; i++) {
    moist_val = analogRead(0);
    avg_moist = avg_moist + moist_val;
    delay(50);
  }
  avg_moist /= 10;
  int mappedValue = map(avg_moist, 570, 210, 0, 100);
  Serial.print("Moisture Mapped Value : ");
  Serial.println(mappedValue);
  return mappedValue;
}

void check_dog_urine(void)
{
  //Urine detection
  int moist_val = get_moisture();
  if (moist_val < 0) {
    Serial.print("Mapping ERROR!");
    delay(2000);
  } else if (moist_val >= 80) {
    Serial.println("Moisture Detected ");
    delay(5000);
    if (get_moisture() <= 60) { //waiting for moisture decrease
      Serial.println("Urine Detected");
      is_urine_present = true;
    }
  }
}

void impl_cleaning(void)
{
  if (is_urine_present || button_pressed) {
    Serial.println("Pump:ON");
    digitalWrite(12, HIGH);
    digitalWrite(11, LOW);
    Serial.print("Cleaning the Kennel\n");
    delay(5000);
    digitalWrite(12, LOW);
    digitalWrite(11, LOW);
    Serial.println("Pump :OFF");
    delay(2000);
    is_urine_present = false;
    food_dispensed=false;
  }
  button_pressed = false;
  current_state = STATE_IDLE;
}
void check_input(void) {
  if (Serial.available()) //Caliberate to zero if input entered Y
  {
    char temp = Serial.read();
    if (temp == 'y' || temp == 'Y')
    {
      Serial.println("Calibrating");
      scale.tare();
    }
    if (temp == 'f' || temp == 'F') {
      impl_food_dispense();
    }
    if (temp == 'c' || temp == 'C') {
      impl_cleaning();
    }
  }
}
