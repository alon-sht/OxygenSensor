
#include <SoftwareSerial.h>
#include <Wire.h>               // Library for I2C communication
#include <LiquidCrystal_I2C.h>  // Library for LCD
/*
sensor Pin3 OxySen (TX) to Arduino Pin 10(RX)
sensor Pin4 OxySen (RX) to Arduino Pin 9(TX)
*/
#define led_pin 13
SoftwareSerial mySerial(10, 11);  // RX, TX
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

const byte numChars = 43;
char receivedChars[numChars];
char tempChars[numChars];  // temporary array for use when parsing

float oxygen = 0.0;  // variables to hold the parsed data
float tempo = 0.0;
int pressure = 0;
float percentage = 0.0;
int errorox = 0;

boolean newData = false;

unsigned long currentMillis = 0;
unsigned long previousReadOxySenMillis = 0;
const long ReadOxySenInterval = 500;
bool dotVisible = false;

// Set up countdown timer
// const int COUNTDOWN_SECONDS = 30; // Countdown for 20 seconds
const int COUNTDOWN_SECONDS = 30;
// unsigned long countdownStartTime = 0;

//============

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0,0);
  // lcd.write("Starting");
  delay(100);
  pinMode(led_pin, OUTPUT);
  blink();
}

//============
// void(* resetFunc) (void) = 0; //declare reset function
void resetFunc() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Restarting...");
  delay(1000);
  asm volatile("  jmp 0");
}
void loop() {
  currentMillis = millis();  // capture the latest value of millis()
  if (currentMillis >= COUNTDOWN_SECONDS*1000*2) {
      resetFunc();
  }

  updateReadOxySen();
  // delay(1000);
}

//============
void blink() {

  digitalWrite(led_pin, HIGH);
  delay(1000);
  digitalWrite(led_pin, LOW);
}



void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = 'O';
  char endMarker = '\n';
  int rc;

  while (mySerial.available() > 0 && newData == false) {
    rc = mySerial.read();


    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      } else {

        receivedChars[ndx] = '\0';  // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

//============

void parseData() {  // split the data into its parts

  char* strtokIndx;  // this is used by strtok() as an index
  float tmp_tempo, tmp_percentage;
  //data from sensor send looks like: "O xxxx.x T yxx.x P xxxx % xxx.xx e xxxx\r\n"
  //e.g. "O 0020.1 T +19.3 P 1013 % 020.16 e 0001\r\n"

  // Use strtok() to extract the oxygen value
  strtokIndx = strtok(tempChars, "T");
  if (strtokIndx != NULL) {
    oxygen = atof(strtokIndx);
  }

  // Use strtok() to extract the temperature value
  strtokIndx = strtok(NULL, "P");
  if (strtokIndx != NULL) {
    tmp_tempo = atof(strtokIndx);
  }

  // Use strtok() to extract the pressure value
  strtokIndx = strtok(NULL, "%");
  if (strtokIndx != NULL) {
    pressure = atoi(strtokIndx);
  }

  // Use strtok() to extract the percentage value
  strtokIndx = strtok(NULL, "e");
  if (strtokIndx != NULL) {
    tmp_percentage = atof(strtokIndx);
  }

  // Use strtok() to extract the errorox value
  strtokIndx = strtok(NULL, "\n");
  if (strtokIndx != NULL) {
    errorox = atoi(strtokIndx);
  }

  // Check whether the formatting of the string matches the expected format
  if (tmp_tempo >= 0 && tmp_percentage >= 0 && tmp_percentage <= 100) {
    // If the formatting matches, assign the temporary variables to the real variables
    tempo = tmp_tempo;
    percentage = tmp_percentage;
  }



}

//============

void showParsedData() {
  /*    Serial.print("Oxygen ");
    Serial.println(oxygen);
    Serial.print("Temperature ");
    Serial.println(tempo);
    Serial.print("Pressure ");
    Serial.println(pressure);
    Serial.print("Percent ");
    Serial.println(percentage);
    Serial.print("Error from sensor ");
    Serial.println(errorox);   
*/
  lcd.setCursor(0, 0);
  lcd.print("Oxygen ");
  lcd.print(percentage);
  if (dotVisible) {
    lcd.print("%  |       ");
  } else {
    lcd.print("%  -      ");
  }
  lcd.setCursor(0, 1);
  lcd.print("Temp. ");
  lcd.print(tempo);
  lcd.print((char)223);
  // lcd.print("C ");
    if (dotVisible) {
    lcd.print("C  -       ");
  } else {
    lcd.print("C  |      ");
  }
  if (percentage > 0.1) {         // if value is less than
    digitalWrite(led_pin, HIGH);  // set LED pin to HIGH
  } else {                        // if value is greater than or equal to
    digitalWrite(led_pin, LOW);   // set LED pin to LOW
  }
}

void updateReadOxySen() {
  if (currentMillis - previousReadOxySenMillis >= ReadOxySenInterval) {
    // time is up, so make anew read
    recvWithStartEndMarkers();
    if (newData == true) {
      strcpy(tempChars, receivedChars);
      // this temporary copy is necessary to protect the original data
      //   because strtok() used in parseData() replaces the commas with \0
      parseData();
      showParsedData();
      newData = false;
      dotVisible = !dotVisible;
    }
    // and save the time of change
    previousReadOxySenMillis += ReadOxySenInterval;
    
  }
}
