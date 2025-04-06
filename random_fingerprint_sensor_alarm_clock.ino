#include <LiquidCrystal.h>         // Liquid Crystal Library
#include <Adafruit_Fingerprint.h>  // Adafruit Fingerprint Sensor Library
#include <Wire.h>                  // Only library  required. // Wire Library
#define DS1307_I2C_ADDRESS 0x68    // I2C Address of the DS1307
#define buzzer 9                   // Buzzer digital pin to control

// Alarm Time (24-hour format)
int alarmHour = 10;    // Hour (24-hour format)
int alarmMinute = 10;  // Minute

int selectedFinger = -1;  // store selected finger

// Struct to hold time and date information
struct timeDate {
  uint8_t Seconds;  // 0-59 seconds
  uint8_t Minutes;  // 0-59 minutes
  uint8_t Hours;    // 0-23 hours
  uint8_t Day;      // day of the week day 1= Monday
  uint8_t Date;     // 1-31 (Date)
  uint8_t Month;    // 1-12 (Month)
  uint8_t Year;     //years after 2000 ---> 25 + 2000 = 2025
} td;               //  creates the time date instance ---> Line 49: td = readTimeDate();  // Read current RTC time

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;  // LCD pins
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);                   // creates an instance --> lcd

// Uses SoftwareSerial for the fingerprint sensor communication
SoftwareSerial myNanoSerial(6, 7);  // creats instance.
// pin 6 (RX)--> recieve pin & pin 7 (TX) --> transmit pin for software serial interface

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&myNanoSerial);  // class named finger + configuration to use myNanoSerial for communication
bool fingerprintMatched = false;                                    // set to true when match found
bool alarmTurnedOff = false;                                        // set to true when alarm is turned off after being on

//array of pointers (a pointer stores the memory address of variable)
const char* fingerLabels[] = {
  // name of array: fingerLabels
  "Right thumb",   // fingerprint #1
  "Right index",   // fingerprint #2
  "Right middle",  // fingerprint #3
  "Right ring",    // fingerprint #4
  "Right pinky",   // fingerprint #5
  "Left thumb",    // fingerprint #6
  "Left index",    // fingerprint #7
  "Left middle",   // fingerprint #8
  "Left ring",     // fingerprint #9
  "Left pinky"     // fingerprint #10
};

