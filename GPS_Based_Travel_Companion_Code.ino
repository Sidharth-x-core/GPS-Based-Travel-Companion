#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <LiquidCrystal.h>
#include <math.h>

// =====================================================
// GPS CONNECTIONS
// GPS TX -> Arduino Pin 7
// GPS RX -> Arduino Pin 6
// =====================================================
static const int RXPin = 7;
static const int TXPin = 6;
static const uint32_t GPSBaud = 9600;

// =====================================================
// LCD CONNECTIONS
// LiquidCrystal(RS, E, D4, D5, D6, D7)
// =====================================================
LiquidCrystal lcd(2, 4, 9, 10, 11, 12);

TinyGPSPlus gps;
SoftwareSerial gpsSerial(RXPin, TXPin);

// =====================================================
// TIMING
// =====================================================
unsigned long lastLCDUpdate = 0;
unsigned long lastSerialUpdate = 0;
unsigned long tripStartTime = 0;

const unsigned long LCD_INTERVAL = 10000;    // Page changes every 10 sec
const unsigned long SERIAL_INTERVAL = 1000; // Serial update every 1 sec

// =====================================================
// DISPLAY PAGE
// =====================================================
int displayPage = 0;

const int TOTAL_PAGES = 10;

// =====================================================
// TRIP VARIABLES
// =====================================================
double totalDistance = 0.0;   // km

double maxSpeed = 0.0;        // km/h
double minSpeed = 9999.0;     // km/h

double speedSum = 0.0;
unsigned long speedSamples = 0;

// =====================================================
// LOCATION VARIABLES
// =====================================================
double previousLat = 0.0;
double previousLng = 0.0;

bool previousLocationValid = false;
bool tripStarted = false;

// =====================================================
// GPS UPDATE COUNTER
// =====================================================
unsigned long gpsUpdateCount = 0;

// =====================================================
// IST DATE/TIME VARIABLES
// =====================================================
int istHour = 0;
int istMinute = 0;
int istSecond = 0;

int istDay = 0;
int istMonth = 0;
int istYear = 0;

// =====================================================
// MOVING / STOPPED
// =====================================================
const double MOVING_SPEED_THRESHOLD = 1.0; // km/h

bool vehicleMoving = false;

// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(9600);
  gpsSerial.begin(GPSBaud);

  lcd.begin(16, 2);

  // ---------------------------------------------------
  // SEARCHING SCREEN
  // ---------------------------------------------------
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Searching Sat...");

  lcd.setCursor(0, 1);
  lcd.print("Sats: 0");

  Serial.println("================================");
  Serial.println("       GPS TRAVEL MONITOR");
  Serial.println("================================");
  Serial.println("Searching for GPS signal...");
}

// =====================================================
// MAIN LOOP
// =====================================================
void loop() {

  // ===================================================
  // READ GPS DATA
  // ===================================================
  while (gpsSerial.available() > 0) {

    char c = gpsSerial.read();

    if (gps.encode(c)) {

      // Count complete GPS sentences/updates
      gpsUpdateCount++;
    }
  }

  // ===================================================
  // GPS NOT LOCKED
  // ===================================================
  if (!gps.location.isValid()) {

    displayPage = 0;

    if (millis() - lastLCDUpdate >= 1000) {

      lastLCDUpdate = millis();

      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("Searching Sat...");

      lcd.setCursor(0, 1);
      lcd.print("Sats:");

      if (gps.satellites.isValid()) {
        lcd.print(gps.satellites.value());
      }
      else {
        lcd.print("0");
      }
    }
  }

  // ===================================================
  // GPS LOCKED
  // ===================================================
  else {

    // -------------------------------------------------
    // START TRIP
    // -------------------------------------------------
    if (!tripStarted) {

      tripStarted = true;

      tripStartTime = millis();

      previousLat = gps.location.lat();
      previousLng = gps.location.lng();

      previousLocationValid = true;

      Serial.println("GPS LOCKED!");
      Serial.println("Trip Started");
    }

    // -------------------------------------------------
    // IST TIME AND DATE
    // -------------------------------------------------
    calculateIST();

    // -------------------------------------------------
    // DISTANCE
    // -------------------------------------------------
    calculateDistance();

    // -------------------------------------------------
    // SPEED STATISTICS
    // -------------------------------------------------
    calculateSpeedStatistics();

    // -------------------------------------------------
    // MOVING / STOPPED
    // -------------------------------------------------
    calculateMovementStatus();

    // -------------------------------------------------
    // CHANGE LCD PAGE
    // -------------------------------------------------
    if (millis() - lastLCDUpdate >= LCD_INTERVAL) {

      lastLCDUpdate = millis();

      displayPage++;

      if (displayPage > TOTAL_PAGES) {
        displayPage = 1;
      }

      updateDisplay();
    }
  }

  // ===================================================
  // SERIAL MONITOR
  // ===================================================
  if (millis() - lastSerialUpdate >= SERIAL_INTERVAL) {

    lastSerialUpdate = millis();

    printGPSData();
  }
}

