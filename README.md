# GPS-Based Travel Companion - Real-Time Navigation and Trip Monitoring System

A compact GPS-based travel companion - Real-Time Navigation and Trip Monitoring System  built using an **Arduino Nano**, **u-blox NEO-6M GPS receiver module**, and a **16x2 character LCD**.

The Arduino reads GPS NMEA data through `SoftwareSerial`, decodes it with `TinyGPS++`, and displays travel information on the LCD. Detailed GPS and trip data are also printed to the Arduino IDE Serial Monitor.

## Features

- Current GPS speed (km/h)
- Satellite count
- Latitude and longitude
- Altitude
- Direction of travel (N, NE, E, SE, S, SW, W, NW)
- Heading in degrees
- Indian Standard Time (IST) and date
- Distance travelled
- Trip duration
- Maximum, minimum, sample-average, and trip-average speed
- Moving/stopped status
- GPS status and GPS fix age
- GPS update count and GPS diagnostic information
- Ten automatically rotating LCD pages
- Serial Monitor output every second
- No separate wheel-speed, distance, or compass sensor required

## Hardware Components

| Component | Purpose |
|---|---|
| Arduino Nano | Main controller |
| u-blox NEO-6M GPS module | Provides position, speed, altitude, course, date, and UTC time |
| 16x2 parallel LCD | Displays information |
| 2S LiPo battery (7.4 V nominal) | Portable power source |
| Perforated board | Soldered, PCB-like assembly |
| Enclosure | Protects the assembled circuit |

## Pin Connections

### GPS module -> Arduino Nano

| GPS module pin | Arduino Nano pin | Description |
|---|---|---|
| TX | D7 | GPS data to Arduino SoftwareSerial RX |
| RX | D6 | Arduino SoftwareSerial TX to GPS |
| VCC | 5V* | Power |
| GND | GND | Common ground |

### 16x2 LCD -> Arduino Nano

The source code uses:

```cpp
LiquidCrystal lcd(2, 4, 9, 10, 11, 12);
```

| LCD pin | Arduino Nano pin |
|---|---|
| RS | D2 |
| EN | D4 |
| D4 | D9 |
| D5 | D10 |
| D6 | D11 |
| D7 | D12 |
| VSS | GND |
| VDD | 5V |
| RW | GND |

Use a suitable contrast circuit/potentiometer according to your LCD module and wiring diagram.

**Power note:** Check the exact NEO-6M breakout-board specifications before connecting VCC. Some boards accept 5 V at their input because they include a regulator; others may require a different supply. A 2S LiPo battery is approximately 7.4 V nominal and 8.4 V fully charged. Do not connect it directly to the Arduino 5V pin. Confirm that your specific Nano board supports the chosen VIN supply and observe correct battery polarity.

## LCD Pages

The program cycles through ten pages, changing pages every **10 seconds**.

| Page | Information |
|---:|---|
| 1 | Speed and satellite count |
| 2 | Latitude and longitude |
| 3 | Altitude and direction |
| 4 | IST time and date |
| 5 | Distance travelled and trip time |
| 6 | Maximum speed and sample-average speed |
| 7 | Moving/stopped status |
| 8 | GPS status and satellite count |
| 9 | Heading and GPS fix age |
| 10 | Trip-average speed and minimum speed |

When the GPS location is not valid, the LCD displays a GPS-searching screen.

## Software and Libraries

Required Arduino libraries:

- **TinyGPS++** - parses GPS NMEA sentences
- **SoftwareSerial** - serial communication on digital pins D7 and D6
- **LiquidCrystal** - controls the parallel LCD
- **math.h** - standard math header; typically supplied with the toolchain

`SoftwareSerial` and `LiquidCrystal` are normally included with the relevant Arduino AVR board package. Install **TinyGPS++** from the Arduino IDE Library Manager if it is not already installed.

See [`LIBRARIES.md`](LIBRARIES.md) for setup notes.

## Main Settings

