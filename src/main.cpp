#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 12

#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

float temp = 0.0;
float umidityAir = 0.0;

unsigned long previousMillis = 0;
const long interval = 10000;


void setup() {
  // put your setup code here, to run once:
  pinMode(A0, INPUT);
  Serial.begin(9600);
  dht.begin();  
}

void loop() {
  // put your main code here, to run repeatedly:
  int moisture = analogRead(A0);

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT))
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
      temp = newT;
      Serial.print("Temperatura: ");
      Serial.println(temp);
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value
    if (isnan(newH))
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
      umidityAir = newH;
      Serial.print("Umidade do Ar: ");
      Serial.println(umidityAir);
    }

    if(moisture <= 340){
      Serial.println("Solo com bastante umidade");
      //send moisture%
    } else if (moisture > 340 && moisture<=800){
      Serial.println("Solo umido");
      //send moisture%
    } else if (moisture >800){
      Serial.println("Baixa umidade do solo");
      //send moisture%
    }
    
  }
}