// =====================================================
// IST TIME + DATE CALCULATION
// UTC + 5 HOURS 30 MINUTES
// =====================================================
void calculateIST() {

  if (!gps.time.isValid() || !gps.date.isValid()) {
    return;
  }

  // Copy UTC time
  istHour = gps.time.hour();
  istMinute = gps.time.minute();
  istSecond = gps.time.second();

  // Copy UTC date
  istDay = gps.date.day();
  istMonth = gps.date.month();
  istYear = gps.date.year();

  // Add 30 minutes
  istMinute += 30;

  if (istMinute >= 60) {

    istMinute -= 60;
    istHour++;
  }

  // Add 5 hours
  istHour += 5;

  // ---------------------------------------------------
  // NEXT DAY
  // ---------------------------------------------------
  if (istHour >= 24) {

    istHour -= 24;

    istDay++;

    int daysInMonth;

    // 31-day months
    if (istMonth == 1 ||
        istMonth == 3 ||
        istMonth == 5 ||
        istMonth == 7 ||
        istMonth == 8 ||
        istMonth == 10 ||
        istMonth == 12) {

      daysInMonth = 31;
    }

    // 30-day months
    else if (istMonth == 4 ||
             istMonth == 6 ||
             istMonth == 9 ||
             istMonth == 11) {

      daysInMonth = 30;
    }

    // February
    else {

      if ((istYear % 4 == 0 && istYear % 100 != 0) ||
          (istYear % 400 == 0)) {

        daysInMonth = 29;
      }
      else {

        daysInMonth = 28;
      }
    }

    // -------------------------------------------------
    // NEXT MONTH
    // -------------------------------------------------
    if (istDay > daysInMonth) {

      istDay = 1;

      istMonth++;

      // -----------------------------------------------
      // NEXT YEAR
      // -----------------------------------------------
      if (istMonth > 12) {

        istMonth = 1;

        istYear++;
      }
    }
  }
}

// =====================================================
// DISTANCE CALCULATION
// =====================================================
void calculateDistance() {

  if (!gps.location.isUpdated()) {
    return;
  }

  double currentLat = gps.location.lat();
  double currentLng = gps.location.lng();

  if (previousLocationValid) {

    double distance = TinyGPSPlus::distanceBetween(
      previousLat,
      previousLng,
      currentLat,
      currentLng
    );

    // distanceBetween() returns meters

    // Ignore very small GPS fluctuations
    if (distance > 2.0) {

      totalDistance += distance / 1000.0;
    }
  }

  previousLat = currentLat;
  previousLng = currentLng;

  previousLocationValid = true;
}

