#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SD.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Pin configuration for TFT display
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4

// Pin configuration for SD card
#define SD_CS     15

// NTP setup
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 7 * 3600, 60000);  // 7 hours offset for WIB

// Structure matching the sender's data format
typedef struct struct_message {
  int co2;              // CO2 ppm from MH-Z19
  float methane;        // Methane ppm from MQ-4
  float temperature;    // Temperature from DHT22
  float humidity;       // Humidity from DHT22
} struct_message;

struct_message receivedData;

// Initialize TFT display
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Callback function to handle incoming data
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  if (len == sizeof(receivedData)) {  // Ensure the received data length is correct
    memcpy(&receivedData, incomingData, sizeof(receivedData));
  } else {
    Serial.println("Error: Invalid data length received.");
  }
}

// Function to initialize WiFi in station mode and ESP-NOW
void initESPNow() {
  WiFi.mode(WIFI_STA); // Set WiFi mode to station (no need for AP mode)
  Serial.println("Initializing ESP-NOW...");
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv); // Register callback function to handle received data
}

// Setup function
void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi
  WiFi.begin("Anwar", "abcdefghi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize TFT display
  tft.initR(INITR_BLACKTAB);  // Correct initialization for many ST7735 displays
  tft.setRotation(3);  // Optional: Set display orientation
  tft.fillScreen(ST77XX_BLACK);  // Clear the screen initially

  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
    tft.println("SD Init Failed!");
    while (1);  // Stop the program here if SD initialization fails
  }
  Serial.println("SD Card initialized.");
  tft.println("SD Initialized");

  // Write CSV header to SD card
  File dataFile = SD.open("/data.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println("Timestamp,CO2,Methane,Temp,Humidity");  // CSV header
    dataFile.close();
  } else {
    Serial.println("Error opening CSV file for writing!");
  }

  // Initialize NTP client for time
  timeClient.begin();
  timeClient.update();

  initESPNow();  // Initialize ESP-NOW
}

// Draw small SD card icon based on initialization status
void drawSDCardIcon(int x, int y) {
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(x, y);
  tft.print("SD");

  if (SD.begin(SD_CS)) {
    tft.fillRect(x + 25, y + 3, 5, 5, ST77XX_GREEN);  // Green square for initialized
  } else {
    tft.fillRect(x + 25, y + 3, 5, 5, ST77XX_RED);  // Red square for failed
  }
}

// Draw small WiFi icon
void drawWiFiIcon(int x, int y) {
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(x, y);

  // Cek status koneksi WiFi
  if (WiFi.status() == WL_CONNECTED) {
    tft.print("WiFi");
    tft.fillCircle(x + 25, y + 5, 3, ST77XX_GREEN);  // Green dot as WiFi indicator when connected
  } else {
    tft.print("Wi
    tft.fillCircle(x + 25, y + 5, 3, ST77XX_RED);  // Red dot as WiFi indicator when disconnected
  }
}


// Main loop
void loop() {
  // Get current time from NTP client and update display every loop
  timeClient.update();
  String timestamp = timeClient.getFormattedTime();

  // Display values on TFT
  tft.fillScreen(ST77XX_BLACK);  // Clear screen
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);  // Smaller text size for better layout
  
  // Set a starting Y position for text
  int yPos = 10;  // Starting vertical position for tighter layout
  int xPos = 10;  // Horizontal position (shifted from the left)

  // Display timestamp with larger font and different color
  tft.setTextColor(ST77XX_CYAN);  // Set color for timestamp
  tft.setCursor(xPos, yPos);  
  tft.print("Time: "); tft.println(timestamp);
  yPos += 16;  // Increase line spacing for timestamp

  // Display each data value with different colors for better readability
  tft.setTextColor(ST77XX_GREEN);  // Set color for CO2
  tft.setCursor(xPos, yPos);  // Position for CO2
  tft.print("CO2: "); tft.print(receivedData.co2 == 0 ? 0 : receivedData.co2); tft.print(" ppm");
  yPos += 16;  // Increase line spacing for CO2

  tft.setTextColor(ST77XX_RED);  // Set color for Methane
  tft.setCursor(xPos, yPos);  // Position for Methane
  tft.print("Methane: ");
  tft.print((int)(receivedData.methane == 0 ? 0.0 : receivedData.methane));  // Display as integer
  tft.print(" ppm");
  yPos += 16;  // Increase line spacing for Methane

  tft.setTextColor(ST77XX_YELLOW);  // Set color for Temperature
  tft.setCursor(xPos, yPos);  // Position for Temperature
  tft.print("Temp: "); tft.print(receivedData.temperature == 0 ? 0.0 : receivedData.temperature); tft.print(" C");
  yPos += 16;  // Increase line spacing for Temperature

  tft.setTextColor(ST77XX_MAGENTA);  // Set color for Humidity
  tft.setCursor(xPos, yPos);  // Position for Humidity
  tft.print("Humidity: "); tft.print(receivedData.humidity == 0 ? 0.0 : receivedData.humidity); tft.print(" %");
  yPos += 16;  // Increase line spacing for Humidity

  // Draw SD card icon and text first, followed by WiFi icon and text
  drawSDCardIcon(95, 100);  // SD card icon at a higher position (near the bottom but not touching bezel)
  drawWiFiIcon(95, 110);  // WiFi icon below the SD card icon

  // Also save the data to SD card with timestamp in CSV format
  File dataFile = SD.open("/data.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.print(timestamp);  // Timestamp
    dataFile.print(",");  // CSV separator
    dataFile.print(receivedData.co2);
    dataFile.print(",");

    dataFile.print((int)(receivedData.methane == 0 ? 0.0 : receivedData.methane));  // Methane as integer
    dataFile.print(",");
    dataFile.print(receivedData.temperature == 0 ? 0.0 : receivedData.temperature);
    dataFile.print(",");
    dataFile.println(receivedData.humidity == 0 ? 0.0 : receivedData.humidity);
    dataFile.close();
  } else {
    Serial.println("Error opening data file on SD card.");
  }

  delay(1000);  // Wait for a second before updating display again
}
