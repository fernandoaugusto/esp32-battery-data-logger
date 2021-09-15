#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "[INSERT_HERE_SSID]";
const char* password = "[INSERT_HERE_PASSWORD]";

#define VOLT_PIN A0  //check analog pin
#define LED_BLUE_PIN 2
#define NUM_SAMPLES 30
#define TIME_PER_SAMPLE 1000 //ms

int volt[NUM_SAMPLES] = {};
int volt_ready[NUM_SAMPLES] = {};
int i_count = 0;

unsigned long start_millis;
unsigned long current_millis;
unsigned long diff;

int res = 0;
boolean ready_to_send = false;

String json = "";
 
void setup(void) {
  Serial.begin(115200);
  
  pinMode(VOLT_PIN, INPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
 
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BLUE_PIN, HIGH);
    delay(200);
    digitalWrite(LED_BLUE_PIN, LOW);
    delay(300);
    Serial.print(".");
  }
  
  Serial.print("WIFI conectou com IP: ");
  Serial.println(WiFi.localIP());

  start_millis = millis();

  xTaskCreatePinnedToCore(core0, "core0", 10000, NULL, 1, NULL, 0);
  delay(500);

}
 
void loop() {

    current_millis = millis();
  
    if (current_millis - start_millis >= TIME_PER_SAMPLE) {
      Serial.println("Voltage " + String(i_count+1) + ": " + String(volt[i_count]) + " mV");
      start_millis = current_millis;
      volt[i_count] = voltage();
      i_count++;
    }
  
    if (i_count == 30) {
      memcpy(volt_ready, volt, sizeof(volt));
      ready_to_send = true;
      i_count = 0;
    }
    
}

void core0(void *pvParametros) {

  while (true) {
    if (ready_to_send) {
      Serial.println("Preparando para enviar os dados...");
      ready_to_send = false;
      sendData();
    }
    delay(10);
  }

}

void sendData() {

  DynamicJsonDocument doc(2048);
  JsonArray voltage_json = doc.createNestedArray("voltage_array");

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BLUE_PIN, HIGH);
    Serial.println("Conectado");
    
    HTTPClient http;
    Serial.println("Enviando...");
    
    doc["batch_datetime"] = String(String(__DATE__) + " " + String(__TIME__));
    
    for (int ii = 0; ii < NUM_SAMPLES; ii++) {
      voltage_json.add(volt_ready[ii]);
    }
    
    String url = "[INSERT_HERE_API_URL]";
    
    http.begin(url);
    http.setTimeout(4000);
    http.addHeader("Content-Type", "application/json");
    
    json = "";
    
    serializeJson(doc, json);
    
    Serial.println(json);
    
    res = http.POST(json);
    Serial.println("Codigo: " + String(res));
    if (res == 200) {
      Serial.println("Informações enviadas... ");
    } else if (res >= 400) {
      Serial.println("Falha ao enviar informações... ");
    }
    http.end();
    digitalWrite(LED_BLUE_PIN, LOW);
  
  } else {
    digitalWrite(LED_BLUE_PIN, LOW);
    Serial.println("Não conectado");
  }

}

int voltage() {

  float avg_read = 0.0;
  float sum_read = 0.0;
  int count_read = 0;

  while (count_read < 100) {
    sum_read += analogRead(VOLT_PIN);
    count_read++;
    delay(10);
  }

  avg_read = sum_read / count_read;

  int resp = 0;
  resp = round(1.511 * avg_read  + 231.362);

  return avg_read;
}