// =====================================================
// SPEED STATISTICS
// =====================================================
void calculateSpeedStatistics() {

  if (!gps.speed.isUpdated()) {
    return;
  }

  double currentSpeed = gps.speed.kmph();

  // ---------------------------------------------------
  // MAXIMUM SPEED
  // ---------------------------------------------------
  if (currentSpeed > maxSpeed) {

    maxSpeed = currentSpeed;
  }

  // ---------------------------------------------------
  // MINIMUM SPEED
  // ---------------------------------------------------
  if (currentSpeed < minSpeed) {

    minSpeed = currentSpeed;
  }

  // ---------------------------------------------------
  // SAMPLE AVERAGE
  // ---------------------------------------------------
  speedSum += currentSpeed;

  speedSamples++;
}

// =====================================================
// MOVING / STOPPED DETECTION
// =====================================================
void calculateMovementStatus() {

  if (!gps.speed.isValid()) {

    vehicleMoving = false;

    return;
  }

  double currentSpeed = gps.speed.kmph();

  if (currentSpeed >= MOVING_SPEED_THRESHOLD) {

    vehicleMoving = true;
  }
  else {

    vehicleMoving = false;
  }
}

// =====================================================
// TRIP AVERAGE SPEED
// Distance / Time
// =====================================================
double getTripAverageSpeed() {

  if (!tripStarted) {
    return 0.0;
  }

  unsigned long elapsed = millis() - tripStartTime;

  if (elapsed < 1000) {
    return 0.0;
  }

  double hours = elapsed / 3600000.0;

  if (hours <= 0.0) {
    return 0.0;
  }

  return totalDistance / hours;
}

