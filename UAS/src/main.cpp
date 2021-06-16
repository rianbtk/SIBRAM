#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>

#define sensorLDR D0
#define sensorSoilMoisture A0
#define PUMP_PUPUK D5
#define PUMP_AIR D6
#define PUMP_INSEK D7
#define MSG_BUFFER_SIZE (50)

const char *ssid = "SINIO123";//silakan disesuaikan sendiri
const char *password = "siniorek";//silakan disesuaikan sendiri
const char *mqtt_server = "ec2-100-24-67-173.compute-1.amazonaws.com";
const char *mqtt_name = "jti";
const char *mqtt_pass = "12345678";

WiFiClient espClient;
PubSubClient client(espClient);
SimpleDHT22 dht22(D1);

int nilaiSensorSoil;
int nilaiSensor;
char msg[MSG_BUFFER_SIZE];
long now = millis();
long lastMeasure = 0;
int value = 0;
unsigned long lastMsg = 0;

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(),mqtt_name,mqtt_pass))
    {
      Serial.println("connected");
      client.subscribe("sibram-pupuk");
      client.subscribe("sibram-air");
      client.subscribe("sibram-insek");

    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void pupuk()
{
  digitalWrite(PUMP_INSEK, HIGH);
  digitalWrite(PUMP_AIR, HIGH);
  digitalWrite(PUMP_PUPUK, LOW);
}

void air()
{
  digitalWrite(PUMP_PUPUK, HIGH);
  digitalWrite(PUMP_INSEK, HIGH);
  digitalWrite(PUMP_AIR, LOW);
}

void insek()
{
  digitalWrite(PUMP_AIR, HIGH);
  digitalWrite(PUMP_PUPUK, HIGH);
  digitalWrite(PUMP_INSEK, LOW);
}

void pumpOff()
{
  digitalWrite(PUMP_AIR, HIGH);
  digitalWrite(PUMP_PUPUK, HIGH);
  digitalWrite(PUMP_INSEK, HIGH);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();

  if (String(topic) == "sibram-pupuk")
  {
    if (messageTemp == "true")
    {
      pumpOff();
      pupuk();
    }
    else
    {
      pumpOff();
    }
  }
  else if (String(topic) == "sibram-air")
  {
    if (messageTemp == "true")
    {
      pumpOff();
      air();
    }
    else
    {
      pumpOff();
    }
  }
  else
  {
    if (messageTemp == "true")
    {
      pumpOff();
      insek();
    }
    else
    {
      pumpOff();
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Mqtt Node-RED");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pumpOff();
  pinMode(PUMP_PUPUK, OUTPUT);
  pinMode(PUMP_AIR, OUTPUT);
  pinMode(PUMP_INSEK, OUTPUT);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  if (!client.loop())
  {
    client.connect("ESP8266Client");
  }
  now = millis();
  if (now - lastMeasure > 2000)
  {
    lastMeasure = now;
    int err = SimpleDHTErrSuccess;
    nilaiSensor = analogRead(sensorLDR);
    nilaiSensorSoil = analogRead(sensorSoilMoisture);
    nilaiSensorSoil = map(nilaiSensorSoil, 1023, 0, 0, 100);
    byte temperature = 0;
    byte humidity = 0;
    if ((err = dht22.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
    {
      Serial.print("Pembacaan DHT22 gagal, err=");
      Serial.println(err);
      delay(1000);
      return;
    }
    static char temperatureTemp[7];
    dtostrf(temperature, 4, 2, temperatureTemp);
    Serial.println(temperatureTemp);
    client.publish("sibram-suhu", temperatureTemp);
    

    static char ldrScore[7];
    dtostrf(nilaiSensor, 4, 2, ldrScore);
    Serial.println(ldrScore);
    client.publish("sibram-ldr", ldrScore);

    static char SoilScore[7];
    dtostrf(nilaiSensorSoil, 4, 2, SoilScore);
    Serial.println(SoilScore);
    client.publish("sibram-soil", SoilScore);
  }
}