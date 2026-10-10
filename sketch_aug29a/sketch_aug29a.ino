#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <qrcode.h>


// ================================================================
// PUREXAI WATER QUALITY MONITOR
// ESP32 + TDS + TURBIDITY + DS18B20
// OLED QR + LOCAL WEB SERVER + FASTAPI BACKEND
// + TELEGRAM ALERT NOTIFICATIONS
// ================================================================


// ================================================================
// ROUTER WIFI
// ================================================================

const char* WIFI_SSID = "wifiname";
const char* WIFI_PASSWORD = "wifipassword";


// ================================================================
// ESP32 ACCESS POINT
// ================================================================

const char* AP_SSID = "PUREXAI_WATER";
const char* AP_PASSWORD = "PUREXAI123";


const char* SERVER_URL =
  "http://192.168.0.7:8000/api/sensor-data";


// ================================================================
// DEVICE ID
// ================================================================

const char* DEVICE_ID =
  "esp32-001";


// ================================================================
// TELEGRAM BOT
// ================================================================
//
// IMPORTANT:
// Replace the values below with your Telegram Bot Token
// and Telegram Chat ID.
//
// Example:
//
// const char* TELEGRAM_BOT_TOKEN =
//   "123456789:ABCxxxxxxxxxxxxxxxx";
//
// const char* TELEGRAM_CHAT_ID =
//   "123456789";
//
// ================================================================

const char* TELEGRAM_BOT_TOKEN =
  "000000000";

const char* TELEGRAM_CHAT_ID =
  "000000000";


// Telegram connection status

bool telegramHealthy = false;


// Used to prevent repeated Telegram alerts

String lastTelegramAlert = "";


// ================================================================
// ESP32 LOCAL WEB SERVER
// ================================================================

WebServer server(80);


// ================================================================
// GPIO
// ================================================================

// TDS
#define TDS_PIN 34

// Turbidity
#define TURBIDITY_PIN 35

// DS18B20
#define DS18B20_PIN 19

// LEDs
#define RED_LED_PIN     25
#define YELLOW_LED_PIN  26
#define GREEN_LED_PIN   27

// Buzzer
#define BUZZER_PIN 4


// ================================================================
// OLED SPI
// ================================================================

#define OLED_SCK   18
#define OLED_MOSI  23
#define OLED_RST   2
#define OLED_DC    15
#define OLED_CS    5


// ================================================================
// OLED
// ================================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &SPI,
  OLED_DC,
  OLED_RST,
  OLED_CS
);


// ================================================================
// DS18B20
// ================================================================

OneWire oneWire(DS18B20_PIN);

DallasTemperature ds18b20(&oneWire);


// ================================================================
// WATER QUALITY THRESHOLDS
// ================================================================

#define TDS_SAFE_MAX 300.0
#define TDS_WARNING_MAX 500.0

#define TURBIDITY_SAFE_MAX 5.0
#define TURBIDITY_WARNING_MAX 10.0


// ================================================================
// TDS CALIBRATION
// ================================================================

#define TDS_CALIBRATION_FACTOR 1.0


// ================================================================
// TURBIDITY CALIBRATION
// ================================================================

#define TURBIDITY_CLEAR_VOLTAGE 1.5626
#define TURBIDITY_CLEAR_NTU 0.0

#define TURBIDITY_MUDDY_VOLTAGE 0.8470
#define TURBIDITY_MUDDY_NTU 74.28


// ================================================================
// SENSOR SETTINGS
// ================================================================

#define ADC_SAMPLES 20


// ================================================================
// TIMING
// ================================================================

#define SENSOR_READ_INTERVAL       2000
#define SERVER_SEND_INTERVAL       5000
#define WIFI_RECONNECT_INTERVAL    10000
#define OLED_UPDATE_INTERVAL       2000


// ================================================================
// QR CODE
// ================================================================

#define QR_VERSION 3

uint8_t qrData[1024];

QRCode qrcode;


// ================================================================
// SENSOR VARIABLES
// ================================================================

float temperatureC = 0.0;

float tdsPPM = 0.0;

float turbidityNTU = 0.0;


// ================================================================
// RAW SENSOR DATA
// ================================================================

float tdsVoltage = 0.0;

float turbidityVoltage = 0.0;

int tdsRaw = 0;

int turbidityRaw = 0;


// ================================================================
// WATER STATUS
// ================================================================

String waterStatus = "STARTING";

String alertMessage = "System starting";


// ================================================================
// SENSOR HEALTH
// ================================================================

bool tdsHealthy = false;

bool turbidityHealthy = false;

bool temperatureHealthy = false;

bool wifiHealthy = false;

bool backendHealthy = false;

bool allSensorsHealthy = false;


// ================================================================
// TIMERS
// ================================================================

unsigned long lastSensorRead = 0;

unsigned long lastServerSend = 0;

unsigned long lastWiFiCheck = 0;

unsigned long lastOLEDUpdate = 0;


// ================================================================
// BUZZER
// ================================================================

unsigned long buzzerTimer = 0;

bool buzzerState = false;