// =====================================================
// LCD DISPLAY
// =====================================================
void updateDisplay() {

  lcd.clear();

  // ===================================================
  // PAGE 1
  // SPEED + SATELLITES
  // ===================================================
  if (displayPage == 1) {

    lcd.setCursor(0, 0);
    lcd.print("Spd:");

    if (gps.speed.isValid()) {

      double speed = gps.speed.kmph();

      if (speed < 10.0) {
        lcd.print(" ");
      }

      lcd.print(speed, 1);
      lcd.print("km/h");
    }
    else {

      lcd.print("N/A");
    }

    lcd.setCursor(0, 1);
    lcd.print("Sats:");

    if (gps.satellites.isValid()) {
      lcd.print(gps.satellites.value());
    }
    else {
      lcd.print("N/A");
    }
  }

  // ===================================================
  // PAGE 2
  // LATITUDE + LONGITUDE
  // ===================================================
  else if (displayPage == 2) {

    lcd.setCursor(0, 0);

    lcd.print("Lat:");
    lcd.print(gps.location.lat(), 4);

    lcd.setCursor(0, 1);

    lcd.print("Lng:");
    lcd.print(gps.location.lng(), 4);
  }

  // ===================================================
  // PAGE 3
  // ALTITUDE + DIRECTION
  // ===================================================
  else if (displayPage == 3) {

    lcd.setCursor(0, 0);

    lcd.print("Alt:");

    if (gps.altitude.isValid()) {

      lcd.print(gps.altitude.meters(), 1);
      lcd.print("m");
    }
    else {

      lcd.print("N/A");
    }

    lcd.setCursor(0, 1);

    lcd.print("Dir:");

    if (gps.course.isValid()) {

      lcd.print(getDirection(gps.course.deg()));
    }
    else {

      lcd.print("N/A");
    }
  }

  // ===================================================
  // PAGE 4
  // IST TIME + DATE
  // ===================================================
  else if (displayPage == 4) {

    lcd.setCursor(0, 0);

    if (gps.time.isValid() && gps.date.isValid()) {

      if (istHour < 10)
        lcd.print("0");

      lcd.print(istHour);
      lcd.print(":");

      if (istMinute < 10)
        lcd.print("0");

      lcd.print(istMinute);
      lcd.print(":");

      if (istSecond < 10)
        lcd.print("0");

      lcd.print(istSecond);

      lcd.print(" IST");
    }
    else {

      lcd.print("Time: N/A");
    }

    lcd.setCursor(0, 1);

    if (gps.date.isValid()) {

      if (istDay < 10)
        lcd.print("0");

      lcd.print(istDay);
      lcd.print("/");

      if (istMonth < 10)
        lcd.print("0");

      lcd.print(istMonth);
      lcd.print("/");

      lcd.print(istYear);
    }
    else {

      lcd.print("Date: N/A");
    }
  }

  // ===================================================
  // PAGE 5
  // DISTANCE + TRIP TIME
  // ===================================================
  else if (displayPage == 5) {

    lcd.setCursor(0, 0);

    lcd.print("Dist:");

    if (totalDistance < 100.0) {

      lcd.print(totalDistance, 1);
      lcd.print("km");
    }
    else {

      lcd.print(totalDistance, 0);
      lcd.print("km");
    }

    lcd.setCursor(0, 1);

    lcd.print("Trip:");

    printTripTimeLCD();
  }

  // ===================================================
  // PAGE 6
  // MAX + SAMPLE AVERAGE SPEED
  // ===================================================
  else if (displayPage == 6) {

    lcd.setCursor(0, 0);

    lcd.print("Max:");

    lcd.print(maxSpeed, 1);
    lcd.print("km/h");

    lcd.setCursor(0, 1);

    lcd.print("Avg:");

    double averageSpeed = 0.0;

    if (speedSamples > 0) {

      averageSpeed = speedSum / speedSamples;
    }

    lcd.print(averageSpeed, 1);
    lcd.print("km/h");
  }

  // ===================================================
  // PAGE 7
  // MOVING / STOPPED
  // ===================================================
  else if (displayPage == 7) {

    lcd.setCursor(0, 0);

    lcd.print("Vehicle:");

    lcd.setCursor(0, 1);

    if (!gps.speed.isValid()) {

      lcd.print("NO SPEED");
    }
    else if (vehicleMoving) {

      lcd.print("MOVING");
    }
    else {

      lcd.print("STOPPED");
    }
  }

  // ===================================================
  // PAGE 8
  // GPS STATUS + SATELLITES
  // ===================================================
  else if (displayPage == 8) {

    lcd.setCursor(0, 0);

    lcd.print("GPS:");

    if (gps.location.isValid()) {

      lcd.print("OK");
    }
    else {

      lcd.print("NO FIX");
    }

    lcd.setCursor(0, 1);

    lcd.print("Sats:");

    if (gps.satellites.isValid()) {

      lcd.print(gps.satellites.value());
    }
    else {

      lcd.print("N/A");
    }
  }

  // ===================================================
  // PAGE 9
  // HEADING + FIX AGE
  // ===================================================
  else if (displayPage == 9) {

    lcd.setCursor(0, 0);

    lcd.print("Head:");

    if (gps.course.isValid()) {

      lcd.print(gps.course.deg(), 0);
      lcd.print((char)223);
    }
    else {

      lcd.print("N/A");
    }

    lcd.setCursor(0, 1);

    lcd.print("Fix:");

    if (gps.location.isValid()) {

      unsigned long age = gps.location.age();

      if (age < 1000) {

        lcd.print(age);
        lcd.print("ms");
      }
      else {

        lcd.print(age / 1000);
        lcd.print("s");
      }
    }
    else {

      lcd.print("N/A");
    }
  }

  // ===================================================
  // PAGE 10
  // TRIP AVERAGE + MINIMUM SPEED
  // ===================================================
  else if (displayPage == 10) {

    double tripAverage = getTripAverageSpeed();

    lcd.setCursor(0, 0);

    lcd.print("TAvg:");
    lcd.print(tripAverage, 1);
    lcd.print("km/h");

    lcd.setCursor(0, 1);

    lcd.print("Min:");

    if (minSpeed == 9999.0) {

      lcd.print("0.0");
    }
    else {

      lcd.print(minSpeed, 1);
    }

    lcd.print("km/h");
  }
}

