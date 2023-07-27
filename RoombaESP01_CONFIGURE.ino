#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <SimpleTimer.h>
#include <Roomba.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


//USER CONFIGURED SECTION START//
const char* ssid = "YOUR_WIRELESS_SSID";
const char* password = "YOUR_WIRELESS_SSID";
const char* mqtt_server = "YOUR_MQTT_SERVER_ADDRESS";
const int mqtt_port = YOUR_MQTT_SERVER_PORT;
const char *mqtt_user = "YOUR_MQTT_USERNAME";
const char *mqtt_pass = "YOUR_MQTT_PASSWORD";
const char *mqtt_client_name = "Roomba"; // Client connections can't have the same connection name

//pick an OTA password to use when updating over the air
const char *OTApassword = "YOUR_OTA_PASSWORD";
const int OTAport = 8266;

//USER CONFIGURED SECTION END//


WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;
Roomba roomba(&Serial, Roomba::Baud115200);


// Variables
bool boot = true;
long battery_Current_mAh = 0;
long battery_Voltage = 0;
long battery_Total_mAh = 0;
long battery_percent = 0;
char battery_percent_send[50];
char battery_Current_mAh_send[50];
uint8_t tempBuf[10];

//Functions

void setup_wifi() 
{
  WiFi.hostname(mqtt_client_name);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
  }
}

void reconnect() 
{
  // Loop until we're reconnected
  int retries = 0;
  while (!client.connected()) 
  {
    if(retries < 50)
    {
      // Attempt to connect
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass, "roomba/status", 0, 0, "Dead Somewhere")) 
      {
        // Once connected, publish an announcement...
        if(boot == false)
        {
          client.publish("checkIn/roomba", "Reconnected"); 
        }
        if(boot == true)
        {
          client.publish("checkIn/roomba", "Rebooted");
          boot = false;
        }
        // ... and resubscribe
        songs();
        ArduinoOTA.handle();
        client.subscribe("roomba/commands");
      } 
      else 
      {
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    if(retries >= 50)
    {
    ESP.restart();
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  String newTopic = topic;
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  if (newTopic == "roomba/commands") 
  {
    if (newPayload == "test")
    {
      client.publish("roomba/status", "Test New");
    }
    if (newPayload == "start")
    {
      startCleaning();
    }
    if (newPayload == "stop")
    {
      stopCleaning();
    }
    if (newPayload == "restart")
    {
      client.publish("roomba/status", "Rebooting");
      ESP.restart();
    }
    if (newPayload == "installsongfour")
    {
      songs();

    }
    if (newPayload == "songone")
    {
      playSongOne();
    }
    if (newPayload == "songtwo")
    {
      playSongTwo();
    }
    if (newPayload == "songthree")
    {
      playSongThree();
    }
  }
}

void startCleaning()
{
  Serial.write(128);
  delay(50);
  Serial.write(131);
  delay(50);
  Serial.write(135);
  client.publish("roomba/status", "Cleaning");
}

void stopCleaning()
{
  Serial.write(128);
  delay(50);
  Serial.write(131);
  delay(50);
  Serial.write(143);
  client.publish("roomba/status", "Returning");
}

void playSongOne()
{
  Serial.write(128);
  delay(50);
  Serial.write(131);
  delay(50);
  byte Playsong1[2]={141,0}; //Strange Encounters
  Serial.write(Playsong1,2);
  client.publish("roomba/status", "Singing song One");
}

void playSongTwo()
{
  Serial.write(128);
  delay(50);
  Serial.write(131);
  delay(50);
  byte Playsong2[2]={141,1}; //Strange Encounters
  Serial.write(Playsong2,2);
  client.publish("roomba/status", "Singing song Two");
}

void playSongThree()
{
  Serial.write(128);
  delay(50);
  Serial.write(131);
  delay(50);
  byte Playsong3[2]={141,2}; //Strange Encounters
  Serial.write(Playsong3,2);
  delay(5000);
  byte Playsong4[2]={141,3}; //Strange Encounters
  Serial.write(Playsong4,2);
  client.publish("roomba/status", "Singing song Three");
}

void sendInfoRoomba()
{
  roomba.start(); 
  roomba.getSensors(21, tempBuf, 1);
  battery_Voltage = tempBuf[0];
  delay(50);
  roomba.getSensors(25, tempBuf, 2);
  battery_Current_mAh = tempBuf[1]+256*tempBuf[0];
  delay(50);
  roomba.getSensors(26, tempBuf, 2);
  battery_Total_mAh = tempBuf[1]+256*tempBuf[0];
  if(battery_Total_mAh != 0)
  {
    int nBatPcent = 100*battery_Current_mAh/battery_Total_mAh;
    String temp_str2 = String(nBatPcent);
    temp_str2.toCharArray(battery_percent_send, temp_str2.length() + 1); //packaging up the data to publish to mqtt
    client.publish("roomba/battery", battery_percent_send);
  }
  if(battery_Total_mAh == 0)
  {  
    client.publish("roomba/battery", "NO DATA");
  }
  String temp_str = String(battery_Voltage);
  temp_str.toCharArray(battery_Current_mAh_send, temp_str.length() + 1); //packaging up the data to publish to mqtt
  client.publish("roomba/charging", battery_Current_mAh_send);
}

void setup() 
{
  Serial.begin(115200);
  Serial.write(129);
  delay(50);
  Serial.write(11);
  delay(50);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  timer.setInterval(5000, sendInfoRoomba);

  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setHostname(mqtt_client_name);
  ArduinoOTA.setPassword((const char *)OTApassword);
  ArduinoOTA.begin();
}

void songs() {
  byte Song0[17]={140,0,7,31,60,43,55,55,50,67,45,79,40,91,35,103,30};
  byte Song1[35]={140,1,16,36,20,37,20,0,40,36,20,37,20,0,30,36,20,37,20,0,20,36,20,37,20,0,10,36,20,37,20,0,5,38,64}; //Jaws
  byte Song2[13]={140,2,5,79,48,81,48,77,48,65,50,72,64}; //Strange Encounters 1
  byte Song3[13]={140,3,5,69,48,71,48,67,48,55,48,62,64}; //Strange Encounters
  Serial.write(128);
  delay(50);
  Serial.write(131);
  delay(50);
  Serial.write(Song0,17);
  Serial.write(Song1,35);
  Serial.write(Song2,13);
  Serial.write(Song3,13);
  client.publish("checkIn/roomba", "Songs Installed");
}


void loop() 
{
  ArduinoOTA.handle();
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
  timer.run();
}