// ================================================================
// FUNCTION DECLARATIONS
// ================================================================

void setupWiFi();

void connectStationWiFi();

void startAccessPoint();

void setupWebServer();

void handleRoot();

void handleData();

void handleNotFound();

void readAllSensors();

void readTemperature();

void readTDS();

void readTurbidity();

int readAverageADC(int pin);

void checkSensorHealth();

void determineWaterStatus();

void updateLEDs();

void handleBuzzer();

void displayQRCode();

void sendDataToServer();

String createJSONPayload();

String createBackendPayload();

String createQRText();

void printSerialData();

float mapFloat(
  float x,
  float in_min,
  float in_max,
  float out_min,
  float out_max
);


// ================================================================
// TELEGRAM FUNCTION DECLARATIONS
// ================================================================

void sendTelegramMessage(String message);

void checkTelegramAlert();


// ================================================================
// SETUP
// ================================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("==============================================");
  Serial.println("       PUREXAI WATER QUALITY MONITOR");
  Serial.println("==============================================");


  // ==============================================================
  // LED
  // ==============================================================

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);


  // ==============================================================
  // BUZZER
  // ==============================================================

  pinMode(BUZZER_PIN, OUTPUT);


  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);


  // ==============================================================
  // ADC
  // ==============================================================

  analogReadResolution(12);

  analogSetPinAttenuation(
    TDS_PIN,
    ADC_11db
  );

  analogSetPinAttenuation(
    TURBIDITY_PIN,
    ADC_11db
  );


  // ==============================================================
  // DS18B20
  // ==============================================================

  ds18b20.begin();

  Serial.print("DS18B20 devices found: ");

  Serial.println(
    ds18b20.getDeviceCount()
  );


  // ==============================================================
  // OLED SPI
  // ==============================================================

  SPI.begin(
    OLED_SCK,
    -1,
    OLED_MOSI,
    OLED_CS
  );


  // ==============================================================
  // OLED
  // ==============================================================

  if (!display.begin(
        SSD1306_SWITCHCAPVCC
      )) {

    Serial.println("OLED FAILED!");

    while (true) {
      delay(1000);
    }
  }


  Serial.println("OLED OK");


  // ==============================================================
  // START SCREEN
  // ==============================================================

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println("PUREXAI");

  display.println();

  display.println("Water Quality");

  display.println("Monitor");

  display.println();

  display.println("Starting...");

  display.display();

  delay(2000);


  // ==============================================================
  // WIFI
  // ==============================================================

  setupWiFi();


  // ==============================================================
  // WEB SERVER
  // ==============================================================

  setupWebServer();


  // ==============================================================
  // LED TEST
  // ==============================================================

  digitalWrite(RED_LED_PIN, HIGH);
  delay(300);
  digitalWrite(RED_LED_PIN, LOW);

  digitalWrite(YELLOW_LED_PIN, HIGH);
  delay(300);
  digitalWrite(YELLOW_LED_PIN, LOW);

  digitalWrite(GREEN_LED_PIN, HIGH);
  delay(300);
  digitalWrite(GREEN_LED_PIN, LOW);


  // ==============================================================
  // INITIAL SENSOR READING
  // ==============================================================

  readAllSensors();

  checkSensorHealth();

  determineWaterStatus();

  updateLEDs();


  // ==============================================================
  // SHOW QR
  // ==============================================================

  displayQRCode();


  // ==============================================================
  // STARTUP INFORMATION
  // ==============================================================

  Serial.println();
  Serial.println("==============================================");
  Serial.println("SYSTEM READY");
  Serial.println("==============================================");

  Serial.print("Device ID: ");
  Serial.println(DEVICE_ID);

  Serial.print("ESP32 AP WiFi: ");
  Serial.println(AP_SSID);

  Serial.print("AP Password: ");
  Serial.println(AP_PASSWORD);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("Router IP: ");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("NOT CONNECTED");
  }

  Serial.println();

  Serial.println("FASTAPI SERVER:");
  Serial.println(SERVER_URL);

  Serial.println();

  Serial.println("TELEGRAM ALERT:");
  Serial.println("UNSAFE water + SENSOR ERROR notifications enabled");

  Serial.println();

  Serial.println("PHONE STEPS:");
  Serial.println("1. Connect to PUREXAI_WATER");
  Serial.println("2. Password: PUREXAI123");
  Serial.println("3. Open http://192.168.4.1");
  Serial.println("==============================================");
}


// ================================================================
// MAIN LOOP
// ================================================================