// =====================================================
// GET DIRECTION
// =====================================================
String getDirection(double heading) {

  if (heading >= 337.5 || heading < 22.5)
    return "N";

  else if (heading >= 22.5 && heading < 67.5)
    return "NE";

  else if (heading >= 67.5 && heading < 112.5)
    return "E";

  else if (heading >= 112.5 && heading < 157.5)
    return "SE";

  else if (heading >= 157.5 && heading < 202.5)
    return "S";

  else if (heading >= 202.5 && heading < 247.5)
    return "SW";

  else if (heading >= 247.5 && heading < 292.5)
    return "W";

  else
    return "NW";
}

// =====================================================
// TRIP TIME ON LCD
// =====================================================
void printTripTimeLCD() {

  unsigned long elapsed = millis() - tripStartTime;

  unsigned long seconds = elapsed / 1000;

  unsigned int hours = seconds / 3600;

  seconds %= 3600;

  unsigned int minutes = seconds / 60;

  seconds %= 60;

  if (hours < 10)
    lcd.print("0");

  lcd.print(hours);
  lcd.print(":");

  if (minutes < 10)
    lcd.print("0");

  lcd.print(minutes);
  lcd.print(":");

  if (seconds < 10)
    lcd.print("0");

  lcd.print(seconds);
}