void setup() {
  Wire.begin();         // Initialize I2C bus session. Necessary for RTC
  td = readTimeDate();  // Read current RTC time
  lcd.begin(16, 2);     // Initialize LCD with 16 columns and 2 rows
  Serial.begin(9600);   // initializes serial comm

  while (!Serial)       // while not serial
    ;
  delay(100);  // delay 100 miliseconds

  // Initialize random seed
  randomSeed(analogRead(0));  // uses electrical noise to generate random numbers
                              // reads voltage on pin A0 and picks on electrical noise

  // Manually set the RTC time to 8th March 2025 12:00:00
  setTimeDate(0, 10, 10, 6, 29, 3, 2025);  // Set RTC time (seconds, minutes, hours) and date (date day, date, month, year)

  pinMode(buzzer, OUTPUT);    // sets buzzer as the output
  digitalWrite(buzzer, LOW);  // Set default buzzer state to LOW
  Serial.println("\n\nAdafruit finger detect test");
  finger.begin(57600);  // baud rate for software serial
  delay(5);             // delay for 5 miliseconds

  if (finger.verifyPassword()) {                  //checks if fingerprint sensor is initalized properly and that communication is established between sensor and nano
    Serial.println("Found fingerprint sensor!");  // Found fingerprint sensor!
    td = readTimeDate();                          // Get current time from RTC    // gets the current time + stores it in td variable
    displayTimeDate(td);                          // Display time on the LCD
  } else {
    Serial.println("Did not find fingerprint sensor :("); // inform that sensor was not found 
    lcd.clear();                                          // clear display
    lcd.print("Sensor NOT FOUND");                        // display "Sensor NOT FOUND"
    return;  // used to exist setup if sensor is not properly intialized
  }

  finger.getTemplateCount();  // displays number of fingerprints stored in the sensor
  if (finger.templateCount == 0) {    // if there are no fingerprint tempaltes,
    Serial.print("No fingerprint data found. Please run the 'enroll' example.");  // serial print instructing user to enroll fingeprrint using enroll example
  } else {
    Serial.println("Waiting for valid finger...");  // serial print "Waiting for valid finger..."
    Serial.print("Sensor contains ");               // prints "Sensor contains"
    Serial.print(finger.templateCount);             // number of fingerprint templates
    Serial.print(" templates");                     // print "templates"
    delay(1000);
  }
}
void loop() {
  td = readTimeDate();  // Read current time from RTC
  displayTimeDate(td);  // Displays time without flickering

  if (td.Hours == alarmHour && td.Minutes == alarmMinute) { // if the current hour and minute are equal to the alarm set hour and minute
    alarmTurnedOff = false;  // Reset the flag when the alarm time is reached

    // Pick a random fingerprint ID to be scanned
    selectedFinger = random(1, 11);               // select a random number between 1 and 10 inclusive
    lcd.clear();                                  // clear the time
    lcd.print("Scan ");                           // Print "Scan: "
    lcd.print(fingerLabels[selectedFinger - 1]);  // displays the finger you are supposed to scan

    while (!fingerprintMatched && !alarmTurnedOff) {  // if the fingerprint match is not found and alarm is not turned off...
      digitalWrite(buzzer, HIGH);                     // sound buzzer for 900 miliseconds
      delay(900);                                      // delay 
      digitalWrite(buzzer, LOW);                       // stop sounding for 900 miliseconds and repeat until valid fingerprint
      delay(900);                                     // delay
      getFingerprintID();                           // checks to see if finger
      delay(100);
    }
    digitalWrite(buzzer, LOW);
    fingerprintMatched = true;
    lcd.clear();
    lcd.print("Good Morning!!");
    delay(2000);
  } else {
    if (fingerprintMatched) {
      alarmTurnedOff = true;  // Set the flag to true when the fingerprint matches
    }
    digitalWrite(buzzer, LOW);
  }
  delay(1000);
}

void setTimeDate(uint8_t s, uint8_t m, uint8_t h, uint8_t d, uint8_t date, uint8_t mon, uint16_t y) {  //  takes parameters for seconds, minutes, etc
  // converts the seconds, months, hours, days, dates, months, years, from binary to BCD
  Wire.beginTransmission(DS1307_I2C_ADDRESS);  // Identify slave device
  Wire.write(0);                               // Set the first register address to fill
  Wire.write(binToBCD(s));                     // Start queueing bytes to transmit
  Wire.write(binToBCD(m));
  Wire.write(binToBCD(h));
  Wire.write(binToBCD(d));
  Wire.write(binToBCD(date));
  Wire.write(binToBCD(mon));
  Wire.write(binToBCD(y % 100));  // Use last two digits of year
  Wire.endTransmission();         // Ship the data to the RTC
}

timeDate readTimeDate() {
  timeDate info;
  Wire.beginTransmission(DS1307_I2C_ADDRESS);  // begins I2C communication with the DS1307 RTC
  Wire.write(0);
  Wire.endTransmission();  // ends I2C communication with the DS1307 RTC
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

  info.Seconds = BCDToBin(Wire.read() & 0x7F);
  info.Minutes = BCDToBin(Wire.read() & 0x7F);
  info.Hours = BCDToBin(Wire.read() & 0x3F);
  info.Day = BCDToBin(Wire.read() & 0x07);
  info.Date = BCDToBin(Wire.read() & 0x3F);
  info.Month = BCDToBin(Wire.read() & 0x1F);
  info.Year = BCDToBin(Wire.read());

  return info;
}