void loop() {

  unsigned long currentMillis = millis();


  // ==============================================================
  // WEB SERVER
  // ==============================================================

  server.handleClient();


  // ==============================================================
  // SENSOR READING
  // ==============================================================

  if (
    currentMillis - lastSensorRead
    >= SENSOR_READ_INTERVAL
  ) {

    lastSensorRead = currentMillis;

    readAllSensors();

    checkSensorHealth();

    determineWaterStatus();

    updateLEDs();

    // Telegram alert check
    checkTelegramAlert();

    printSerialData();
  }


  // ==============================================================
  // WIFI CHECK
  // ==============================================================

  if (
    currentMillis - lastWiFiCheck
    >= WIFI_RECONNECT_INTERVAL
  ) {

    lastWiFiCheck = currentMillis;


    if (
      WiFi.status()
      == WL_CONNECTED
    ) {

      wifiHealthy = true;

    }

    else {

      wifiHealthy = false;

      backendHealthy = false;

      telegramHealthy = false;

      Serial.println();
      Serial.println("Router WiFi disconnected!");
      Serial.println("Attempting reconnect...");

      WiFi.disconnect();

      WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
      );
    }
  }


  // ==============================================================
  // SERVER SEND
  // ==============================================================

  if (
    currentMillis - lastServerSend
    >= SERVER_SEND_INTERVAL
  ) {

    lastServerSend = currentMillis;


    if (
      WiFi.status()
      == WL_CONNECTED
    ) {

      sendDataToServer();

    }

    else {

      backendHealthy = false;

      Serial.println(
        "Backend not contacted - WiFi offline"
      );
    }
  }


  // ==============================================================
  // OLED
  // ==============================================================

  if (
    currentMillis - lastOLEDUpdate
    >= OLED_UPDATE_INTERVAL
  ) {

    lastOLEDUpdate = currentMillis;

    displayQRCode();
  }


  // ==============================================================
  // BUZZER
  // ==============================================================

  handleBuzzer();
}


// ================================================================
// WIFI SETUP
// ================================================================

void setupWiFi() {

  Serial.println();
  Serial.println("Starting WiFi...");


  WiFi.mode(WIFI_AP_STA);


  // ==============================================================
  // ESP32 ACCESS POINT
  // ==============================================================

  startAccessPoint();


  // ==============================================================
  // ROUTER WIFI
  // ==============================================================

  connectStationWiFi();
}


// ================================================================
// ACCESS POINT
// ================================================================

void startAccessPoint() {

  Serial.println();
  Serial.println("Starting ESP32 Access Point...");


  bool result =
    WiFi.softAP(
      AP_SSID,
      AP_PASSWORD
    );


  if (!result) {

    Serial.println(
      "ESP32 AP FAILED!"
    );

  }

  else {

    Serial.println(
      "ESP32 AP STARTED"
    );

    Serial.print(
      "AP SSID: "
    );

    Serial.println(
      AP_SSID
    );

    Serial.print(
      "AP IP: "
    );

    Serial.println(
      WiFi.softAPIP()
    );
  }
}


// ================================================================
// CONNECT ROUTER WIFI
// ================================================================

void connectStationWiFi() {

  Serial.println();

  Serial.print(
    "Connecting to router: "
  );

  Serial.println(
    WIFI_SSID
  );


  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  int attempts = 0;


  while (
    WiFi.status()
    != WL_CONNECTED
    &&
    attempts < 20
  ) {

    delay(500);

    Serial.print(".");

    attempts++;
  }


  Serial.println();


  if (
    WiFi.status()
    == WL_CONNECTED
  ) {

    wifiHealthy = true;

    Serial.println(
      "ROUTER WIFI CONNECTED"
    );

    Serial.print(
      "ESP32 Router IP: "
    );

    Serial.println(
      WiFi.localIP()
    );

    Serial.print(
      "RSSI: "
    );

    Serial.print(
      WiFi.RSSI()
    );

    Serial.println(
      " dBm"
    );

  }

  else {

    wifiHealthy = false;

    Serial.println(
      "ROUTER WIFI NOT CONNECTED"
    );

    Serial.println(
      "AP WiFi will still work."
    );
  }
}


// ================================================================
// WEB SERVER
// ================================================================

void setupWebServer() {

  server.on(
    "/",
    handleRoot
  );


  server.on(
    "/data",
    handleData
  );


  server.onNotFound(
    handleNotFound
  );


  server.begin();


  Serial.println(
    "WEB SERVER STARTED"
  );
}


// ================================================================
// MAIN WEBPAGE
// ================================================================

