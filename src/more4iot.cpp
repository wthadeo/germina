#include "more4iot.h"

bool DataAttribute::serializeDataAt(JsonObject &jsonObj) const
{
  if (dataName)
  {
    switch (dataType)
    {
    case TYPE_BOOL:
      jsonObj[dataName] = dataValue.boolean;
      break;
    case TYPE_INT:
      jsonObj[dataName] = dataValue.integer;
      break;
    case TYPE_REAL:
      jsonObj[dataName] = dataValue.real;
      break;
    case TYPE_STR:
      jsonObj[dataName] = dataValue.str;
      break;
    }
  }
  return true;
}

void More4iotDefaultLogger::log(const char *msg)
{
  Serial.print(F("[MORE4IoT] "));
  Serial.println(msg);
}

bool More4iotCoap::connect()
{
  coap.start();
}

void More4iotCoap::loop()
{
  coap.loop();
}

bool More4iotCoap::send()
{
  String data = getDataPacketJson();
  coap.put(ip, port, endpointInput.c_str(), data.c_str());
  Serial.println(data.c_str());
  return true;
}