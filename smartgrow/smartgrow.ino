#include "DHT22.h"
#include "SEN0193.h"
#include "MAX44009.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// WIFI SETTINGS
#define WIFI_SSID "mon-reseau-wifi"
#define WIFI_KEY "ma-clef-wifi"

// THINGSPEAK SETTINGS
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_API_KEY "ABCDEFGH12345678"

#define FIELD_TEMPERATURE 1
#define FIELD_HUMIDITY 2
#define FIELD_MOISTURE 3
#define FIELD_LIGHT 4

// PIN SETTINGS
#define PIN_SCL D1
#define PIN_SDA D2
#define PIN_DHT D3
#define PIN_SEN0193 A0

// PROGRAM SETTINGS
#define TIME_SLEEP 1000
#define CPT_PUBLISH 30

// VAR DECLARATION
HTTPClient http;

DHT dht(PIN_DHT, DHT22);
SEN0193 sen(PIN_SEN0193);
Max44009 max44(0x4A, PIN_SDA, PIN_SCL);

uint16_t cptLoop = 0;

float temperature = 0;
float humidity = 0;
float moisture = 0;
float light = 0;

uint16_t cptTemperature = 0;
uint16_t cptHumidity = 0;
uint16_t cptMoisture = 0;
uint16_t cptLight = 0;

// Fonction exécutée au démarrage du système
void setup()
{
  // Initialisation de la sortie Serial pour nous permettre de monitorer notre système
  Serial.begin(9600);
  Serial.println("\nSetup");
  delay(50);

  // Initialisation du WIFI
  WiFi.begin(WIFI_SSID, WIFI_KEY);

  Serial.printf("Connecting to %s with key %s\n", WIFI_SSID, WIFI_KEY);
  
  uint32_t timer = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);

    // Si il n'arrive pas à se connecter au bout de 20 secondes, le système redémarre (au cas ou un bug serait survenu)
    if (millis() - timer >= 20000)
    {
      Serial.println("Unable to connect, restarting...");
      ESP.restart();
    }
  }
 
  Serial.println(" WiFi connected");

  // Initialisation pour le DHT22 et le YL100
  dht.begin();
  sen.begin();
}

// Fonction exécutéee en boucle une fois la fonction setup() terminée
void loop()
{
  // Incrémentation de 1 du compteur de boucle
  cptLoop++;
  
  // Lecture des capteurs
  float _temperature = dht.readTemperature();
  if (_temperature >= -40 && _temperature <= 80)
  {
    temperature += _temperature;
    cptTemperature++;
  }

  float _humidity = dht.readHumidity();
  if (_humidity >= 0 && _humidity <= 200)
  {
    humidity += _humidity;
    cptHumidity++;
  }

  float _moisture = sen.readSoilMoisture();
  if (_moisture >= 0 && _moisture <= 200)
  {
    moisture += _moisture;
    cptMoisture++;
  }

  float _light = max44.getLux();
  if (_light >= 0 && _light <= 1000000)
  {
    light += _light;
    cptLight++;
  }

  // Si le compteur de boucle arrive à CPT_PUBLISH
  if (cptLoop >= CPT_PUBLISH)
  {
    temperature = temperature / cptTemperature;
    humidity = humidity / cptHumidity;
    moisture = moisture / cptMoisture;
    light = light / cptLight;
    
    // Affichage à l'écran des valeurs lues
    Serial.println("======");
    Serial.printf("Temperature : %s C°\n", String(temperature).c_str());
    Serial.printf("Humidity : %s %%\n", String(humidity).c_str());
    Serial.printf("Moisture : %s %%\n", String(moisture).c_str());
    Serial.printf("Light : %s lux\n", String(light).c_str());

    // Si nous ne sommes plus connectés au WiFi
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Connection to the SSID lost, trying to reconnect...");
      
      uint32_t timer = millis();
      while (WiFi.status() != WL_CONNECTED)
      {
        Serial.print(".");
        delay(200);
    
        if (millis() - timer >= 20000)
        {
          Serial.println(" Unable to connect, restarting...");
          ESP.restart();
        }
      }
    }

    // Création de l'URL
    String url;
    url.reserve(256);
    url = "http://"+ String(THINGSPEAK_HOST) +"/update?api_key="+ String(THINGSPEAK_API_KEY);

    // Ici on intègre les champs et leurs valeurs.
    if (cptTemperature > 0)
      url += "&field"+ String(FIELD_TEMPERATURE) +"="+ temperature;
    else
      Serial.println("Unable to read temperature");

    if (cptHumidity > 0)
      url += "&field"+ String(FIELD_HUMIDITY) +"="+ humidity;
    else
      Serial.println("Unable to read humidity");

    if (cptMoisture > 0)
      url += "&field"+ String(FIELD_MOISTURE) +"="+ moisture;
    else
      Serial.println("Unable to read soil moisture");

    if (cptLight > 0)
      url += "&field"+ String(FIELD_LIGHT) +"="+ light;
    else
      Serial.println("Unable to read lux");
        
    Serial.printf("Fetching %s\n", url.c_str());      

    // Etablissement de la communication HTTP
    http.begin(url);
    int httpCode = http.GET();

    // Si nous n'avons pas réussi à émettre les données, nous redémarrons le système (au cas ou un bug serait survenu)
    if(httpCode != HTTP_CODE_OK)
    {
      Serial.printf("[HTTP] GET... failed, code: %d, error: %s\n", httpCode, http.errorToString(httpCode).c_str());
      ESP.restart();
    }
  
    Serial.println("Data sent !");
  
    http.end();

    // Remise à zéro
    cptLoop = 0;
    
    temperature = 0;
    humidity = 0;
    moisture = 0;
    light = 0;

    cptTemperature = 0;
    cptHumidity = 0;
    cptMoisture = 0;
    cptLight = 0;
  }

  // Temps en millisecondes où le programme ne fait rien
  delay(TIME_SLEEP);
}