#include <SoftwareSerial.h>
#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
#include <TM1637Display.h>

// // Define the connections pins
// #define CLK 3
// #define DIO 4
// // Create a display object of type TM1637Display
// TM1637Display display = TM1637Display(CLK, DIO);
// // Create an array that turns all segments ON
// const uint8_t allON[] = {0xff, 0xff, 0xff, 0xff};

// // Create an array that turns all segments OFF
// const uint8_t allOFF[] = {0x00, 0x00, 0x00, 0x00};
/*
sensor Pin3 OxySen (TX) to Arduino Pin 10(RX)
sensor Pin4 OxySen (RX) to Arduino Pin 11(TX)
*/

SoftwareSerial mySerial(10, 11); // RX, TX
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

const byte numChars = 43;
char receivedChars[numChars];
char tempChars[numChars];     // temporary array for use when parsing
      
float oxygen = 0.0;  // variables to hold the parsed data
float tempo = 0.0;
int pressure = 0;
float percentage = 0.0;
int errorox = 0;

boolean newData = false;

unsigned long currentMillis = 0;
unsigned long previousReadOxySenMillis = 0;
const long ReadOxySenInterval = 1000;



// Set up countdown timer
const int COUNTDOWN_SECONDS = 60; // Countdown for 60 seconds



//============

void setup() {
    Serial.begin(9600);
    mySerial.begin(9600);
    lcd.init();
    lcd.backlight();
    // display.setBrightness(5);
    // display.setSegments(allON);
    // delay(2000);
	// display.clear();
}

//============
void(* resetFunc) (void) = 0; //declare reset function

// =================

void loop() {
    currentMillis = millis();  // capture the latest value of millis()

    if (currentMillis >= COUNTDOWN_SECONDS*1000) { // Update once per second
        lcd.setCursor(0,0);
        lcd.print("Restarting...  ");
        lcd.setCursor(0,1);
        lcd.print("(Every ");lcd.print(COUNTDOWN_SECONDS);lcd.print(" sec)");
        delay(4000);
        resetFunc(); // Reboot the Arduino
    }

    updateReadOxySen();
}

//============

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = 'O';
    //char endMarker = 'e';
    char endMarker = '\n';
    char rc;

    while (mySerial.available() > 0 && newData == false) {
        rc = mySerial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
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

void parseData() {      // split the data into its parts

    char * strtokIndx; // this is used by strtok() as an index

    //data from sensor send looks like: "O xxxx.x T yxx.x P xxxx % xxx.xx e xxxx\r\n"
    //e.g. "O 0020.1 T +19.3 P 1013 % 020.16 e 0001\r\n" 

    strtokIndx = strtok(tempChars,"T");      // get the first part - the string
    oxygen = atof(strtokIndx); // copy it to oxygen
    
    strtokIndx = strtok(NULL, "P"); // this continues where the previous call left off
    tempo = atof(strtokIndx);     // convert this part to a float

    strtokIndx = strtok(NULL, "%"); // this continues where the previous call left off
    pressure = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, "e"); // this continues where the previous call left off
    percentage = atof(strtokIndx);     // convert this part to a float

    strtokIndx = strtok(NULL, "\n"); // this continues where the previous call left off
    errorox = atoi(strtokIndx);     // convert this part to an integer
    
}

//============

void showParsedData() {
    Serial.print("Oxygen ");
    Serial.println(oxygen);
    Serial.print("TemperatureO ");
    Serial.println(tempo);
    Serial.print("Pressure ");
    Serial.println(pressure);
    Serial.print("Percent ");
    Serial.println(percentage); 
    Serial.print("Error from sensor ");
    Serial.println(errorox);   
    Serial.println("");   
    lcd.setCursor(0,0);
    lcd.print("Oxygen ");lcd.print(percentage); lcd.print("%");
    lcd.setCursor(0,1);
    lcd.print("Temp. ");lcd.print(tempo); lcd.print((char)223); lcd.print("C ");
    // display.showNumberDec(percentage);	
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
        }           
          // and save the time of change
       previousReadOxySenMillis += ReadOxySenInterval;
    }
}