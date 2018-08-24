#include "DHT22.h"
#include "YL100.h"
#include "MAX44009.h"
#include "K30.h"
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
#define FIELD_CO2 5

// PIN SETTINGS
#define PIN_SCL D1
#define PIN_SDA D2
#define PIN_DHT D3
#define PIN_YL100 A0
#define PIN_K30_RX D6
#define PIN_K30_TX D7

// PROGRAM SETTINGS
#define TIME_SLEEP 1000
#define CPT_PUBLISH 30

// VAR DECLARATION
HTTPClient http;

DHT dht(PIN_DHT, DHT22);
YL100 yl(PIN_YL100);
Max44009 max44(0x4A, PIN_SDA, PIN_SCL);
K30 k30(PIN_K30_RX, PIN_K30_TX);

uint16_t cptLoop = 0;

float temperature = 0;
float humidity = 0;
float moisture = 0;
float light = 0;
float co2 = 0;

uint16_t cptTemperature = 0;
uint16_t cptHumidity = 0;
uint16_t cptMoisture = 0;
uint16_t cptLight = 0;
uint16_t cptCo2 = 0;

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
  yl.begin();
  k30.begin();
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

  float _moisture = yl.readSoilMoisture();
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

  float _co2 = k30.getCO2();
  if (_co2 > 0 && _co2 <= 100000)
  {
    co2 += _co2;
    cptCo2++;
  }

  // Si le compteur de boucle arrive à CPT_PUBLISH
  if (cptLoop >= CPT_PUBLISH)
  {
    temperature = temperature / cptTemperature;
    humidity = humidity / cptHumidity;
    moisture = moisture / cptMoisture;
    light = light / cptLight;
    co2 = co2 / cptCo2;
    
    // Affichage à l'écran des valeurs lues
    Serial.println("======");
    Serial.printf("Temperature : %s C°\n", String(temperature).c_str());
    Serial.printf("Humidity : %s %%\n", String(humidity).c_str());
    Serial.printf("Moisture : %s %%\n", String(moisture).c_str());
    Serial.printf("Light : %s lux\n", String(light).c_str());
    Serial.printf("Co2 : %s ppm\n", String(co2).c_str());

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

    if (cptCo2 > 0)
      url += "&field"+ String(FIELD_CO2) +"="+ co2;
    else
      Serial.println("Unable to read co2");
        
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
    co2 = 0;

    cptTemperature = 0;
    cptHumidity = 0;
    cptMoisture = 0;
    cptLight = 0;
    cptCo2 = 0;
  }

  // Temps en millisecondes où le programme ne fait rien
  delay(TIME_SLEEP);
}
