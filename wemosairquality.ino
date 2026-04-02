// Program Monitoring Kualitas Udara dengan Wemos D1 Mini + Blynk
// Sensor MQ135 terhubung ke pin A0 (analog)
// Sensor DHT22 terhubung ke pin D4 (digital)
// Display OLED 0.96" (SSD1306, I2C) - alamat I2C 0x3C
// Data dikirim ke aplikasi Blynk untuk monitoring remote
// Menggunakan Arduino IDE dengan board ESP8266
// Ditambahkan fitur on/off sistem via Blynk (tombol switch di V4) dan tombol fisik off (pin D5)

// Library yang diperlukan:
// - DHT sensor library (by Adafruit)
// - Adafruit SSD1306
// - Adafruit GFX Library
// - Blynk (install via Library Manager: cari "Blynk")
// - ESP8266WiFi (built-in)
#define BLYNK_TEMPLATE_ID "TMPL64LL87GwX"
#define BLYNK_TEMPLATE_NAME "Kualitas Udara"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> 
#include <DHT.h>
#include <ESP8266WiFi.h>   
#include <BlynkSimpleEsp8266.h>
#include <SoftwareSerial.h>

// Definisi pin
#define MQ135_PIN A0  // Pin analog untuk MQ135
#define DHT_PIN D4    // Pin digital untuk DHT22
#define DHT_TYPE DHT22  // Tipe sensor DHT 
#define BUTTON_PIN D5  // Pin digital untuk tombol fisik off (gunakan pull-up internal)

// Definisi OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Inisialisasi DHT
DHT dht(DHT_PIN, DHT_TYPE);

// Kredensial WiFi dan Blynk
char auth[] = "PF6M-jOSK63c850QrQ0CdK_OoyqNLPLg";  // Ganti dengan Auth Token dari aplikasi Blynk
char ssid[] = "Hujan";         // Ganti dengan nama WiFi Anda
char pass[] = "11111111";     // Ganti dengan password WiFi Anda

// Virtual Pins untuk Blynk (sesuaikan di aplikasi Blynk)
// V0: MQ135 Analog Value
// V1: PPM Estimate
// V2: Temperature
// V3: Humidity
// V4: System On/Off (Switch Widget: 1 = On, 0 = Off)
#define VPIN_MQ135 V0
#define VPIN_PPM V1
#define VPIN_TEMP V2
#define VPIN_HUM V3
#define VPIN_SYSTEM_ON V4
#define VPIN_SYSTEM_ON5 V5

// Variabel sensor
float mq135Value = 0;
float ppmEstimate = 0;
float temperature = 0;
float humidity = 0;

// Variabel kontrol sistem
bool isSystemOn = true;  // Default sistem menyala
bool isSystemOn5 = true;  // Default sistem menyala
bool lastButtonState = HIGH;  // State tombol sebelumnya (HIGH karena pull-up)

// Konstanta untuk MQ135 (sesuaikan berdasarkan kalibrasi dan gas target)
// Untuk CO2: a = 116.6020682, b = -2.769034857
// R_L = resistor load (ohm), sesuaikan dengan hardware Anda (misal 1000 ohm)
// R0 = resistansi baseline di udara bersih (ohm), kalibrasi diperlukan
#define RL 1000.0  // Resistor load (sesuaikan)
#define R0 10000.0  // Nilai R0 dari kalibrasi (ubah berdasarkan pengukuran Anda)
#define VCC 3.3  // Tegangan catu daya (sesuaikan: 3.3V atau 5V)
#define A_CO2 116.6020682  // Konstanta a untuk CO2
#define B_CO2 -2.769034857  // Konstanta b untuk CO2

//-----------------
SoftwareSerial wemos(14,12);

// Handler untuk tombol on/off di Blynk
BLYNK_WRITE(VPIN_SYSTEM_ON) {
  isSystemOn = param.asInt();  // 1 = On, 0 = Off
  Serial.print("System Status (Blynk): ");
  Serial.println(isSystemOn ? "ON" : "OFF");
}