```cpp
static const int RXPin = 7;
static const int TXPin = 6;
static const uint32_t GPSBaud = 9600;

LiquidCrystal lcd(2, 4, 9, 10, 11, 12);

const unsigned long LCD_INTERVAL = 10000;
const unsigned long SERIAL_INTERVAL = 1000;

const double MOVING_SPEED_THRESHOLD = 2.0;
```

- GPS serial baud rate: **9600**
- LCD page interval: **10 seconds**
- Serial Monitor interval: **1 second**
- Moving threshold: **2 km/h**

## How It Works

1. The GPS module receives signals from visible satellites.
2. It sends NMEA data to the Arduino Nano.
3. `SoftwareSerial` receives the serial stream.
4. `TinyGPS++` decodes the GPS fields.
5. The program calculates distance and speed statistics, derives moving/stopped status, and converts UTC time to IST.
6. The LCD rotates through the ten information pages.
7. The Serial Monitor prints detailed readings and GPS diagnostics every second.

### Distance travelled

The program uses `TinyGPSPlus::distanceBetween()` to calculate the distance between successive valid GPS positions. The result is converted from metres to kilometres. Movement of 2 metres or less between successive positions is ignored to reduce small position fluctuations; this does not eliminate all GPS drift.

### Moving/stopped status

The program compares GPS speed with the `MOVING_SPEED_THRESHOLD` value:

- Speed at or above 2 km/h: `MOVING`
- Speed below 2 km/h: `STOPPED`

This is an estimate based on GPS speed and may fluctuate when reception is poor or speed is close to the threshold.

### Average speed values

- **Sample-average speed:** average of the GPS speed samples collected by the program.
- **Trip-average speed:** total recorded distance divided by elapsed trip time.

These are different calculations and may produce different values.

### IST date and time

GPS time is supplied in UTC. The program adds **5 hours and 30 minutes** to obtain IST and adjusts the date when the conversion crosses midnight, including month/year rollover and leap years.

## Uploading the Program

1. Install the Arduino IDE.
2. Install the required libraries.
3. Open the `.ino` file in the `src/` folder (or your actual source-code folder).
4. Connect the Arduino Nano by USB.
5. Select **Arduino Nano**, the appropriate processor option, and the correct COM port.
6. Upload the sketch.
7. Open Serial Monitor at **9600 baud**.
8. Place the GPS antenna/module with a clear view of the sky and allow time for a valid fix. The first fix may take longer, especially outdoors after power-up.

## Serial Monitor

The sketch reports GPS status, satellite count, latitude, longitude, speed, moving/stopped status, maximum/minimum/sample-average/trip-average speed, altitude, direction, heading, fix age, distance, trip duration, update count, UTC time, IST time/date, characters processed, and checksum failures.

## Limitations

- GPS accuracy depends on satellite visibility, antenna quality, and signal conditions.
- Direction/course is most meaningful while the vehicle is moving.
- GPS altitude is generally less accurate than horizontal position.
- Distance calculations can be affected by GPS drift.
- Moving/stopped status can fluctuate around the selected threshold.
- The system does not measure wind speed.
- The project does not use dedicated wheel-speed, distance, or magnetic-heading sensors.

## Possible Future Improvements

- SD-card trip logging
- Bluetooth or Wi-Fi data transmission
- Smartphone or web dashboard
- Trip reset and trip-history storage
- Battery-voltage and low-battery monitoring
- Improved GPS filtering
- Map-based route visualization
- A larger display

## Project Status

**Working prototype** - GPS-based travel data display, trip calculations, and Serial Monitor diagnostics.

## Author

**Sidharth M Suraj**  
GitHub: `https://github.com/Sidharth-x-core`

## Acknowledgements

- Arduino ecosystem
- TinyGPS++ library
- u-blox GPS technology
- Arduino open-source community

## Topics

`arduino-nano` `neo-6m` `gps` `tinygps-plus-plus` `gps-speedometer` `travel-monitor` `embedded-systems` `16x2-lcd` `arduino-project` `vehicle-monitoring`
