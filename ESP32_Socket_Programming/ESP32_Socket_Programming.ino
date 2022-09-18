#include <Arduino.h>
#include <iostream>
using namespace std;
#include <WiFi.h>
String str;
// char received;
const char *ssid = "Trace Paradox";
const char *password = "meowmeow";

const uint16_t port = 8090;
const char *host = "192.168.93.160";

void setup()
{

  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("...");
  }

  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  WiFiClient client;

  if (!client.connect(host, port))
  {

    Serial.println("Connection to host failed");

    delay(1000);
    return;
  }

  //Serial.println("Connected to server successful!");

  delay(1000);
  uint8_t ReceiveBuffer[1024];
  while (client.connected() && client.available())
  {
    // Serial.println("Yup it is");
    char received = client.read();
    // client.write(received);
    String str;
    str = str + received;
    // cout << str;
    Serial.print(str);
  }
  Serial.println();
  //Serial.print(str);
  // client.print(received);
  // delay(500);
  //   if (client.available());
  //   Serial.println("It is Available!");
  //   char received = client.read();
  //   Serial.println(received);
  // Serial.println("Disconnecting...");
  client.stop();

  delay(1000);
}
