#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi
const char *ssid = "GODAnubis"; // Enter your WiFi name
const char *password = "A######";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";// broker address
const char *topic = "esp32/plant_automation"; // define topic 
const char *mqtt_username = "emqx"; // username for authentication
const char *mqtt_password = "public";// password for authentication
const int mqtt_port = 1883;// port of MQTT over TCP

// Create an instance of the MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

//make variables for each pin
const int soilMoisturePin = 34;  // GPIO pin to which soil moisture sensor is connected
const int ldrPin = 35;           // GPIO pin to which LDR module is connected
const int motorPin = 12;         // GPIO pin to which the motor is connected
const int lightPin = 13;         // GPIO pin to which the light is connected
const int fanPin = 4;           // GPIO pin to which the fan is connected
const int dhtPin = 14;           // GPIO pin to which DHT11 sensor is connected
const int buzzerPin = 5;
const int LCDAddress = 0x27; // I2C address of your LCD display (SDA->D21 , SLC->D22)


DHT dht(dhtPin, DHT11);          // Create a DHT object
LiquidCrystal_I2C lcd(LCDAddress, 16, 2); // Adjust the columns (16) and rows (2) based on your LCD


// SETup#############################################################################
void setup() {
  //start serial communication
  Serial.begin(115200);
  lcd.init();                      // Initialize the LCD display
  lcd.backlight();                 // Turn on the backlight
  lcd.setCursor(0, 0);
  lcd.print("Temperature:      C");
  lcd.setCursor(0, 1);
  lcd.print("Humidity:  %");

 // connecting to a WiFi network
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println("Connecting to WiFi..");
 }
 Serial.println("Connected to the WiFi network");

  //connecting to a mqtt broker
 client.setServer(mqtt_broker, mqtt_port);
 client.setCallback(callback);
 while (!client.connected()) {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
         Serial.println("Public emqx mqtt broker connected");
     } else {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(2000);
     }
 }
 // publish and subscribe
 client.publish(topic, "Hi EMQX I'm ESP32 ^^");
 client.subscribe(topic);
  //set output pins
  pinMode(motorPin, OUTPUT); // Set motor pin as output
  pinMode(lightPin, OUTPUT); // Set light pin as output
  pinMode(fanPin, OUTPUT);   // Set fan pin as output
  pinMode(buzzerPin, OUTPUT);

  dht.begin();               // Initialize the DHT sensor
  Wire.begin();

}

void callback(char *topic, byte *payload, unsigned int length) {
 Serial.print("Message arrived in topic: ");
 Serial.println(topic);
 Serial.print("Message:");
 for (int i = 0; i < length; i++) {
     Serial.print((char) payload[i]);
 }
 Serial.println();
 Serial.println("-----------------------");
}

//loop#################################################################
void loop() {
  // Read soil moisture value from the sensor
  int soilMoisture = analogRead(soilMoisturePin);
  // Read light intensity from the LDR module
  int lightIntensity = analogRead(ldrPin);

  // Map the analog values (0-4095) to moisture percentage (0-100) and light intensity (0-100)
  int moisturePercentage = map(soilMoisture, 4095, 0, 0, 100);
  int lightPercentage = map(lightIntensity, 4095, 0, 0, 100);

  // Read temperature and humidity from the DHT11 sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  Serial.print("Soil Moisture Percentage %: ");
  Serial.println(moisturePercentage);
  Serial.print("Light Intensity Percentage %: ");
  Serial.println(lightPercentage);
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");


  // Update the LCD display with the sensor data
  lcd.setCursor(6, 0);
  lcd.print(temperature);
  lcd.setCursor(10, 1);
  lcd.print(humidity);
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(humidity);
  lcd.print("%");
  lcd.setCursor(0, 0);
  lcd.print("Temper: ");
  lcd.print(temperature);
  lcd.print(" C");


// Connect to MQTT broker**********************
  if (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
    }
  }
  // Check soil moisture condition
  if (moisturePercentage < 20) { 
    digitalWrite(motorPin, LOW);
    Serial.println("Watering the plant!");
  } else {
    digitalWrite(motorPin, HIGH);
    Serial.println("Soil Moisture is nomal!!");
  }

  // Check light condition
  if (lightPercentage < 70) {
    digitalWrite(lightPin, HIGH);
    //digitalWrite(buzzerPin, HIGH);
    //delay(500);
    //digitalWrite(buzzerPin, LOW);
    Serial.println("Light intensity is not enough. Turn on Artificial light!");
  } else {
    digitalWrite(lightPin, LOW);
    Serial.println("Light intensity is enough");
  }

  // Check temperature condition
  if (temperature > 30) {
    digitalWrite(fanPin, LOW);
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    Serial.println("Temperature is high. Turning on the fan!");
  } else {
    digitalWrite(fanPin, HIGH);
    Serial.println("Temperature is normal");
  }
  
// Publish sensor data to MQTT topics
  client.publish("plant_automation/temperature", String(temperature).c_str());
  client.publish("plant_automation/humidity", String(humidity).c_str());
  client.publish("plant_automation/light", String(lightIntensity).c_str());
  client.publish("plant_automation/moisture", String(soilMoisture).c_str());
  client.publish("plant_automation/moisture/presentage", String(moisturePercentage).c_str());
  client.publish("plant_automation/light/presentage", String(lightPercentage).c_str());
  
  delay(5000); // Delay for 1 second
}
