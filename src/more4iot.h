#ifndef more4iot_arduino_sdk_h
#define more4iot_arduino_sdk_h

#define DATA_HEADER_FIELDS_AMT 3

#include <vector>
#include <ArduinoJson.h>
#include "ArduinoJson/Polyfills/type_traits.hpp"

#include <ArduinoHttpClient.h>
#include <PubSubClient.h>
#include <coap-simple.h>

class More4iotDefaultLogger
{
public:
  static void log(const char *msg);
};
using Logger = More4iotDefaultLogger;

class DataAttribute
{
  friend class DataObjectImpl;
  friend class Action;

public:
  inline DataAttribute()
      : dataType(TYPE_NONE), dataName(NULL), dataValue() {}

  inline DataAttribute(const char *name, int value)
      : dataType(TYPE_INT), dataName(name), dataValue() { dataValue.integer = value; }

  inline DataAttribute(const char *name, bool value)
      : dataType(TYPE_BOOL), dataName(name), dataValue() { dataValue.boolean = value; }

  inline DataAttribute(const char *name, double value)
      : dataType(TYPE_REAL), dataName(name), dataValue() { dataValue.real = value; }

  inline DataAttribute(const char *name, const char *value)
      : dataType(TYPE_STR), dataName(name), dataValue() { dataValue.str = value; }

  const char *toStringValueStr()
  {
    if (dataType == TYPE_STR)
    {
      return (String(dataName) + String(": ") + String(dataValue.str)).c_str();
    }
    return NULL;
  }

  bool serializeDataAt(JsonObject &jsonObj) const;

private:
  union DataAttributeValueUnion
  {
    const char *str;
    bool boolean;
    int integer;
    double real;
  };

  enum DataAttributeTypeEnum
  {
    TYPE_NONE,
    TYPE_BOOL,
    TYPE_INT,
    TYPE_REAL,
    TYPE_STR,
  };

  DataAttributeTypeEnum dataType;
  const char *dataName;
  DataAttributeValueUnion dataValue;
};

class DataObjectImpl
{
protected:
  std::vector<DataAttribute> dataHeader;
  std::vector<DataAttribute> dataFields;

  inline size_t jsonObjectSize(size_t size) { return size * sizeof(ARDUINOJSON_NAMESPACE::VariantSlot); }

  String getDataPacketJson()
  {
    size_t bufferSize = jsonObjectSize(dataHeader.size() + dataFields.size() + 1);
    DynamicJsonDocument jsonBuffer(bufferSize);
    JsonObject jsonRoot = jsonBuffer.to<JsonObject>();

    for (DataAttribute d : dataHeader)
    {
      if (d.serializeDataAt(jsonRoot) == false)
      {
        Serial.println("unable to serialize data");
        return "";
      }
    }
    JsonObject jsonData = jsonRoot.createNestedObject("data");
    for (DataAttribute d : dataFields)
    {
      if (d.serializeDataAt(jsonData) == false)
      {
        Serial.println("unable to serialize data");
        return "";
      }
    }

    String payload;
    serializeJson(jsonRoot, payload);
    return payload;
  }

public:
  bool newDataPacket(const char *uuid)
  {
    dataHeader.clear();
    dataFields.clear();
    dataHeader.push_back(DataAttribute("uuid", uuid));
    Serial.println("data packet created...");
    return true;
  }

  bool newDataPacket(const char *uuid, double lon = 0.0, double lat = 0.0)
  {
    dataHeader.clear();
    dataFields.clear();
    dataHeader.push_back(DataAttribute("uuid", uuid));
    dataHeader.push_back(DataAttribute("lat", lat));
    dataHeader.push_back(DataAttribute("lon", lon));
    Serial.println("data packet created...");
    return true;
  }

  template <typename T>
  bool addField(const char *nameField, T value)
  {
    dataFields.push_back(DataAttribute(nameField, value));
    return true;
  }
};

class Action {
public:
  inline Action(){}
  inline ~Action() {}

  template<typename T>
  T getData(const uint8_t *payload, const char *key){
    if (payload == nullptr) {
      return false;
    }

    StaticJsonDocument<64> filter;
    DynamicJsonDocument json(1024);
    filter["data"][key] = true;
    DeserializationError error = deserializeJson(json, payload, DeserializationOption::Filter(filter));
    
    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    }

