#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#define BLYNK_TEMPLATE_ID "TMPL2KWcoVIYJ"
#define BLYNK_TEMPLATE_NAME "MQTT DHT2"
#define BLYNK_AUTH_TOKEN "N_GbHF8QH3j-8Mpt4VQVRzz4MK-LH0lX"
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <SPIFFS.h>
#include <FS.h>



// ====== WiFi CONFIGURATION ======
const char* ssid = "SMART-IOT";
const char* pass = "70491746";

// DHT11 sensor setup
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Gas sensor setup
#define GAS_SENSOR_PIN 35  // Changed from 34 to avoid conflict

// Buzzer setup
#define BUZZER_PIN 5

// Gas threshold for alarm
#define GAS_THRESHOLD 2200

// Timing variables
BlynkTimer timer;
int dhtErrorCount = 0;
bool alarmActive = false;
unsigned long lastAlarmTime = 0;
const unsigned long alarmInterval = 1000;

// ====== Pins ======
int lrPin = 34;   // ADC input pin for light sensor
int ldPin = 21;   // PWM output pin for light control

// ====== Control parameters ======
float Kp = 0.05;
int target = 500;
int pwmValue = 127;

// Sensor values storage
float temperature = 0;
float humidity = 0;
int gasValue = 0;

// Data logging
bool loggingEnabled = true;
unsigned long lastLogTime = 0;
const unsigned long logInterval = 600000; // Log every 30 seconds

// ====== Web server ======
WebServer server(80);

