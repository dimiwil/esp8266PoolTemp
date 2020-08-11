#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define MQTT_VERSION MQTT_VERSION_3_1_1
#define ONE_WIRE_BUS 2   /* Digitalport Pin 2 */

// Wifi:
const char* WIFI_SSID = "Dimis Netz";
const char* WIFI_PASSWORD = "!Starwars1301";
WiFiClient wifiClient;

// MQTT:
const PROGMEM char* MQTT_CLIENT_ID = "pool_temp1";
const PROGMEM char* MQTT_SERVER_IP = "192.168.178.118";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "";
const PROGMEM char* MQTT_PASSWORD = "";

const PROGMEM char* MQTT_SENSOR_TOPIC = "pool/temperature";
PubSubClient client(wifiClient);

// sleeping time
const PROGMEM uint16_t SLEEPING_TIME_IN_SECONDS = 300; // 300s = 5min

// Sensor
OneWire ourWire(ONE_WIRE_BUS);
DallasTemperature sensors(&ourWire);

// Battery
ADC_MODE(ADC_VCC);

// function called to publish the temperature
void publishData(float temperature, int battery) {
  StaticJsonDocument<200> doc;
  JsonObject root = doc.to<JsonObject>();
  root["temperature"] = (String) temperature;
  root["battery"] = (String) battery;
  serializeJsonPretty(root, Serial);
  Serial.println("");
  /*
   * {
   *   "temperature": "23.39",
   *   "battery": "3400"
   * }
   */
   char data[200];
   serializeJson(root, data, measureJson(root) +1);
   client.publish(MQTT_SENSOR_TOPIC, data, true);
   yield();
}

// function called when a MQTT message arrived
void callback(char* topic, byte* payload, unsigned int length) {}

void reconnect(){
  // Loop until we're reconnected
  while(!client.connected()) {
    Serial.println("INFO: Attempting MQTT connection ...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("INFO: Connected");
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.print("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void adresseAusgeben(void) {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];

  Serial.print("Suche 1-Wire-Devices ...\n\r");
  while(ourWire.search(addr)) {
      Serial.print("\n\r\n\r1-Wire-Device gefunden mit Adresse:\n\r");
      for ( i = 0; i < 8; i++) {
        Serial.print("0x");
        if (addr[i] < 16) {
          Serial.print('0');
        }
        Serial.print(addr[i], HEX);
        if (i < 7) {
          Serial.print(", ");
        }
      }
      if (OneWire :: crc8(addr, 7) != addr[7]) {
        Serial.print("CRC is not valid!\n\r");
      }
  }
  Serial.println();
  ourWire.reset_search();
  return;
}

void setup() {
  // init the serial
  Serial.begin(115200);
  
  Serial.println("INFO: Initialisiere Temperaturmessung...");
  delay(1000);
  sensors.begin();

  adresseAusgeben();

  // init the WiFi connection
  Serial.println();
  Serial.println();
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);


  pinMode(LED_BUILTIN, OUTPUT);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.println("INFO: IP address: ");
  Serial.println(WiFi.localIP());
  
  digitalWrite(LED_BUILTIN, LOW);

  // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Reading battery output
  int battery = ESP.getVcc();

  // Reading temperature
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  if (isnan(temp)) {
    Serial.println("ERROR: Failed to read from sensors");
    return;
  } else {
    publishData(temp, battery);
  }

  Serial.println("INFO: Closing the MQTT connection");
  client.disconnect();

  Serial.println("INFO: Closing the WiFi connection");
  WiFi.disconnect();

  if (battery < 3100) {
    ESP.deepSleep(ESP.deepSleepMax());
  } else {
    ESP.deepSleep(SLEEPING_TIME_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
  }
  delay(1000); // wait for sleep to happen
}