    // json value to type from T
    JsonObject obj = json.as<JsonObject>();
    return obj["data"][key].as<T>();
  }

  template<typename T>
  T getCommand(const uint8_t *payload, const char *key){
    if (payload == nullptr) {
      return false;
    }

    StaticJsonDocument<64> filter;
    DynamicJsonDocument json(1024);
    // filter for two fields EX: commands/light or commands/water-pump
    filter["commands"][key] = true;
    DeserializationError error = deserializeJson(json, payload, DeserializationOption::Filter(filter));
    
    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    }

    // json value to type from T
    JsonObject obj = json.as<JsonObject>();
    return obj["commands"][key].as<T>();
  }
};

class More4iot : public DataObjectImpl, public Action
{
public:
  More4iot(){};
  ~More4iot(){};
  bool connect(){};
  virtual bool connect(const char *host, int port){};
  virtual inline void disconnect(){};
  virtual inline bool connected(){};
  virtual bool send(){};
  virtual void loop(){};
};

class More4iotMqtt : public More4iot
{
private:
  PubSubClient mqttClient;
  String topicPublish = "input";
  String user = "more4iot";
  String pass = "1234";
  IPAddress ip;
  int port;


public:
  inline More4iotMqtt(Client &client, IPAddress &ip, int port = 1883)
      : mqttClient(client), ip(ip), port(port){}
  inline ~More4iotMqtt() {}

  bool connect()
  {
    if (!ip)
    {
      Serial.println("Failed to connect: host not found...");
      return false;
    }
    Serial.println("putting mqtt host and port...");
    mqttClient.setServer(ip, port);
    Serial.println("connecting mqtt...");
    return mqttClient.connect("resource_id", user.c_str(), pass.c_str());
  }

  inline void disconnect() override
  {
    mqttClient.disconnect();
  }

  inline bool connected() override
  {
    return mqttClient.connected();
  }

  void loop() override
  {
    mqttClient.loop();
  }

  bool send() override
  {
    if (!this->connected())
    {
      Serial.println("not send...");
      return false;
    }
    String data = getDataPacketJson();
    mqttClient.publish(topicPublish.c_str(), data.c_str());
    Serial.println(data.c_str());
    return true;
  }
};

class More4iotHttp : public More4iot
{
private:
  HttpClient httpClient;
  const char *host;
  int port;
  String route = "/inputCommunicator";
  String contentType = "application/json; charset=utf-8";

public:
  inline More4iotHttp(Client &client,
                      const char *host, int port = 80)
      : httpClient(client, host, port), host(host), port(port) {}
  inline ~More4iotHttp() {}

  bool connect(const char *host, int port)
  {
    if (!httpClient.connect(host, port))
    {
      Serial.println("connect to server failed");
      return false;
    }
    return true;
  };

  inline void disconnect()
  {
    httpClient.stop();
  };

  bool connected()
  {
    return httpClient.connected();
  };

  void loop(){};

  bool send()
  {

    if (!this->connected())
    {
      if (!this->connect(host, port))
      {
        return false;
      }
    }
    String payload = getDataPacketJson();
    Serial.println(payload);
    httpClient.post(route, contentType, payload);
    if (httpClient.responseStatusCode() != HTTP_SUCCESS && httpClient.responseStatusCode() != HTTP_ERROR_INVALID_RESPONSE)
    {
      Serial.print("data not sent: ");
      Serial.println(httpClient.responseStatusCode());
      Serial.println(httpClient.responseBody());
      this->disconnect();
      return false;
    }

    this->disconnect();
    Serial.println("data sent");
    return true;
  }
};

class More4iotCoap : public More4iot
{
private:
  Coap coap;
  // endpoint for more4iot input communicator
  String endpointInput = "input";
  // more4iot connection
  IPAddress ip;
  int port;

public:
  inline More4iotCoap(UDP& udp, IPAddress& ip, int port = 5683)
      : coap(udp), ip(ip), port(port) {}
  inline ~More4iotCoap() {}

  bool connect();
  void loop() override;
  bool send() override;

  inline void disconnect() override
  {
    return;
  }

  inline bool connected() override
  {
    return true;
  }

  void response(CoapCallback c){
    coap.response(c);
  }

  void server(CoapCallback c, String url){
    coap.server(c, url);
  }

  void sendResponse(IPAddress ip, int port, uint16_t messageid, const char *payload){
    coap.sendResponse(ip,port,messageid,payload);
  }
};

#endif