// Handler untuk tombol on/off di Blynk
BLYNK_WRITE(VPIN_SYSTEM_ON5) {
  isSystemOn5 = param.asInt();  // 1 = On, 0 = Off
  Serial.print("System Status (Blynk): ");
  Serial.println(isSystemOn5 ? "ON5" : "OFF5");
}

void setup() {
  // Inisialisasi Serial
  Serial.begin(115200);
  wemos.begin(9600);
  // Inisialisasi DHT
  dht.begin();
  
  // Inisialisasi tombol fisik
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Pull-up internal
  
  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Air quality");
  display.println("Connecting to Blynk...");
  display.display();
  
  // Koneksi ke WiFi dan Blynk
  Blynk.begin(auth, ssid, pass);
  
  // Set initial value untuk tombol on/off di Blynk
  Blynk.virtualWrite(VPIN_SYSTEM_ON, isSystemOn);
  
  // Konfirmasi koneksi
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connected to Blynk!");
  display.display();
  delay(2000);
}

void loop() {
  // Jalankan Blynk
  Blynk.run();
  
  // Baca state tombol fisik (debounce sederhana)
  bool buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {  // Tombol ditekan (falling edge)
    isSystemOn = false;  // Matikan sistem
    Blynk.virtualWrite(VPIN_SYSTEM_ON, isSystemOn);  // Update Blynk
    Serial.println("System turned OFF via physical button");
    delay(200);  // Debounce delay
  }
  lastButtonState = buttonState;
  
  // Jika sistem dimatikan, tampilkan status dan skip monitoring
  if (!isSystemOn) {
    wemos.print('a');
  }
  else if(isSystemOn){
    wemos.print('A');
  }
  if (!isSystemOn5) {
    wemos.print('b');
  }
  else if(isSystemOn5){
    wemos.print('B');
  }

  // Baca nilai dari MQ135
  mq135Value = analogRead(MQ135_PIN);
  float voltage = (mq135Value / 1023.0) * 3.3;  // Konversi ADC ke tegangan
  
  // Hitung R_s (resistansi sensor)
  float V_RL = voltage;  // V_RL adalah tegangan di resistor load
  float R_s = ((VCC - V_RL) / V_RL) * RL;
  
  // Hitung PPM menggunakan rumus logaritmik untuk CO2
  float ratio = R_s / R0;
  ppmEstimate = A_CO2 * pow(ratio, B_CO2);
  
  // Baca suhu dan kelembaban dari DHT22
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    temperature = 0;
    humidity = 0;
  }
  
  // Tampilkan data di Serial Monitor
  Serial.print("MQ135 Analog: ");
  Serial.print(mq135Value);
  Serial.print(" | Voltage: ");
  Serial.print(voltage);
  Serial.print(" | R_s: ");
  Serial.print(R_s);
  Serial.print(" | PPM Estimate: ");
  Serial.print(ppmEstimate);
  Serial.print(" | Temp: ");
  Serial.print(temperature);
  Serial.print(" C | Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  
  // Kirim data ke Blynk
  Blynk.virtualWrite(VPIN_MQ135, mq135Value);
  Blynk.virtualWrite(VPIN_PPM, ppmEstimate);
  Blynk.virtualWrite(VPIN_TEMP, temperature);
  Blynk.virtualWrite(VPIN_HUM, humidity);
  
  // Tampilkan data di OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Kualitas Udara");
  display.setCursor(0, 10);
  display.print("MQ135: ");
  display.print(mq135Value);
  display.setCursor(0, 20);
  display.print("PPM Est: ");
  display.print(ppmEstimate, 1);
  display.setCursor(0, 30);
  display.print("Temp: ");
  display.print(temperature, 1);
  display.print(" C");
  display.setCursor(0, 40);
  display.print("Humidity: ");
  display.print(humidity, 1);
  display.print(" %");
  display.setCursor(0, 50);
  display.print("System: ");
  display.print(isSystemOn ? "ON" : "OFF");
  display.display();
  
  // Delay 2 detik
  delay(2000);
}