// ====== HTML UI ======
String htmlPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Smart Farm Dashboard</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #2d2d2d;
      margin: 0;
      padding: 15px;
      color: #fff;
    }
    .dashboard-container {
      max-width: 1500px;
      margin: 0 auto;
    }
    .main-content {
      display: flex;
      flex-wrap: wrap;
      gap: 20px;
      margin-bottom: 20px;
    }
    .control-panel, .status-panel {
      background: #3a3a3a;
      border-radius: 10px;
      padding: 15px;
      flex: 1;
      min-width: 300px;
    }
    .panel-title {
      font-size: 18px;
      margin-bottom: 15px;
      color: #fdbb2d;
      border-bottom: 1px solid #555;
      padding-bottom: 8px;
    }
    .target-display {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 15px;
      background: #444;
      padding: 10px;
      border-radius: 8px;
    }
    #targetVal {
      font-size: 20px;
      font-weight: bold;
      color: #fdbb2d;
    }
    .slidecontainer {
      width: 100%;
      margin: 15px 0;
    }
    .slider {
      width: 100%;
      height: 10px;
      border-radius: 5px;
      background: #555;
      outline: none;
    }
    .slider::-webkit-slider-thumb {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: #fdbb2d;
      cursor: pointer;
    }
    .button-container {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 10px;
      margin-top: 15px;
    }
    button {
      border: none;
      border-radius: 8px;
      padding: 12px;
      cursor: pointer;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      background: #444;
      color: white;
      font-size: 14px;
    }
    .button-emoji {
      font-size: 20px;
      margin-bottom: 5px;
    }
    .carrot { border-top: 3px solid #e67e22; }
    .broccoli { border-top: 3px solid #27ae60; }
    .onion { border-top: 3px solid #9b59b6; }
    .tomato { border-top: 3px solid #e74c3c; }
    .btn-water { 
      background: #3498db; 
      border-top: 3px solid #2980b9; 
      font-size: 12px;
      padding: 8px;
    }
    .btn-air { 
      background: #2ecc71; 
      border-top: 3px solid #27ae60; 
      font-size: 12px;
      padding: 8px;
    }
    .btn-data { 
      background: #9b59b6; 
      border-top: 3px solid #8e44ad; 
      font-size: 12px;
      padding: 8px;
    }
    .status-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 10px;
    }
    .status-item {
      background: #444;
      padding: 10px;
      border-radius: 8px;
    }
    .status-label {
      font-size: 12px;
      color: #aaa;
      margin-bottom: 5px;
    }
    .status-value {
      font-size: 16px;
      font-weight: bold;
    }
    #tempVal { color: #1abc9c; }
    #humVal { color: #3498db; }
    #gasStatus { color: #e74c3c; }
    .gauges-wrapper {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 15px;
      margin-top: 15px;
    }
    .gauge {
      text-align: center;
      background: #444;
      border-radius: 8px;
      padding: 10px;
    }
    .bar {
      width: 80px;
      height: 80px;
      border-radius: 50%;
      background: conic-gradient(var(--color) var(--angle,0deg), #555 0deg);
      transform: rotate(180deg);
      margin: 0 auto 10px;
    }
    .value {
      font-size: 14px;
      font-weight: bold;
      margin-bottom: 5px;
    }
    .title {
      font-size: 12px;
      color: #aaa;
    }
    .gas-alarm {
      background: #e74c3c;
      padding: 10px;
      border-radius: 8px;
      margin-top: 15px;
      text-align: center;
      display: none;
    }
    .data-section {
      margin-top: 20px;
      background: #3a3a3a;
      border-radius: 10px;
      padding: 15px;
    }
    .data-buttons {
      display: flex;
      gap: 10px;
      margin-top: 10px;
    }
    .data-buttons button {
      flex: 1;
    }
  </style>
</head>
<body>
<h1>SMART FARM</h1>
  <div class="dashboard-container">
    <div class="main-content">
      <div class="control-panel">
        <div class="panel-title">Light Control</div>
        <div class="target-display">
          <span>Target Value:</span>
          <span id="targetVal">500</span>
        </div>
        <div class="slidecontainer">
          <input type="range" min="100" max="1000" value="500" class="slider" id="myRange">
        </div>
        <div class="panel-title">Preset Values</div>
        <div class="button-container">
          <button class="carrot" onclick="setValue(200)">
            <span class="button-emoji">🥕</span>
            <span>Carrot (200)</span>
          </button>
          <button class="broccoli" onclick="setValue(400)">
            <span class="button-emoji">🥦</span>
            <span>Broccoli (400)</span>
          </button>
          <button class="onion" onclick="setValue(700)">
            <span class="button-emoji">🧅</span>
            <span>Onion (700)</span>
          </button>
          <button class="tomato" onclick="setValue(900)">
            <span class="button-emoji">🍅</span>
            <span>Tomato (900)</span>
          </button>
          <button class="btn-water">Watering</button>
          <button class="btn-air">Fresh Air</button>  
        </div>
      </div>
      <div class="status-panel">
        <div class="panel-title">System Status</div>
        <div class="status-grid">
          <div class="status-item">
            <span class="status-label">Temperature</span>
            <span id="tempVal" class="status-value">0°C</span>
          </div>
          <div class="status-item">
            <span class="status-label">Humidity</span>
            <span id="humVal" class="status-value">0%</span>
          </div>
          <div class="status-item">
            <span class="status-label">GAS</span>
            <span id="gasStatus" class="status-value">Normal</span>
          </div>
          <div class="status-item">
            <span class="status-value">AI SUGGESTION</span>            
          </div>
        </div>
        <div id="gasAlarm" class="gas-alarm">
          WARNING: HIGH GAS LEVEL DETECTED!
        </div>
        <div class="panel-title">Sensor Readings</div>
        <div class="gauges-wrapper">
          <div class="gauge">
            <div class="bar" id="bar1" style="--color:#16a34a"></div>
            <div class="value" id="val1">0°C</div>
            <div class="title">Temperature</div>
          </div>

          <div class="gauge">
            <div class="bar" id="bar2" style="--color:#0ea5e9"></div>
            <div class="value" id="val2">0%</div>
            <div class="title">Humidity</div>
          </div>

          <div class="gauge">
            <div class="bar" id="bar3" style="--color:#f97316"></div>
            <div class="value" id="val3">0 ppm</div>
            <div class="title">Gas Level</div>
          </div>
        </div>
        
        <div class="panel-title">Data Logging</div>
        <div class="data-section">
          <div class="status-item">
            <span class="status-label">Data Logging</span>
            <span id="logStatus" class="status-value">Active</span>
          </div>
          <div class="data-buttons">
            <button class="btn-data" onclick="downloadData()">
              <span class="button-emoji">📥</span>
              <span>Download CSV</span>
            </button>
            <button class="btn-data" onclick="toggleLogging()">
              <span class="button-emoji">⏯️</span>
              <span id="toggleLogText">Pause Logging</span>
            </button>
            <button class="btn-data" onclick="deleteData()">
              <span class="button-emoji">🗑️</span>
              <span>Clear Data</span>
            </button>
          </div>
        </div>
      </div>
    </div>
  </div>
  <script>
    var slider = document.getElementById("myRange");
    var targetLabel = document.getElementById("targetVal");
    var loggingEnabled = true;
    
    slider.oninput = function() {
      targetLabel.innerHTML = this.value;
      fetch("/setTarget?value=" + this.value);
    }
    
    function setValue(val) {
      slider.value = val;
      targetLabel.textContent = val;
      fetch("/setTarget?value=" + val); 
    }
    
    const gauges = [
      {bar: document.getElementById("bar1"), val: document.getElementById("val1"), max: 50, unit:"°C", value:0, target:0},
      {bar: document.getElementById("bar2"), val: document.getElementById("val2"), max:100, unit:"%", value:0, target:0},
      {bar: document.getElementById("bar3"), val: document.getElementById("val3"), max:4096, unit:"ppm", value:0, target:0}
    ];

    function animate() {
      gauges.forEach(g => {
        g.value += (g.target - g.value) * 0.1;
        const frac = Math.max(0, Math.min(1, g.value/g.max));
        g.bar.style.setProperty("--angle", (frac*180) + "deg");
        g.val.textContent = Math.round(g.value) + " " + g.unit;
      });
      requestAnimationFrame(animate);
    }
    animate();

    function updateDashboard() {
      fetch("/status").then(r => r.json()).then(data => {
        // Update status values
        document.getElementById("tempVal").textContent = data.temperature + "°C";
        document.getElementById("humVal").textContent = data.humidity + "%";
        
        // Update gas status
        const gasStatus = document.getElementById("gasStatus");
        const gasAlarm = document.getElementById("gasAlarm");
        if (data.gas > 2200) {
          gasStatus.textContent = "DANGEROUS";
          gasStatus.style.color = "#e74c3c";
          gasAlarm.style.display = "block";
        } else if (data.gas > 1500) {
          gasStatus.textContent = "WARNING";
          gasStatus.style.color = "#f39c12";
          gasAlarm.style.display = "none";
        } else {
          gasStatus.textContent = "NORMAL";
          gasStatus.style.color = "#2ecc71";
          gasAlarm.style.display = "none";
        }
        
        // Update gauges
        gauges[0].target = data.temperature;
        gauges[1].target = data.humidity;
        gauges[2].target = data.gas;
        
        // Update logging status
        document.getElementById("logStatus").textContent = data.logging ? "Active" : "Paused";
      });
    }
    
    function downloadData() {
      window.open("/download", "_blank");
    }
    
    function toggleLogging() {
      fetch("/toggleLogging").then(() => {
        updateDashboard();
      });
    }
    
    function deleteData() {
      if (confirm("Are you sure you want to delete all logged data?")) {
        fetch("/deleteData").then(() => {
          alert("Data deleted successfully");
        });
      }
    }
    
    // Update dashboard every 2 seconds
    setInterval(updateDashboard, 2000);
    updateDashboard(); // Initial update
  </script>
</body>
</html>
  )rawliteral";
  return page;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleSetTarget() {
  if (server.hasArg("value")) {
    target = server.arg("value").toInt();
    Serial.println("Target set to: " + String(target));
  }
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"gas\":" + String(gasValue) + ",";
  json += "\"logging\":" + String(loggingEnabled ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleDownload() {
  File file = SPIFFS.open("/sensor_data.csv", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  
  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=sensor_data.csv");
  server.sendHeader("Connection", "close");
  
  // Stream the file to the client
  server.streamFile(file, "text/csv");
  file.close();
}

void handleToggleLogging() {
  loggingEnabled = !loggingEnabled;
  server.send(200, "text/plain", loggingEnabled ? "Logging enabled" : "Logging disabled");
}

void handleDeleteData() {
  if (SPIFFS.exists("/sensor_data.csv")) {
    SPIFFS.remove("/sensor_data.csv");
    
    // Create a new file with headers
    File file = SPIFFS.open("/sensor_data.csv", "w");
    if (file) {
      file.println("Timestamp,Temperature,Humidity,Gas");
      file.close();
    }
  }
  server.send(200, "text/plain", "Data deleted");
}

void logSensorData() {
  if (!loggingEnabled) return;
  
  unsigned long currentTime = millis();
  if (currentTime - lastLogTime >= logInterval) {
    lastLogTime = currentTime;
    
    File file = SPIFFS.open("/sensor_data.csv", "a");
    if (file) {
      // Get current time
      String timestamp = String(millis() / 1000);
      
      // Create CSV line
      String dataLine = timestamp + "," + 
                        String(temperature) + "," + 
                        String(humidity) + "," + 
                        String(gasValue);
      
      file.println(dataLine);
      file.close();
      
      Serial.println("Data logged: " + dataLine);
    } else {
      Serial.println("Failed to open file for logging");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nConnecting to WiFi...");
   Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }
  
  // Create data file with headers if it doesn't exist
  if (!SPIFFS.exists("/sensor_data.csv")) {
    File file = SPIFFS.open("/sensor_data.csv", "w");
    if (file) {
      file.println("Timestamp,Temperature,Humidity,Gas");
      file.close();
      Serial.println("Created new data file");
    }
  }
  
  pinMode(ldPin, OUTPUT);
  analogWrite(ldPin, pwmValue);

  // Initialize DHT sensor
  dht.begin();
  delay(2000);
  
  // Initialize gas sensor pin
  pinMode(GAS_SENSOR_PIN, INPUT);
  
  // Initialize buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Connect to WiFi
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());

  // Setup web server
  server.on("/", handleRoot);
  server.on("/setTarget", handleSetTarget);
  server.on("/status", handleStatus);
  server.on("/download", handleDownload);
  server.on("/toggleLogging", handleToggleLogging);
  server.on("/deleteData", handleDeleteData);
  server.begin();

  // Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  // Setup a function to be called every 2 seconds
  timer.setInterval(2000L, readSensors);
  
  Serial.println("System initialized with data logging...");
}

void loop() {
  server.handleClient();
  Blynk.run();
  timer.run();
  
  // Handle light control
  int adcValue = analogRead(lrPin);
  int error = target - adcValue;
  
  // Proportional control
  pwmValue += -Kp * error;
  pwmValue = constrain(pwmValue, 0, 255);
  analogWrite(ldPin, pwmValue);
  
  // Handle alarm buzzer
  handleAlarm();
  
  // Log sensor data
  logSensorData();

  delay(10);
}

void readSensors() {
  // Read DHT11 sensor data
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  // Check if DHT reads failed
  if (isnan(h) || isnan(t)) {
    dhtErrorCount++;
    Serial.println("DHT11 Read Error - Attempt: " + String(dhtErrorCount));
    
    // Try to reinitialize after multiple errors
    if (dhtErrorCount > 5) {
      Serial.println("Reinitializing DHT sensor...");
      dht.begin();
      delay(2000);
      dhtErrorCount = 0;
    }
    
    Blynk.virtualWrite(V3, "DHT Error");
  } else {
    // Reset error count on successful read
    dhtErrorCount = 0;
    
    // Store values for web interface
    humidity = h;
    temperature = t;
    
    // Send DHT data to Blynk
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, humidity);
    
    // Print values to Serial Monitor
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("°C, Humidity: ");
    Serial.print(humidity);
  }
  
  // Read gas sensor
  gasValue = analogRead(GAS_SENSOR_PIN);
  
  // Send gas data to Blynk
  Blynk.virtualWrite(V2, gasValue);
  Serial.print("%, Gas Level: ");
  Serial.println(gasValue);
  
  // Check gas threshold and activate alarm if needed
  if (gasValue > GAS_THRESHOLD) {
    if (!alarmActive) {
      Serial.println("GAS ALARM ACTIVATED! Level: " + String(gasValue));
      Blynk.virtualWrite(V3, "GAS ALARM!");
      alarmActive = true;
      
      // Send push notification
      Blynk.logEvent("gas_alert", "Warning: High gas level detected!");
    }
  } else {
    if (alarmActive) {
      Serial.println("Gas level back to normal");
      Blynk.virtualWrite(V3, "Online");
      alarmActive = false;
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
}

void handleAlarm() {
  if (alarmActive) {
    unsigned long currentTime = millis();
    if (currentTime - lastAlarmTime >= alarmInterval) {
      lastAlarmTime = currentTime;
      // Toggle buzzer
      digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN));
    }
  }
}

// Blynk connection handler
BLYNK_CONNECTED() {
  Blynk.syncAll();
  Serial.println("Connected to Blynk server");
}

// Blynk disconnect handler
BLYNK_DISCONNECTED() {
  Serial.println("Disconnected from Blynk server");
  Blynk.virtualWrite(V3, "Offline");
  digitalWrite(BUZZER_PIN, LOW);
}

// Manual buzzer control from Blynk app
BLYNK_WRITE(V4) {
  int buttonState = param.asInt();
  if (buttonState == 1) {
    // Manual alarm test
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// Manual alarm reset from Blynk app
BLYNK_WRITE(V5) {
  int buttonState = param.asInt();
  if (buttonState == 1) {
    // Reset alarm
    alarmActive = false;
    digitalWrite(BUZZER_PIN, LOW);
    Blynk.virtualWrite(V3, "Online");
    Serial.println("Alarm manually reset");
  }
}
