#include <Arduino.h>
#include <MHZ19.h>
#include <DHT.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <esp_now.h>
#include <ESP_Google_Sheet_Client.h>

// Konfigurasi pin dan sensor
#define RX_PIN 16
#define TX_PIN 17
#define BAUDRATE 9600
#define DHTPIN_1 13
#define DHTTYPE DHT22
const int MQ4_Pin = 33;       // Pin MQ-4 pada GPIO 33
const float R_0 = 945.0;      // Nilai R_0 untuk kalibrasi sensor MQ-4

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

const float V_REF = 3.3;      // Tegangan referensi untuk ESP32
const int ADC_RESOLUTION = 4095; // Resolusi 12-bit untuk ESP32

// Struktur data untuk pengiriman
typedef struct struct_message {
  int co2;                    // CO2 ppm dari MH-Z19 (tanpa desimal)
  float methane;              // Metana ppm dari MQ-4 (tanpa desimal)
  float temperature;          // Suhu dari DHT22
  float humidity;             // Kelembapan dari DHT22
} struct_message;

struct_message myData;

// Alamat MAC penerima
uint8_t receiverMac[] = {0x34, 0x5F, 0x45, 0xA9, 0x17, 0xB0};  // Update

// Inisialisasi sensor dan tampilan
MHZ19 myMHZ19;
HardwareSerial mySerial(1);
DHT dht1(DHTPIN_1, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variabel global untuk pengaturan waktu
unsigned long previousMillis = 0;
const long interval = 30000; // Kirim data setiap 30 detik (30000 ms)
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200; // GMT+7 dalam detik
const int daylightOffset_sec = 0; // Tidak ada waktu siang

// Kredensial WiFi
const char* ssid1 = "airmon";
const char* password1 = "12345678";
const char* ssid2 = "Anwar";
const char* password2 = "abcdefghi";

// Google Sheets Config
#define PROJECT_ID "datalogging-441707"
#define CLIENT_EMAIL "test-822@datalogging-441707.iam.gserviceaccount.com"
const char PRIVATE_KEY[] PROGMEM =  "-----BEGIN PRIVATE KEY-----\nMIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCgqIxMI0Z6LpeT\ndrVAQMpEIxqqwBkFiT60WPbsE8yl59Cp3IZInVvv4/3F8vKqdrbEwfmF5KUpS+RP\nv3B2BS/a/piBE6ZeEVjgXcE1PgIeS5V4HHXdGjonVsDmCR3F0I0CmiErJt9KGGQv\nxQsMXwYs7ewfUJcoJfx4M1Oy5Cizr11nkJfTdoB73juS5vDz6xdKcQIyyI6/+YVn\ntC9wLDi5vnzig7ODxR2Ikr4s01WF3I/ijl2RJ4CWd4uFz0sw0DFHaAopSFQ0yU4M\nA3oKi1eiEvNmdUB6Cs9wLFPchpa8YA77hnBz6oWVQLKj2JpT9BRLQl03RfBQ7pFp\nZ0+u6Hr1AgMBAAECggEAQvTr/+Flwo8/gehj8uLtyYb92rMT2pBQD1bSliJMVMqv\n1tM/Le1TPz/8aG7v/uZaFtem+EwKH5NvFGN7adyQjMs14PnBCQxex6ebWea9eEXV\nBgmKf7sqCHIqE9Ux0Nsxoad98l+RiO5wds4+5AmIZ9pC4ewzCzsFpUzy8agQZI4k\nV2EOFF6RSaauyS51+Eb42iv/qspIpuh6FKZXqAPRYcz+59qQF1Ia3HzlJmV+Zamx\nNAvubxGLweAubz4AhuEZnRoWLEke18Zq4rIyL7vREzJcP0znYDGpe/EgnBIaR3QL\nnAzyOUcrovpOiUXfKtv5L4i9AseMVaJJ62+EnfbjAwKBgQDf3SSSCUVG6+RCtkek\nIyO7Ngy5UpdPG0WzTDesZd5EgZ0aroeoDA/thWDCAz2xaFiY/a2omGHK1fZBpIE0\nhslVEPMRJFmIxUiFQwW7U7kQQuTwahEYVoAa4gUHLJfs8tFczvy0o1ikiOJa3Yk/\npTwsGDoq8Ilr3qANzM+KtpaOewKBgQC3uKTpC2pLISBYJ6SztNDj9VdufEoqlf2n\nZ4SgL5RtjUXSkr8/5QdQUIDRLnAvaQQdAXaNa05rz9R0NS1EhV+0I1bqr1uSjv9X\nffn0cFkvlqvP3HsJ2aW/mGWTX7p949XVdz82uvUeGICAWlGxec3CSqR3W3fY+NrM\nfvw3NXGZTwKBgGZOy4rOH9IEtmHiqiUSRh8l2XTMkQf/H8CMYZkxSP4n+iOahbbA\nJHtZjFm+X3B3jp4EuVumoHKxjAR96OqXeuWchGleImkGOFlmwvTUk2wiFjzlTIDB\n73PZvZCEyb3pTtNKaWUojbdMM55xRmtG2ZQRUmwTV8priNwlsOflADOHAoGBAIdM\nq+YUSgtaMf+58kmF9/BpViI/1j7Whx8p1TpC0AR97dXNzJq5iFFVeAiVMnk31Zem\n5C/xvqcXP1b1cojr0DdRb0kWK1IbjWBZniKuOoAZbh5+OP5gEviuut91uFnN2ESE\ntUERHkMzuC7OVStmkGltnwFnkzNu0XOYcRYXfblxAoGBANtKLlGDg4r5SX6n0nGv\nhVGTn53u4Y/1fujPku2beW6PiVz9mSua5KVh7lWwcMjppRykxN9H8Wx1IFCYbO0B\nabrxmcPas92PtOZt9EM2dEgCuqeL4qrSqhzQ07L0GoeM9ZzoBp0nviZZDKT48rpj\nMVqsPNwFI80cXxW3sdnCektj\n-----END PRIVATE KEY-----\n";
const char spreadsheetId[] = "1yM9vLFUUWT48zEH9EPKclbbSOWV0icDGXDCWA906m8U";

// Fungsi untuk mengambil waktu saat ini
String getCurrentTime() {
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buffer);
}