void handleRoot() {

  String html = R"rawliteral(
<!DOCTYPE html>
<html>

<head>

<meta name="viewport"
content="width=device-width, initial-scale=1">

<title>PUREXAI Water Quality</title>

<style>

body {
  font-family: Arial, sans-serif;
  background: #07111f;
  color: white;
  margin: 0;
  padding: 20px;
}

.container {
  max-width: 600px;
  margin: auto;
}

.header {
  text-align: center;
  margin-bottom: 20px;
}

.header h1 {
  color: #00d9ff;
  margin-bottom: 5px;
}

.header p {
  color: #9fb3c8;
}

.status {
  text-align: center;
  padding: 18px;
  border-radius: 15px;
  margin-bottom: 20px;
  font-size: 28px;
  font-weight: bold;
}

.card {
  background: #101d2e;
  border-radius: 15px;
  padding: 18px;
  margin-bottom: 14px;
  box-shadow: 0 5px 20px rgba(0,0,0,0.3);
}

.label {
  color: #9fb3c8;
  font-size: 14px;
}

.value {
  font-size: 30px;
  font-weight: bold;
  margin-top: 5px;
}

.safe {
  background: #0b6338;
}

.warning {
  background: #9a6800;
}

.unsafe {
  background: #9b2020;
}

.error {
  background: #551c75;
}

.grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.footer {
  text-align: center;
  color: #71859a;
  margin-top: 20px;
  font-size: 13px;
}

.online {
  color: #00ff88;
}

.offline {
  color: #ff5555;
}

</style>

</head>

<body>

<div class="container">

<div class="header">

<h1>PUREXAI</h1>

<p>Water Quality Monitoring System</p>

</div>

<div id="status"
class="status">

Loading...

</div>

<div class="grid">

<div class="card">

<div class="label">
Temperature
</div>

<div id="temperature"
class="value">
-- °C
</div>

</div>


<div class="card">

<div class="label">
TDS
</div>

<div id="tds"
class="value">
-- ppm
</div>

</div>


<div class="card">

<div class="label">
Turbidity
</div>

<div id="turbidity"
class="value">
-- NTU
</div>

</div>


<div class="card">

<div class="label">
Device
</div>

<div id="device"
class="value"
style="font-size:16px">
--
</div>

</div>

</div>


<div class="card">

<div class="label">
Alert
</div>

<div id="alert"
class="value"
style="font-size:20px">
--
</div>

</div>


<div class="card">

<div class="label">
Sensor Health
</div>

<p>
Temperature:
<span id="tempHealth">
--
</span>
</p>

<p>
TDS:
<span id="tdsHealth">
--
</span>
</p>

<p>
Turbidity:
<span id="turbHealth">
--
</span>
</p>

<p>
WiFi:
<span id="wifiHealth">
--
</span>
</p>

<p>
Backend:
<span id="backendHealth">
--
</span>
</p>

</div>


<div class="footer">

Live data updates every 2 seconds

<br>

PUREXAI ESP32 Local Web Server

</div>

</div>


<script>

function healthText(value) {

  if (value) {
    return '<span class="online">OK</span>';
  }

  return '<span class="offline">ERROR</span>';
}


function updateData() {

  fetch('/data')

  .then(response => response.json())

  .then(data => {

    document.getElementById(
      'temperature'
    ).innerHTML =
      Number(data.temperature).toFixed(1)
      + ' °C';


    document.getElementById(
      'tds'
    ).innerHTML =
      Number(data.tds).toFixed(1)
      + ' ppm';


    document.getElementById(
      'turbidity'
    ).innerHTML =
      Number(data.turbidity).toFixed(1)
      + ' NTU';


    document.getElementById(
      'device'
    ).innerHTML =
      data.device_id;


    document.getElementById(
      'alert'
    ).innerHTML =
      data.alert;


    let status =
      document.getElementById(
        'status'
      );


    status.innerHTML =
      data.water_status;


    status.className =
      'status';


    if (
      data.water_status
      === 'SAFE'
    ) {

      status.classList.add(
        'safe'
      );

    }

    else if (
      data.water_status
      === 'WARNING'
    ) {

      status.classList.add(
        'warning'
      );

    }

    else if (
      data.water_status
      === 'UNSAFE'
    ) {

      status.classList.add(
        'unsafe'
      );

    }

    else {

      status.classList.add(
        'error'
      );
    }


    document.getElementById(
      'tempHealth'
    ).innerHTML =
      healthText(
        data.temperature_healthy
      );


    document.getElementById(
      'tdsHealth'
    ).innerHTML =
      healthText(
        data.tds_healthy
      );


    document.getElementById(
      'turbHealth'
    ).innerHTML =
      healthText(
        data.turbidity_healthy
      );


    document.getElementById(
      'wifiHealth'
    ).innerHTML =
      healthText(
        data.wifi_healthy
      );


    document.getElementById(
      'backendHealth'
    ).innerHTML =
      healthText(
        data.backend_healthy
      );

  })

  .catch(error => {

    console.log(error);

  });

}


updateData();

setInterval(
  updateData,
  2000
);

</script>

</body>

</html>

)rawliteral";


  server.send(
    200,
    "text/html",
    html
  );
}


// ================================================================
// LOCAL JSON
// ================================================================

void handleData() {

  String json =
    createJSONPayload();


  server.send(
    200,
    "application/json",
    json
  );
}


// ================================================================
// NOT FOUND
// ================================================================

void handleNotFound() {

  server.send(
    404,
    "text/plain",
    "PUREXAI: Page not found"
  );
}


// ================================================================
// READ ALL SENSORS
// ================================================================

void readAllSensors() {

  readTemperature();

  readTDS();

  readTurbidity();
}


// ================================================================
// TEMPERATURE
// ================================================================

void readTemperature() {

  ds18b20.requestTemperatures();


  float value =
    ds18b20.getTempCByIndex(0);


  if (
    value == DEVICE_DISCONNECTED_C
  ) {

    temperatureC = -127.0;

    Serial.println(
      "DS18B20 DISCONNECTED!"
    );

  }

  else {

    temperatureC = value;
  }
}