void displayTimeDate(timeDate td) {
  String hourFormat = (td.Hours < 10) ? "0" + String(td.Hours) : String(td.Hours);          // if hour is less than 10, adding leading zero
  String minuteFormat = (td.Minutes < 10) ? "0" + String(td.Minutes) : String(td.Minutes);  // if minute is less than 10, adding leading zero
  String secondFormat = (td.Seconds < 10) ? "0" + String(td.Seconds) : String(td.Seconds);  // if second is less than 10, add leading zero

  lcd.setCursor(0, 0);                                              // set cursor to 0,0 (first row, first column)
  lcd.print("Time: ");                                              // print "Time: " on the liquid crystal display
  lcd.print(hourFormat + ":" + minuteFormat + ":" + secondFormat);  // displays the hour, minute, and second

  // Print time in 24-hour format
  Serial.print("Time: ");      // serial print "Time: " on the liquid crystal display
  Serial.print(hourFormat);    // print the hour on the liquid crystal display
  Serial.print(":");           // print the colon to seperate the hour and minute
  Serial.print(minuteFormat);  // print the minute
  Serial.print(":");
  Serial.print(secondFormat);
  Serial.print(" | Date: ");
  Serial.print(td.Day);  // Day of the week (1-7, 1 = Sunday)
  Serial.print(" ");
  Serial.print(2000 + td.Year);  // Year (formatted to 4 digits)
  Serial.print(" ");
  Serial.print(td.Month);  // Month (1-12)
  Serial.print(" ");
  Serial.println(td.Date);  // Day of the month
}

// Convert normal decimal numbers to binary coded decimal
uint8_t binToBCD(uint8_t val) {
  return (val / 10 << 4) + val % 10;
}

// Convert binary coded decimal to normal decimal numbers
uint8_t BCDToBin(uint8_t val) {
  return (val >> 4) * 10 + (val & 0x0F);
}

uint8_t getFingerprintID() {
  // Check for the selected fingerprint ID
  uint8_t printResult = finger.getImage();
  switch (printResult) {
    case FINGERPRINT_OK:  // If a finger is found, inform that image is taken
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");  // If no finger is detected, inform that no finger is found
      return printResult;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");  // Communication error
      return printResult;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");  // Imaging error
      return printResult;
    default:
      Serial.println("Unknown error");  // Unknown error
      return printResult;
  }

  // Image successfully taken, now process it
  printResult = finger.image2Tz();
  switch (printResult) {
    case FINGERPRINT_OK:  // Image converted successfully
      Serial.println("Image converted");      
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");  // Image too messy
      return printResult;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");  // Could not find fingerprint features
      return printResult;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Invalid image");  // Invalid image
      return printResult;
    default:
      Serial.println("Unknown error");  // Unknown error
      return printResult;
  }

  // Checks if a match is found or not
  printResult = finger.fingerFastSearch();
  if (printResult == FINGERPRINT_OK) {  // if the fingerprint result
    if (finger.fingerID == selectedFinger) {  // the fingerprint match is found
      Serial.println("Found print match!");   // Serial print "Found print match"
      lcd.clear();      // clear display
      delay(500);       // delay 500 miliseconds
      Serial.print("Welcome!"); // Serial print "Welcome!"
      lcd.print("Welcome!");// LCD print "Welcome!"
      fingerprintMatched = true;  // Set flag to true when match is found
      alarmTurnedOff = true;      // Set the flag to true when the fingerprint matches
      lcd.clear();                // clear display
      delay(2000);                // delay 2000 miliseconds
    } else {
      Serial.println("Wrong finger, try again");  // if printResult != selectedFinger
      lcd.setCursor(0, 1);                        // set cursor to first column, second row
      lcd.print("Wrong finger, try again");       // Print "Wrong finger, try again"
    }
  } else if (printResult == FINGERPRINT_NOTFOUND) { // if finger is not found 
    Serial.println("No match found");                // Serial print "No match found"
    lcd.setCursor(0, 1);                            // first row second column
    lcd.print("No match found");                    // Print "No match found"
  } else {
    Serial.println("Communication error");          // Communication error--> an issue besides a match found, not right match, or a finger not found 
  }
  return printResult;                               //capturing + processing print image
  // the result of getFingerprintID function
}
