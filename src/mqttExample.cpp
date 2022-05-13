#include "more4iot.h"
//#include "secrets.h"

#include "WiFi.h"
#include "WiFiUdp.h"

#define TIME_TO_SEND 10000

#define UUID "deviceXX"

// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port);

char ssid[] = "";
char password[] = "";
char uuid[] = UUID;

// more4iot coap server ip and port
IPAddress ip(192,168,0,186);
int port = 5683;

// UDP and MORE4IoT CoAP class
WiFiUDP Udp;
More4iotCoap md(Udp, ip, port);

// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port)
{
  Serial.println("[Coap Response]");

  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  Serial.println(p);
}

void setup()
{
  Serial.begin(115200);
  delay(10);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // client response callback.
  // this endpoint is single callback.
  // Serial.println("Setup Response Callback");
  md.response(callback_response);

  // start coap client
  md.connect();
}

void loop()
{

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to AP ...");
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    return;
  }

  md.newDataPacket(uuid, 1.0, -2.0);
  md.addField("temperature", 25);
  md.addField("humidity", 60);
  md.send();

  delay(TIME_TO_SEND);
  md.loop();
}