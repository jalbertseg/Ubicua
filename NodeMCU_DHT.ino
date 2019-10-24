//REGION: Librerias

#include "DHT.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>

//REGION: Configuración wireless

const char* ssid="wifi_ssid";
const char* password = "pass";
const char* namehost="host_name";
const char* clientname = "name_client";

//REGION: MQTT Configuración de la conexión

const char* mqtt_server = "000.000.000.000"; 
const char* mqtt_user = "username"; 
const char* mqtt_password = "user_pass"; 

//REGION: Conectividad

WiFiClient espClient; 
PubSubClient client(espClient); 
MDNSResponder mdns;

//REGION: Tags de MQTT donde se publica la información

#define humidity_topic "sensor/humidity" 
#define temperature_topic "sensor/temperature" 

//REGION: Configuración sensor de temperatura/humedad

#define DHTPIN 2           // Pin al que se conecta el sensor
#define DHTTYPE DHT22      // Tipo de sensor que hemos conectado
DHT dht(DHTPIN, DHTTYPE);  // Creamos la variable DHT
int timeSinceLastRead = 0; // Variable que usamos para los intervalos de lectura del sensor

static char celsiusTemp[7];     // Variable para mostrar grados celsius
static char fahrenheitTemp[7];  // Variable para mostrar grados fahrenheit
static char humidityTemp[7];    // Variable para mostrar la humedad

static float celsiusT;     // Variable para mostrar grados celsius
static float fahrenheitT;  // Variable para mostrar grados fahrenheit
static float humidityT;    // Variable para mostrar la humedad


//REGION: Configuración del servidor web

ESP8266WebServer server(80);
String webPage = "";


void setup() 
{
  dht.begin();
  initSerial();
  initWifi();
  initWebServer();
  initMQTTServer();
}

void loop() 
{
    //Se pone a la escucha al servidor web
    server.handleClient();

    //Nos aseguramos de que conecte
    if (!client.connected()) {
      reconnect();
    }
  
    //MQTT comienza a funcionar
    client.loop();

    readData();
}

void initSerial()
{
  Serial.begin(115200);
  Serial.setTimeout(5000);

  // Inicialización del puerto serie
  while(!Serial) { }

  //Mensaje de bienvenida
  Serial.println("-------------------------------------");
  Serial.println("Dispositivo en marcha");
  Serial.println("-------------------------------------");
}

void initWifi()
{
  Serial.println();
  Serial.print("Wifi conectando a: ");
  Serial.println( ssid );

  WiFi.hostname(namehost);
  WiFi.begin(ssid,password);

  Serial.println();
  Serial.print("Conectando");

  while( WiFi.status() != WL_CONNECTED ){
      delay(500);
      Serial.print(".");        
  }
  Serial.println("Wifi conectada!");
  Serial.print("NodeMCU IP Address : ");
  Serial.println(WiFi.localIP() );

  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS arrancado");
  }
}

void initMQTTServer()
{
 client.setServer(mqtt_server, 1883); 
}

//Vuelve a conectar al servidor de MQTT si se desconecta
void reconnect() 
{
  while (!client.connected()) {
    Serial.print("Conectando al broker MQTT ...");

    
    if (client.connect(clientname, mqtt_user, mqtt_password)) {
      Serial.println("OK");
    } else {
      Serial.print("KO, error : ");
      Serial.print(client.state());
      Serial.println(" Esperando 5 segundos para reintentar");
      delay(5000);
    }
  }
}

void initWebServer()
{
    server.on("/", []() {
    webPage = "";
    webPage += "<!DOCTYPE HTML>";
    webPage += "<html>";
    webPage += "<head>Temperatura y humedad</head><body><h1>ESP8266 - Temperatura y humedad</h1><h3>Temperatura en Celsius: ";
    webPage += celsiusTemp;
    webPage += "*C</h3><h3>Temperatura en Fahrenheit: ";
    webPage += fahrenheitTemp;
    webPage += "*F</h3><h3>Humedad: ";
    webPage += humidityTemp;
    webPage += "%</h3><h3>";
    webPage += "</body></html>";
    server.send(200, "text/html", webPage);
  });

  server.begin();
  Serial.println("HTTP server arrancado");
}

void readData()
{
      if(timeSinceLastRead > 5000) {

    // El sensor tarda en tomar la lectura 250 milisegundos, por lo que tomamos un tiempo de 2 segundos para reportar las nuevas lecturas.
    // La temperatura se lee por defecto en grados celsius, si queremos mostrar farenheit necesitamos indicarlo con el parametro true.

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float f = dht.readTemperature(true);

    // Control de errores de lectura
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Error al leer del sensor DHT!");
      timeSinceLastRead = 0;
      return;
    }

    // Calculo del indice de calor, esto es la sensación de calor que se tiene realmente puesto que se tiene en cuenta temperatura y humedad
    // Calculo para grados farenheit
    float hif = dht.computeHeatIndex(f, h);
    fahrenheitT = hif;
    // Calculo para grados celsius
    float hic = dht.computeHeatIndex(t, h, false);
    celsiusT = hic;
    humidityT = h;

    //Aqui convertimos los float a cadena de caracteres
    dtostrf(hic, 6, 2, celsiusTemp);
    dtostrf(hif, 6, 2, fahrenheitTemp);
    dtostrf(h, 6, 2, humidityTemp);

    //Imprimimos por el puerto serie los datos
    Serial.print("Humedad: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperatura: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t");
    Serial.print("Sensación térmica: ");
    Serial.print(hic);
    Serial.print(" *C ");
    Serial.print(hif);
    Serial.println(" *F");

    client.publish(temperature_topic, String(celsiusT).c_str(), true);
    client.publish(humidity_topic, String(humidityT).c_str(), true);

    String result = "Temperatura: ";
    result = result + celsiusTemp;
    char charBuf[result.length() + 1];
    result.toCharArray(charBuf,result.length());

    //Reiniciamos el contador de tiempo
    timeSinceLastRead = 0;
  }
  //Volvemos a sumar tiempo al contador
  delay(100);
  timeSinceLastRead += 100;
}