// ================================================================
// AVERAGE ADC
// ================================================================

int readAverageADC(int pin) {

  long total = 0;


  for (
    int i = 0;
    i < ADC_SAMPLES;
    i++
  ) {

    total += analogRead(pin);

    delay(5);
  }


  return total / ADC_SAMPLES;
}


// ================================================================
// TDS
// ================================================================

void readTDS() {

  tdsRaw =
    readAverageADC(
      TDS_PIN
    );


  tdsVoltage =
    tdsRaw *
    (3.3 / 4095.0);


  float compensationCoefficient = 1.0;


  if (
    temperatureC > -50
    &&
    temperatureC < 100
  ) {

    compensationCoefficient =
      1.0
      +
      0.02 *
      (temperatureC - 25.0);
  }


  float compensationVoltage =
    tdsVoltage /
    compensationCoefficient;


  tdsPPM = (

    133.42 *
    compensationVoltage *
    compensationVoltage *
    compensationVoltage

    -

    255.86 *
    compensationVoltage *
    compensationVoltage

    +

    857.39 *
    compensationVoltage

  ) * 0.5;


  tdsPPM =
    tdsPPM *
    TDS_CALIBRATION_FACTOR;


  if (
    tdsPPM < 0
  ) {

    tdsPPM = 0;
  }
}


// ================================================================
// TURBIDITY
// ================================================================

void readTurbidity() {

  turbidityRaw =
    readAverageADC(
      TURBIDITY_PIN
    );


  turbidityVoltage =
    turbidityRaw *
    (3.3 / 4095.0);


  turbidityNTU =
    mapFloat(

      turbidityVoltage,

      TURBIDITY_CLEAR_VOLTAGE,

      TURBIDITY_MUDDY_VOLTAGE,

      TURBIDITY_CLEAR_NTU,

      TURBIDITY_MUDDY_NTU
    );


  if (
    turbidityNTU < 0
  ) {

    turbidityNTU = 0;
  }


  if (
    turbidityNTU > 5000
  ) {

    turbidityNTU = 5000;
  }
}


// ================================================================
// SENSOR HEALTH
// ================================================================

void checkSensorHealth() {

  temperatureHealthy =
    temperatureC > -10
    &&
    temperatureC < 100;


  tdsHealthy =

    tdsRaw > 10

    &&

    tdsRaw < 4085

    &&

    tdsVoltage >= 0

    &&

    tdsVoltage <= 3.3

    &&

    tdsPPM >= 0

    &&

    tdsPPM < 5000;


  turbidityHealthy =

    turbidityRaw > 10

    &&

    turbidityRaw < 4085

    &&

    turbidityVoltage >= 0

    &&

    turbidityVoltage <= 3.3

    &&

    turbidityNTU >= 0

    &&

    turbidityNTU <= 5000;


  wifiHealthy =
    WiFi.status()
    == WL_CONNECTED;


  allSensorsHealthy =

    tdsHealthy

    &&

    turbidityHealthy

    &&

    temperatureHealthy;
}


// ================================================================
// WATER STATUS
// ================================================================

void determineWaterStatus() {

  if (
    !allSensorsHealthy
  ) {

    waterStatus =
      "SENSOR_ERROR";

    alertMessage =
      "Sensor health error";

    return;
  }


  // ==============================================================
  // UNSAFE
  // ==============================================================

  if (

    tdsPPM >
    TDS_WARNING_MAX

    ||

    turbidityNTU >
    TURBIDITY_WARNING_MAX

  ) {

    waterStatus =
      "UNSAFE";


    if (
      turbidityNTU >
      TURBIDITY_WARNING_MAX
    ) {

      alertMessage =
        "High turbidity";

    }

    else {

      alertMessage =
        "High TDS";
    }


    return;
  }


  // ==============================================================
  // WARNING
  // ==============================================================

  if (

    tdsPPM >
    TDS_SAFE_MAX

    ||

    turbidityNTU >
    TURBIDITY_SAFE_MAX

  ) {

    waterStatus =
      "WARNING";


    if (
      turbidityNTU >
      TURBIDITY_SAFE_MAX
    ) {

      alertMessage =
        "Turbidity warning";

    }

    else {

      alertMessage =
        "TDS warning";
    }


    return;
  }


  // ==============================================================
  // SAFE
  // ==============================================================

  waterStatus =
    "SAFE";

  alertMessage =
    "Water quality normal";
}


// ================================================================
// LED
// ================================================================

void updateLEDs() {

  digitalWrite(
    RED_LED_PIN,
    LOW
  );

  digitalWrite(
    YELLOW_LED_PIN,
    LOW
  );

  digitalWrite(
    GREEN_LED_PIN,
    LOW
  );


  if (
    waterStatus ==
    "SAFE"
  ) {

    digitalWrite(
      GREEN_LED_PIN,
      HIGH
    );
  }


  else if (
    waterStatus ==
    "WARNING"
  ) {

    digitalWrite(
      YELLOW_LED_PIN,
      HIGH
    );
  }


  else if (
    waterStatus ==
    "UNSAFE"
  ) {

    digitalWrite(
      RED_LED_PIN,
      HIGH
    );
  }


  else {

    digitalWrite(
      RED_LED_PIN,
      HIGH
    );

    digitalWrite(
      YELLOW_LED_PIN,
      HIGH
    );
  }
}


