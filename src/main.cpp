// https://github.com/jandrassy/WiFiEspAT
// https://arduino-pico.readthedocs.io/en/latest/index.html
#include <Arduino.h>
#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <config.h>
#include <pinout.h>
#include <Ticker.h>

float core_temp = 0;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

Adafruit_NeoPixel pixels(1, LED_RGB, NEO_GBR);

struct STATUS
{
  bool state = false;
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t brightness = 0;
  bool gp20 = 0;
  bool gp21 = 0;
  bool gp22 = 0;
  float core_temp = 0;
}status;

void sendState(void);

void setRGB(){ 
    
    if(status.state){
      pixels.setPixelColor(0, pixels.Color( map(status.b, 0, 255, 0, status.brightness), 
                                            map(status.g, 0, 255, 0, status.brightness), 
                                            map(status.r, 0, 255, 0, status.brightness)
                                          )
                          );  
    }
    else{
      pixels.setPixelColor(0,0,0,0);
    }

    pixels.show();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String top = String(topic);
  String pay = "";

  if(top.indexOf("rgb")>0){
      StaticJsonDocument<256> doc;
      deserializeJson(doc, payload, length);
      serializeJsonPretty(doc, Serial);
      
      if(doc["state"] == "ON")  status.state = true;
      else status.state = false;

      if(doc["color"]){
        status.r = doc["color"]["r"];
        status.g = doc["color"]["g"];
        status.b = doc["color"]["b"]; 
      }
      
      if(doc["brightness"]) status.brightness = doc["brightness"];

      setRGB();

      sendState();

  }
  else{
    for (int i = 0; i < length; i++) {
      pay += (char)payload[i];
    }
    Serial.println(pay);
  }
}

void sendState() {
  StaticJsonDocument<256> doc;

  doc["state"] = (status.state) ? "ON" : "OFF";
  doc["brightness"] = status.brightness;
  
  JsonObject color = doc.createNestedObject("color");
  color["r"] = status.r;
  color["g"] = status.g;
  color["b"] = status.b;
  
  JsonObject buttons = doc.createNestedObject("buttons");
  buttons["GP20"] = (status.gp20)? "ON" : "OFF";
  buttons["GP21"] = (status.gp21)? "ON" : "OFF";
  buttons["GP22"] = (status.gp22)? "ON" : "OFF";

  // JsonArray buttons = doc.createNestedArray("buttons");
  // buttons[0] = status.gp20;
  // buttons[1] = status.gp21;
  // buttons[2] = status.gp22;

  doc["core_temp"] = status.core_temp;
  
  char buffer[256];
  serializeJson(doc, buffer);

  client.publish(STATE_TOPIC, buffer, true);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "MakerPico-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS, LWT_TOPIC,1, true,"offline")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(LWT_TOPIC, "online", true);
      // ... and resubscribe
      client.subscribe(SUB_RGB_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_wifi(){
  
  WiFi.init(Serial1);

  // No necesario, ver https://github.com/JAndrassy/WiFiEspAT#persistent-wifi-connection
  // WiFi.begin(ssid, password);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // waiting for connection to Wifi network set with the SetupWiFiConnection sketch
  Serial.println("Waiting for connection to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println();

  IPAddress ip = WiFi.localIP();
  Serial.println();
  Serial.print("Connected to WiFi network, IP: ");
  Serial.println(ip);
}

void read_core_temp(){
  status.core_temp = analogReadTemp();
  sendState();
}

void read_buttons(){
  bool send = false;
  if(digitalRead(BUTTON1) != status.gp20) { status.gp20 = digitalRead(BUTTON1); send = true; }
  if(digitalRead(BUTTON2) != status.gp21) { status.gp21 = digitalRead(BUTTON2); send = true; }
  if(digitalRead(BUTTON3) != status.gp22) { status.gp22 = digitalRead(BUTTON3); send = true; }
  if(send) sendState();
}

Ticker t_read_core_temp(read_core_temp, 10000, 0, MILLIS);
Ticker t_read_buttons(read_buttons, 100, 0, MILLIS);

void setup() {

  Serial.begin(115200);
  delay(5000);
  // while (!Serial);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);

  Serial1.setPinout(16, 17);
  
  Serial1.begin(115200);

  setup_wifi();

  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);

  pixels.begin();
  setRGB();

  // Por interrupcion anda joya, pero bloquea al ESP
  // attachInterrupt(digitalPinToInterrupt(BUTTON1), read_buttons, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(BUTTON2), read_buttons, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(BUTTON3), read_buttons, CHANGE);

  t_read_core_temp.start();
  t_read_buttons.start();
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  t_read_core_temp.update();
  t_read_buttons.update();

}
