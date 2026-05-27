#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <WiFiS3.h>

char ssid[] = "iPhone de Iago"; 
char pass[] = "audi1005!";
int status = WL_IDLE_STATUS;

char server[] = "52.15.237.15";
int port = 8080;
WiFiClient client;

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

const int PINOS_SENSORES[4] = {2, 3, 4, 5};
const int PINOS_SERVOS[4]   = {6, 7, 8, A0};  

Servo servos[4];
bool estadosAnteriores[4] = {true, true, true, true};

void enviarStatusBackend(int nichoId, String statusKit, String ra) {
  if (client.connect(server, port)) {
    String jsonBody = "{\"nicho_id\": " + String(nichoId) + ", \"status\": \"" + statusKit + "\", \"user_ra\": \"" + ra + "\"}";
    
    client.println("POST /kit/status HTTP/1.1");
    client.print("Host: "); client.println(server);
    client.println("Content-Type: application/json");
    client.print("Content-Length: "); client.println(jsonBody.length());
    client.println("Connection: close");
    client.println();
    client.println(jsonBody);
    
    client.stop();
  }
}

void setup() {
  lcd.init();
  lcd.backlight();
  
  SPI.begin();
  rfid.PCD_Init();

  for (int i = 0; i < 4; i++) {
    pinMode(PINOS_SENSORES[i], INPUT_PULLUP);
    servos[i].attach(PINOS_SERVOS[i]);
    servos[i].write(0);
  }

  lcd.setCursor(0, 0);
  lcd.print("Conectando Wi-Fi");
  lcd.setCursor(0, 1);
  lcd.print("Aguarde...      ");

  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(2000); 
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SISTEMA ONLINE  ");
  lcd.setCursor(0, 1);
  lcd.print("AWS CONECTADA!  ");
  delay(2000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aproxime o card ");
  lcd.setCursor(0, 1);
  lcd.print("p/ liberar nicho");
}

void loop() {
  for (int i = 0; i < 4; i++) {
    bool temMetal = (digitalRead(PINOS_SENSORES[i]) == LOW);

    if (temMetal != estadosAnteriores[i]) {
      int numeroNicho = i + 1;

      if (temMetal) {
        enviarStatusBackend(numeroNicho, "disponivel", "SISTEMA");
      } else {
        enviarStatusBackend(numeroNicho, "ocupado", "RA_USER"); 
      }
      estadosAnteriores[i] = temMetal;
    }
  }

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Detectando se tem");
  lcd.setCursor(0, 1);
  lcd.print("nicho disponivel");
  delay(2000);

  int nichoParaAbrir = -1;

  for (int i = 0; i < 4; i++) {
    bool temMetal = (digitalRead(PINOS_SENSORES[i]) == LOW);
    
    if (temMetal) {
      nichoParaAbrir = i; 
      break;
    }
  }

  lcd.clear();
  if (nichoParaAbrir != -1) {
    int numeroFinal = nichoParaAbrir + 1;
    
    lcd.setCursor(0, 0);
    lcd.print("Nicho " + String(numeroFinal) + " disponiv.");
    lcd.setCursor(0, 1);
    lcd.print("Retire o kit... ");
    
    servos[nichoParaAbrir].write(90); 
    delay(4000);                      

    servos[nichoParaAbrir].write(0);

  } else {
    lcd.setCursor(0, 0);
    lcd.print("Nichos ocupados ");
    lcd.setCursor(0, 1);
    lcd.print("Nenhum disponiv.");
    delay(3000);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aproxime o card ");
  lcd.setCursor(0, 1);
  lcd.print("p/ liberar nicho");

  rfid.PICC_HaltA();
}