// Fungsi untuk menambahkan data ke Google Sheets
void appendToGoogleSheets(int co2, float methane, float temperature, float humidity) {
    FirebaseJson valueRange;
    String currentTime = getCurrentTime(); // Ambil waktu saat ini

    valueRange.add("majorDimension", "COLUMNS");
    valueRange.set("values/[0]/[0]", currentTime);
    valueRange.set("values/[1]/[0]", co2);
    valueRange.set("values/[2]/[0]", methane);
    valueRange.set("values/[3]/[0]", temperature);
    valueRange.set("values/[4]/[0]", humidity);

    FirebaseJson response;
    bool success = GSheet.values.append(&response, spreadsheetId, "Sheet1", &valueRange);

    if (success) {
        Serial.println("Data successfully appended to Google Sheets.");
    } else {
        Serial.printf("Error appending to Google Sheets: %s\n", GSheet.errorReason().c_str());
    }
}

// Fungsi untuk mengirim data menggunakan ESP-NOW
void sendData(int CO2, float methane, float temperature, float humidity) {
  myData.co2 = CO2;             // Menyimpan nilai CO2
  myData.methane = round(methane);     // Membulatkan nilai Metana ke angka bulat
  myData.temperature = temperature;  // Menyimpan nilai Suhu
  myData.humidity = humidity;  // Menyimpan nilai Kelembapan

  // Mengirim data dengan ESP-NOW
  esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.println("Data sent successfully");
  } else {
    Serial.println("Error sending data");
  }
}
// Fungsi untuk menghubungkan ke WiFi
void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid1, password1);

    Serial.print("Connecting to WiFi ");
    for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
        Serial.print("...");
        delay(1000);
    }

    // Jika tidak terhubung, coba SSID kedua
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect. Trying second SSID...");
        WiFi.disconnect(); // Putuskan dari SSID pertama
        WiFi.begin(ssid2, password2);
        for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
            Serial.print("...");
            delay(1000);
        }
    }

    // Periksa status koneksi
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi!");
    } else {
        Serial.println("\nFailed to connect to WiFi.");
    }
}
float getMethanePPM() {
    int sensorValue = analogRead(MQ4_Pin); // Baca nilai analog dari sensor
    float voltage = sensorValue * (V_REF / ADC_RESOLUTION);
    float ppm = (R_0 * (V_REF / voltage - 1)) / 100; // Hitung nilai PPM untuk gas methane
    return ppm;
}

// Sinkronisasi waktu menggunakan NTP
void syncTime() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(2000);
    while (!time(nullptr)) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nTime synchronized!");
}

// Setup awal
// Setup awal
void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Konfigurasi peer
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, receiverMac, 6);
    peerInfo.channel = 0; // Channel default
    peerInfo.encrypt = false;

    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result == ESP_OK) {
        Serial.println("Peer added successfully");
    } else {
        Serial.printf("Failed to add peer: %s\n", esp_err_to_name(result));
    }

    mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
    myMHZ19.begin(mySerial);
    myMHZ19.autoCalibration();
    dht1.begin();

    connectToWiFi(); // Koneksi ke WiFi
    syncTime(); // Sinkronisasi waktu

    // Inisialisasi Google Sheets
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

    // Inisialisasi tampilan OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, OLED_RESET)) {  
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);  
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Initializing...");
    display.display();
}


// Loop utama
void loop() {
    unsigned long currentMillis = millis();

    // Cek apakah sudah 30 detik
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        // Baca sensor dan simpan data ke variabel
        int CO2 = myMHZ19.getCO2();        // Nilai CO2 dari MH-Z19
        float temp1 = dht1.readTemperature();   // Suhu dari DHT22
        float hum1 = dht1.readHumidity();      // Kelembapan dari DHT22
        float methanePPM = getMethanePPM();    // Nilai metana dari MQ-4

        // Cetak data ke Serial
        String currentTime = getCurrentTime(); // Ambil waktu saat ini
        Serial.print(currentTime);
        Serial.print(" - CO2: ");
        Serial.print(CO2);
        Serial.print(" ppm, Methane: ");
        Serial.print((int)round(methanePPM)); // Membulatkan nilai metana dan mengkonversi ke integer
        Serial.print(" ppm, Temp1: ");
        Serial.print(temp1);
        Serial.print(" C, Hum1: ");
        Serial.print(hum1);
        Serial.println(" %");

        // Kirim data menggunakan ESP-NOW
        sendData(CO2, methanePPM, temp1, hum1);

        // Tampilkan data di OLED
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("CO2: ");
        display.print(CO2);
        display.println(" ppm");

        display.print("Methane: ");
        display.print((int)round(methanePPM)); // Membulatkan nilai metana dan mengkonversi ke integer
        display.println(" ppm");

        display.print("Temp: ");
        display.print(temp1);
        display.println(" C");

        display.print("Humidity: ");
        display.print(hum1);
        display.println(" %");

        display.display();

        // Tambahkan data ke Google Sheets
        appendToGoogleSheets(CO2, methanePPM, temp1, hum1);
    }
}
