/* ESTE CÓDIGO NÃO FOI REVISADO, OU SEJA, PROVAVÉL QUE TENHA REDUNDÂNCIA. 
 *  
 ********************************* HORTA AUTOMATIZADA ********************************* 
 *  
 *  created by: Douglas Maciel & Renan Almeida, in 2018
*/


#include <WiFi.h>
#include <WiFiMulti.h>
#include "DHT.h"

#define LDR_sensor 36
#define valvula 16
#define solo_1 34
//#define solo_2 2
//#define solo_3 5
#define sensor_chuva 23
#define DHTPIN 33
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiMulti WiFiMulti;

const char* ssid     = "teste"; // Nome do wifi
const char* password = "1234567899"; 

const char* host = "api.thingspeak.com";
String api_key = "MJF4VPAC9DMGD0WY"; // Your API Key provied by thingspeak

float valor_temp;
float valor_umd;

// Declarando as variaveis inteiras
int umid_1;
int umid_2;
int umid_3;
int LDR_enviar;
int estado_sensores;
int estado_irrigacao;
int estado_sensor_chuva = digitalRead(sensor_chuva); //sensor de chuva recebe 0 ou seja falso


void setup() {
  Serial.begin(115200); // ESP é 115200
  pinMode(valvula, OUTPUT);
  pinMode(sensor_chuva, INPUT); // sensor de chuva
  dht.begin();
  Connect_to_Wifi();
}

void loop() {  // Criar uma função antes para checar se os valores estao certos

  luminosidade();
  sensor_Temperatura_Umidade();
  mapar_valores();
  checar_sensores_para_irrigar();
  delay(100);
}

int luminosidade() { // Vamos usar um resistor de: (IMPORTANTE DEFINIR)

  int LDR = analogRead(LDR_sensor);
  Serial.println(analogRead(LDR_sensor));
  if (LDR <= 200) {
    Serial.println("Ensolarado");
    LDR_enviar = -1;                // Indica ao Virtuino que está ensolarado
  }

  if (LDR > 200 && LDR < 500) {
    Serial.println("Nublado");
    LDR_enviar = 0;                 // Indica ao Virtuino que está nublado
  }

  if (LDR >= 500) {
    digitalWrite(valvula, LOW);
    Serial.println("Escuro");
    LDR_enviar = 1;                 // Indica ao Virtuino que está escuro
    delay(1000);
  }
}

void mapar_valores() {
  umid_1 = map(analogRead(solo_1), 0, 4095, 100, 0);
  //umid_2 = map(analogRead(solo_2), 0, 4095, 100, 0);
  //umid_3 = map(analogRead(solo_3), 0, 4095, 100, 0);

}


void checar_sensores_para_irrigar() {
  if (isnan(valor_temp) || isnan(umid_1) || isnan(umid_2) || isnan(umid_3)) {
    estado_sensores = 1;
    Serial.println("Alguns dos sensores esta com problema");
    Checar_tempo_enviar();
  }
  else {
    estado_sensores = 0;
    irrigacao();
  }
}

void irrigacao() {
  int umid = umid_1; // + umid_2 + umid_3)/3;

  if (umid <= 33) {
    Serial.println(" Status: Seco");

    if ( valor_temp > 23 && estado_sensor_chuva == LOW && LDR_enviar != 1) {
      digitalWrite(valvula, HIGH);
      Serial.println("Irrigando a plantação");
      estado_irrigacao = 1;
      Checar_tempo_enviar();
      delay(60000);
      digitalWrite(valvula, LOW);
    }
    else {
      estado_irrigacao = 0;
      Checar_tempo_enviar();
    }
  }

  if (umid > 33 && umid < 66) {
    Serial.println(" Status: parcialmente umido");
    if ( valor_temp > 23 && estado_sensor_chuva == LOW && LDR_enviar != 1) {
      digitalWrite(valvula, HIGH);
      Serial.println("Irrigando a plantação");
      estado_irrigacao = 1;
      Checar_tempo_enviar();
      delay(30000);
      digitalWrite(valvula, LOW);
    }
    else {
      estado_irrigacao = 0;
      Checar_tempo_enviar();
    }
  }
  if (umid >= 66) {
    Serial.println(umid);
    Serial.println(" Status: Umido");
    delay(30000);
  }
  delay(1000);
}

void sensor_Temperatura_Umidade() {  // colocar os parametros

  float valor_temp_bruto = dht.readTemperature();             //Alocando valor vindo direto do sensor
  float valor_umd_bruto = dht.readHumidity();                 //Alocando valor vindo direto do sensor

  if (isnan(valor_temp_bruto) || isnan(valor_umd_bruto))   //Checando se esses valores alocados são realmente numeros
  {
    valor_temp = 0;
    valor_umd = 0;
    Serial.println("Sensor DHT11 desconectado ou queimado");
    delay(1500);
  }
  else {                                                    // Se forem, guardar elas na variável global
    valor_temp = dht.readTemperature();
    valor_umd = dht.readHumidity();
    Serial.print(valor_temp);
    Serial.println("°C");
    Serial.print(valor_umd);
    Serial.println("%");
    Serial.println();
    delay(1500);
  }
}

void Connect_to_Wifi()
{
  // We start by connecting to a WiFi network
  WiFiMulti.addAP(ssid, password);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void Checar_tempo_enviar()
{
  unsigned long tempototal = millis();
  if ( tempototal % 60000 < 5000 ) {
    Send_Data();
  }
  else {
    return;
  }

}
void Send_Data()
{

  Serial.println("Prepare to send data");

  // Use WiFiClient class to create TCP connections
  WiFiClient client;

  const int httpPort = 80;

  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  else
  {
    String data_to_send = api_key;
    data_to_send += "&field1=";
    data_to_send += String(umid_1);
    data_to_send += "&field2=";
    data_to_send += String(umid_2);  // Certo
    data_to_send += "&field3=";
    data_to_send += String(umid_3);  // Certo
    data_to_send += "&field4=";
    data_to_send += String(valor_temp);   // Certo
    data_to_send += "&field5=";
    data_to_send += String(estado_sensor_chuva);   // Mudar a condição de acender as luzes no app, botar maior ou igual lá e tals
    data_to_send += "&field6=";
    data_to_send += String(LDR_enviar); // Certo
    data_to_send += "&field7=";
    data_to_send += String(estado_irrigacao);  //Situação Irrigação
    data_to_send += "&field8=";
    data_to_send += String(estado_sensores);  //Status Sensores
    data_to_send += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + api_key + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(data_to_send.length());
    client.print("\n\n");
    client.print(data_to_send);

    delay(1000);
  }
  client.stop();
}