// ================================================================
// BUZZER
// ================================================================

void handleBuzzer() {

  unsigned long currentMillis =
    millis();


  // SAFE

  if (
    waterStatus ==
    "SAFE"
  ) {

    digitalWrite(
      BUZZER_PIN,
      LOW
    );

    buzzerState = false;

    return;
  }


  // WARNING

  if (
    waterStatus ==
    "WARNING"
  ) {

    if (
      currentMillis -
      buzzerTimer >= 1000
    ) {

      buzzerTimer =
        currentMillis;

      buzzerState =
        !buzzerState;

      digitalWrite(
        BUZZER_PIN,
        buzzerState
        ? HIGH
        : LOW
      );
    }

    return;
  }


  // UNSAFE

  if (
    waterStatus ==
    "UNSAFE"
  ) {

    if (
      currentMillis -
      buzzerTimer >= 300
    ) {

      buzzerTimer =
        currentMillis;

      buzzerState =
        !buzzerState;

      digitalWrite(
        BUZZER_PIN,
        buzzerState
        ? HIGH
        : LOW
      );
    }

    return;
  }


  // SENSOR ERROR

  if (
    waterStatus ==
    "SENSOR_ERROR"
  ) {

    if (
      currentMillis -
      buzzerTimer >= 150
    ) {

      buzzerTimer =
        currentMillis;

      buzzerState =
        !buzzerState;

      digitalWrite(
        BUZZER_PIN,
        buzzerState
        ? HIGH
        : LOW
      );
    }

    return;
  }


  digitalWrite(
    BUZZER_PIN,
    LOW
  );

  buzzerState = false;
}


// ================================================================
// QR
// ================================================================

String createQRText() {

  return "http://192.168.4.1";
}


// ================================================================
// DISPLAY QR
// ================================================================

void displayQRCode() {

  String qrText =
    createQRText();


  int8_t result =
    qrcode_initText(

      &qrcode,

      qrData,

      QR_VERSION,

      ECC_LOW,

      qrText.c_str()
    );


  if (
    result < 0
  ) {

    Serial.println(
      "QR CODE GENERATION FAILED"
    );

    return;
  }


  uint8_t qrSize =
    qrcode.size;


  int scale = 2;


  int qrPixelSize =
    qrSize * scale;


  int offsetX =
    (SCREEN_WIDTH -
     qrPixelSize) / 2;


  int offsetY =
    (SCREEN_HEIGHT -
     qrPixelSize) / 2;


  display.clearDisplay();


  for (
    uint8_t y = 0;
    y < qrSize;
    y++
  ) {

    for (
      uint8_t x = 0;
      x < qrSize;
      x++
    ) {

      if (
        qrcode_getModule(
          &qrcode,
          x,
          y
        )
      ) {

        display.fillRect(

          offsetX +
          x * scale,

          offsetY +
          y * scale,

          scale,

          scale,

          SSD1306_WHITE
        );
      }
    }
  }


  display.display();
}


// ================================================================
// LOCAL ESP32 JSON
// ================================================================

String createJSONPayload() {

  StaticJsonDocument<1024> doc;


  doc["device_id"] =
    DEVICE_ID;


  doc["temperature"] =
    temperatureC;


  doc["tds"] =
    tdsPPM;


  doc["turbidity"] =
    turbidityNTU;


  doc["tds_voltage"] =
    tdsVoltage;


  doc["turbidity_voltage"] =
    turbidityVoltage;


  doc["tds_raw"] =
    tdsRaw;


  doc["turbidity_raw"] =
    turbidityRaw;


  doc["water_status"] =
    waterStatus;


  doc["alert"] =
    alertMessage;


  doc["tds_healthy"] =
    tdsHealthy;


  doc["turbidity_healthy"] =
    turbidityHealthy;


  doc["temperature_healthy"] =
    temperatureHealthy;


  doc["wifi_healthy"] =
    wifiHealthy;


  doc["backend_healthy"] =
    backendHealthy;


  if (
    WiFi.status()
    == WL_CONNECTED
  ) {

    doc["wifi_rssi"] =
      WiFi.RSSI();

  }

  else {

    doc["wifi_rssi"] =
      0;
  }


  doc["ap_ip"] =
    WiFi.softAPIP().toString();


  String json;


  serializeJson(
    doc,
    json
  );


  return json;
}


// ================================================================
// FASTAPI JSON PAYLOAD
// ================================================================
//
// FastAPI expects:
//
// tds_ppm
// turbidity_ntu
// temperature_c
// device_id
// ip_address
//
// ================================================================