// =====================================================
// SERIAL MONITOR
// =====================================================
void printGPSData() {

  Serial.println("--------------------------------");

  // ===================================================
  // GPS STATUS
  // ===================================================
  Serial.print("GPS Status: ");

  if (gps.location.isValid())
    Serial.println("OK");
  else
    Serial.println("NO FIX");

  // ===================================================
  // SATELLITES
  // ===================================================
  Serial.print("Satellite Count: ");

  if (gps.satellites.isValid())
    Serial.println(gps.satellites.value());
  else
    Serial.println("N/A");

  // ===================================================
  // LATITUDE
  // ===================================================
  Serial.print("Latitude: ");

  if (gps.location.isValid())
    Serial.println(gps.location.lat(), 6);
  else
    Serial.println("N/A");

  // ===================================================
  // LONGITUDE
  // ===================================================
  Serial.print("Longitude: ");

  if (gps.location.isValid())
    Serial.println(gps.location.lng(), 6);
  else
    Serial.println("N/A");

  // ===================================================
  // SPEED
  // ===================================================
  Serial.print("Speed: ");

  if (gps.speed.isValid())
    Serial.print(gps.speed.kmph(), 2);
  else
    Serial.print("N/A");

  Serial.println(" km/h");

  // ===================================================
  // MOVING / STOPPED
  // ===================================================
  Serial.print("Vehicle Status: ");

  if (!gps.speed.isValid())
    Serial.println("NO SPEED");
  else if (vehicleMoving)
    Serial.println("MOVING");
  else
    Serial.println("STOPPED");

  // ===================================================
  // MAXIMUM SPEED
  // ===================================================
  Serial.print("Maximum Speed: ");
  Serial.print(maxSpeed, 2);
  Serial.println(" km/h");

  // ===================================================
  // MINIMUM SPEED
  // ===================================================
  Serial.print("Minimum Speed: ");

  if (minSpeed == 9999.0)
    Serial.print("0");
  else
    Serial.print(minSpeed, 2);

  Serial.println(" km/h");

  // ===================================================
  // SAMPLE AVERAGE SPEED
  // ===================================================
  Serial.print("Sample Average Speed: ");

  if (speedSamples > 0)
    Serial.print(speedSum / speedSamples, 2);
  else
    Serial.print("0");

  Serial.println(" km/h");

  // ===================================================
  // TRIP AVERAGE SPEED
  // ===================================================
  Serial.print("Trip Average Speed: ");
  Serial.print(getTripAverageSpeed(), 2);
  Serial.println(" km/h");

  // ===================================================
  // ALTITUDE
  // ===================================================
  Serial.print("Altitude: ");

  if (gps.altitude.isValid())
    Serial.print(gps.altitude.meters(), 2);
  else
    Serial.print("N/A");

  Serial.println(" m");

  // ===================================================
  // DIRECTION
  // ===================================================
  Serial.print("Direction: ");

  if (gps.course.isValid())
    Serial.println(getDirection(gps.course.deg()));
  else
    Serial.println("N/A");

  // ===================================================
  // HEADING
  // ===================================================
  Serial.print("Heading: ");

  if (gps.course.isValid())
    Serial.print(gps.course.deg(), 2);
  else
    Serial.print("N/A");

  Serial.println(" deg");

  // ===================================================
  // GPS FIX AGE
  // ===================================================
  Serial.print("GPS Fix Age: ");

  if (gps.location.isValid()) {

    unsigned long age = gps.location.age();

    if (age < 1000) {

      Serial.print(age);
      Serial.println(" ms");
    }
    else {

      Serial.print(age / 1000.0, 2);
      Serial.println(" sec");
    }
  }
  else {

    Serial.println("N/A");
  }

  // ===================================================
  // DISTANCE
  // ===================================================
  Serial.print("Distance Travelled: ");
  Serial.print(totalDistance, 3);
  Serial.println(" km");

  // ===================================================
  // TRIP TIME
  // ===================================================
  unsigned long elapsed = 0;

  if (tripStarted) {
    elapsed = millis() - tripStartTime;
  }

  unsigned long seconds = elapsed / 1000;

  unsigned int hours = seconds / 3600;

  seconds %= 3600;

  unsigned int minutes = seconds / 60;

  seconds %= 60;

  Serial.print("Trip Time: ");

  if (hours < 10)
    Serial.print("0");

  Serial.print(hours);
  Serial.print(":");

  if (minutes < 10)
    Serial.print("0");

  Serial.print(minutes);
  Serial.print(":");

  if (seconds < 10)
    Serial.print("0");

  Serial.println(seconds);

  // ===================================================
  // GPS UPDATE COUNT
  // ===================================================
  Serial.print("GPS Update Count: ");
  Serial.println(gpsUpdateCount);

  // ===================================================
  // GPS UTC TIME
  // ===================================================
  Serial.print("GPS Time (UTC): ");

  if (gps.time.isValid()) {

    if (gps.time.hour() < 10)
      Serial.print("0");

    Serial.print(gps.time.hour());
    Serial.print(":");

    if (gps.time.minute() < 10)
      Serial.print("0");

    Serial.print(gps.time.minute());
    Serial.print(":");

    if (gps.time.second() < 10)
      Serial.print("0");

    Serial.println(gps.time.second());
  }
  else {

    Serial.println("N/A");
  }

  // ===================================================
  // IST TIME
  // ===================================================
  Serial.print("IST Time: ");

  if (gps.time.isValid() && gps.date.isValid()) {

    if (istHour < 10)
      Serial.print("0");

    Serial.print(istHour);
    Serial.print(":");

    if (istMinute < 10)
      Serial.print("0");

    Serial.print(istMinute);
    Serial.print(":");

    if (istSecond < 10)
      Serial.print("0");

    Serial.println(istSecond);
  }
  else {

    Serial.println("N/A");
  }

  // ===================================================
  // IST DATE
  // ===================================================
  Serial.print("IST Date: ");

  if (gps.date.isValid()) {

    if (istDay < 10)
      Serial.print("0");

    Serial.print(istDay);
    Serial.print("/");

    if (istMonth < 10)
      Serial.print("0");

    Serial.print(istMonth);
    Serial.print("/");

    Serial.println(istYear);
  }
  else {

    Serial.println("N/A");
  }

  // ===================================================
  // GPS CHARACTER / SENTENCE STATS
  // ===================================================
  Serial.print("GPS Characters Processed: ");
  Serial.println(gps.charsProcessed());

  Serial.print("GPS Failed Checksum: ");
  Serial.println(gps.failedChecksum());

  Serial.println("--------------------------------");
}