#include "DHT22.h"
#include "YL100.h"
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
#define PIN_YL100 A0

// PROGRAM SETTINGS
#define TIME_SLEEP 1000
#define CPT_PUBLISH 30

// VAR DECLARATION
HTTPClient http;

DHT dht(PIN_DHT, DHT22);
YL100 yl(PIN_YL100);
Max44009 max44(0x4A, PIN_SDA, PIN_SCL);

uint16_t cpt = 0;

float temperature = 0;
float humidity = 0;
float moisture = 0;
float light = 0;

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
}

// Fonction exécutéee en boucle une fois la fonction setup() terminée
void loop()
{
  // Incrémentation de 1 du compteur de boucle
  cpt++;

  // Lecture des capteurs
  temperature += dht.readTemperature();
  humidity += dht.readHumidity();
  moisture += yl.readSoilMoisture();
  light += max44.getLux();

  // Si le compteur de boucle arrive à CPT_PUBLISH
  if (cpt >= CPT_PUBLISH)
  {
    char strTemp[16];
    char strHum[16];
    char strMoisture[16];
    char strLight[16];

    // Conversion des float en char[]
    dtostrf(temperature/cpt, -1, 2, strTemp);
    dtostrf(humidity/cpt, -1, 2, strHum);
    dtostrf(moisture/cpt, -1, 2, strMoisture);
    dtostrf(light/cpt, -1, 2, strLight);

    // Affichage à l'écran des valeurs lues
    Serial.println("======");
    Serial.printf("Temperature : %s C°\n", strTemp);
    Serial.printf("Humidity : %s %%\n", strHum);
    Serial.printf("Moisture : %s %%\n", strMoisture);
    Serial.printf("Light : %s lux\n", strLight);

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
    char url[256];
    sprintf(url, "http://%s/update?api_key=%s&field%d=%s&field%d=%s&field%d=%s&field%d=%s", THINGSPEAK_HOST, THINGSPEAK_API_KEY, FIELD_TEMPERATURE, strTemp, FIELD_HUMIDITY, strHum, FIELD_MOISTURE, strMoisture, FIELD_LIGHT, strLight);
    
    Serial.printf("Fetching %s\n", url);      

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
    cpt = 0;
    
    light = 0;
    temperature = 0;
    humidity = 0;
    moisture = 0;
  }

  // Temps en millisecondes où le programme ne fait rien
  delay(TIME_SLEEP);
}