String createBackendPayload() {

  StaticJsonDocument<512> doc;


  doc["tds_ppm"] =
    tdsPPM;


  doc["turbidity_ntu"] =
    turbidityNTU;


  doc["temperature_c"] =
    temperatureC;


  doc["device_id"] =
    DEVICE_ID;


  // Send ESP32 router IP

  if (
    WiFi.status()
    == WL_CONNECTED
  ) {

    doc["ip_address"] =
      WiFi.localIP().toString();

  }

  else {

    doc["ip_address"] =
      "0.0.0.0";
  }


  String json;


  serializeJson(
    doc,
    json
  );


  return json;
}


// ================================================================
// SEND DATA TO FASTAPI
// ================================================================

void sendDataToServer() {

  if (
    WiFi.status()
    != WL_CONNECTED
  ) {

    backendHealthy = false;

    Serial.println(
      "Cannot send: Router WiFi disconnected"
    );

    return;
  }


  HTTPClient http;


  Serial.println();
  Serial.println(
    "=============================================="
  );

  Serial.println(
    "SENDING DATA TO FASTAPI"
  );

  Serial.print(
    "Server: "
  );

  Serial.println(
    SERVER_URL
  );


  // ==============================================================
  // CONNECT
  // ==============================================================

  http.begin(
    SERVER_URL
  );


  // ==============================================================
  // HEADERS
  // ==============================================================

  http.addHeader(
    "Content-Type",
    "application/json"
  );


  http.setTimeout(
    5000
  );


  // ==============================================================
  // CREATE CORRECT JSON
  // ==============================================================

  String jsonData =
    createBackendPayload();


  Serial.println(
    "JSON SENT:"
  );

  Serial.println(
    jsonData
  );


  // ==============================================================
  // POST
  // ==============================================================

  int httpCode =
    http.POST(
      jsonData
    );


  // ==============================================================
  // SUCCESS
  // ==============================================================

  if (
    httpCode == 200
    ||
    httpCode == 201
  ) {

    backendHealthy = true;


    Serial.println();
    Serial.println(
      "Backend data sent successfully"
    );


    Serial.print(
      "HTTP Code: "
    );

    Serial.println(
      httpCode
    );


    String response =
      http.getString();


    Serial.println(
      "Backend Response:"
    );

    Serial.println(
      response
    );
  }


  // ==============================================================
  // ERROR
  // ==============================================================

  else {

    backendHealthy = false;


    Serial.println();
    Serial.println(
      "BACKEND ERROR"
    );


    Serial.print(
      "HTTP Code: "
    );

    Serial.println(
      httpCode
    );


    String response =
      http.getString();


    Serial.println(
      "Backend Response:"
    );

    Serial.println(
      response
    );
  }


  http.end();


  Serial.println(
    "=============================================="
  );
}


// ================================================================
// TELEGRAM MESSAGE
// ================================================================

void sendTelegramMessage(String message) {

  // ==============================================================
  // CHECK WIFI
  // ==============================================================

  if (
    WiFi.status()
    != WL_CONNECTED
  ) {

    Serial.println(
      "Telegram alert skipped - WiFi disconnected"
    );

    telegramHealthy = false;

    return;
  }


  // ==============================================================
  // CREATE HTTPS REQUEST
  // ==============================================================

  HTTPClient http;


  String telegramURL =
    "https://api.telegram.org/bot" +
    String(TELEGRAM_BOT_TOKEN) +
    "/sendMessage";


  // ==============================================================
  // CONNECT TO TELEGRAM
  // ==============================================================

  http.begin(
    telegramURL
  );


  // ==============================================================
  // HEADERS
  // ==============================================================

  http.addHeader(
    "Content-Type",
    "application/json"
  );


  http.setTimeout(
    10000
  );


  // ==============================================================
  // CREATE JSON
  // ==============================================================

  StaticJsonDocument<1024> doc;


  doc["chat_id"] =
    TELEGRAM_CHAT_ID;


  doc["text"] =
    message;


  String jsonData;


  serializeJson(
    doc,
    jsonData
  );


  // ==============================================================
  // SEND MESSAGE
  // ==============================================================

  Serial.println();
  Serial.println(
    "=============================================="
  );

  Serial.println(
    "SENDING TELEGRAM ALERT"
  );

  Serial.println(
    message
  );


  int httpCode =
    http.POST(
      jsonData
    );


  // ==============================================================
  // SUCCESS
  // ==============================================================

  if (
    httpCode >= 200
    &&
    httpCode < 300
  ) {

    telegramHealthy = true;

    Serial.println(
      "Telegram notification sent successfully"
    );

    Serial.print(
      "Telegram HTTP Code: "
    );

    Serial.println(
      httpCode
    );
  }


  // ==============================================================
  // ERROR
  // ==============================================================

  else {

    telegramHealthy = false;

    Serial.println(
      "TELEGRAM ERROR"
    );

    Serial.print(
      "HTTP Code: "
    );

    Serial.println(
      httpCode
    );


    String response =
      http.getString();


    Serial.println(
      "Telegram Response:"
    );

    Serial.println(
      response
    );
  }


  http.end();


  Serial.println(
    "=============================================="
  );
}


// ================================================================
// TELEGRAM ALERT CHECK
// ================================================================
//
// Sends Telegram notifications when:
//
// 1. Water becomes UNSAFE
// 2. Any sensor stops working
//
// Repeated alerts are prevented.
//
// ================================================================

void checkTelegramAlert() {

  // ==============================================================
  // UNSAFE WATER
  // ==============================================================

  if (
    waterStatus == "UNSAFE"
  ) {

    if (
      lastTelegramAlert != "UNSAFE"
    ) {

      String message =
        "🚨 PUREXAI WATER ALERT 🚨\n\n"
        "⚠️ UNSAFE WATER DETECTED!\n\n"
        "Device: " +
        String(DEVICE_ID) +
        "\n\n"
        "TDS: " +
        String(tdsPPM, 1) +
        " ppm\n"
        "Turbidity: " +
        String(turbidityNTU, 1) +
        " NTU\n"
        "Temperature: " +
        String(temperatureC, 1) +
        " °C\n\n"
        "Reason: " +
        alertMessage +
        "\n\n"
        "Please check the water immediately.";


      sendTelegramMessage(
        message
      );


      lastTelegramAlert =
        "UNSAFE";
    }


    return;
  }


  // ==============================================================
  // SENSOR ERROR
  // ==============================================================

  if (
    waterStatus == "SENSOR_ERROR"
  ) {

    if (
      lastTelegramAlert != "SENSOR_ERROR"
    ) {

      String message =
        "🚨 PUREXAI SENSOR ALERT 🚨\n\n"
        "❌ SENSOR NOT WORKING!\n\n"
        "Device: " +
        String(DEVICE_ID) +
        "\n\n"
        "Temperature Sensor: " +
        (
          temperatureHealthy
          ? "OK"
          : "ERROR"
        ) +
        "\n"
        "TDS Sensor: " +
        (
          tdsHealthy
          ? "OK"
          : "ERROR"
        ) +
        "\n"
        "Turbidity Sensor: " +
        (
          turbidityHealthy
          ? "OK"
          : "ERROR"
        ) +
        "\n\n"
        "Please check the sensor connections.";


      sendTelegramMessage(
        message
      );


      lastTelegramAlert =
        "SENSOR_ERROR";
    }


    return;
  }


  // ==============================================================
  // RESET ALERT STATE
  // ==============================================================

  if (
    waterStatus == "SAFE"
    ||
    waterStatus == "WARNING"
  ) {

    lastTelegramAlert = "";
  }
}


// ================================================================
// SERIAL MONITOR
// ================================================================

void printSerialData() {

  Serial.println();

  Serial.println(
    "----------------------------------------------"
  );


  Serial.print(
    "Temperature : "
  );

  Serial.print(
    temperatureC,
    2
  );

  Serial.println(
    " C"
  );


  Serial.print(
    "TDS Raw     : "
  );

  Serial.println(
    tdsRaw
  );


  Serial.print(
    "TDS Voltage : "
  );

  Serial.print(
    tdsVoltage,
    4
  );

  Serial.println(
    " V"
  );


  Serial.print(
    "TDS         : "
  );

  Serial.print(
    tdsPPM,
    2
  );

  Serial.println(
    " ppm"
  );


  Serial.print(
    "Turb Raw    : "
  );

  Serial.println(
    turbidityRaw
  );


  Serial.print(
    "Turb Voltage: "
  );

  Serial.print(
    turbidityVoltage,
    4
  );

  Serial.println(
    " V"
  );


  Serial.print(
    "Turbidity   : "
  );

  Serial.print(
    turbidityNTU,
    2
  );

  Serial.println(
    " NTU"
  );


  Serial.print(
    "Water Status: "
  );

  Serial.println(
    waterStatus
  );


  Serial.print(
    "Device ID   : "
  );

  Serial.println(
    DEVICE_ID
  );


  Serial.print(
    "AP IP       : "
  );

  Serial.println(
    WiFi.softAPIP()
  );


  Serial.print(
    "Router IP   : "
  );

  if (
    WiFi.status()
    == WL_CONNECTED
  ) {

    Serial.println(
      WiFi.localIP()
    );

  }

  else {

    Serial.println(
      "DISCONNECTED"
    );
  }


  Serial.print(
    "Backend     : "
  );

  if (backendHealthy) {

    Serial.println(
      "CONNECTED"
    );

  }

  else {

    Serial.println(
      "NOT CONNECTED"
    );
  }


  Serial.print(
    "Telegram    : "
  );

  if (telegramHealthy) {

    Serial.println(
      "CONNECTED"
    );

  }

  else {

    Serial.println(
      "NOT CONNECTED"
    );
  }


  Serial.println(
    "----------------------------------------------"
  );
}


// ================================================================
// FLOAT MAP
// ================================================================

float mapFloat(

  float x,

  float in_min,

  float in_max,

  float out_min,

  float out_max

) {

  if (
    in_max == in_min
  ) {

    return out_min;
  }


  return (

    (x - in_min)

    *

    (out_max - out_min)

    /

    (in_max - in_min)

    +

    out_min
  );
}
