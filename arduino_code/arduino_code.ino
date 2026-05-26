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
WiFiClient client;

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

const int PINOS_SENSORES[4] = {2, 3, 4, 5};       // Sensores Indutivos (Nichos 1 a 4)
const int PINOS_SERVOS[4]   = {6, 7, 8, A0};      // Servos Motores (Nichos 1 a 4)

Servo servos[4];
bool estadosAnteriores[4] = {true, true, true, true}; // Armazena o estado antigo de cada sensor para evitar envios duplicados

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
}

void loop() {
  for (int i = 0; i < 4; i++) {
    bool kitNaVaga = (digitalRead(PINOS_SENSORES[i]) == LOW);

    if (kitNaVaga != estadosAnteriores[i]) {
      lcd.clear();
      int numeroNicho = i + 1;

      if (kitNaVaga) {
        lcd.setCursor(0, 0);
        lcd.print("Nicho " + String(numeroNicho) + ": DISPON.");
        lcd.setCursor(0, 1);
        lcd.print("Aproxime o Card ");
        enviarStatusBackend(numeroNicho, "disponivel", "SISTEMA");
      } else {
        lcd.setCursor(0, 0);
        lcd.print("Nicho " + String(numeroNicho) + ": EM USO ");
        lcd.setCursor(0, 1);
        lcd.print("Aproxime o Card ");
        enviarStatusBackend(numeroNicho, "ocupado", "RA_USER"); 
      }
      estadosAnteriores[i] = kitNaVaga;
      delay(1000); 
    }
  }

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Card Detectado!");
  lcd.setCursor(0, 1);
  lcd.print("Buscando Vaga...");
  delay(1500);

  int nichoParaAbrir = -1;
  for (int i = 0; i < 4; i++) {
    bool kitNaVaga = (digitalRead(PINOS_SENSORES[i]) == LOW);
    
    if (kitNaVaga) {
      nichoParaAbrir = i; 
      break; 
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Nicho " + String(i + 1) + " ocupado");
      lcd.setCursor(0, 1);
      lcd.print("Checando proximo");
      delay(1200);
    }
  }

  
  lcd.clear();
  if (nichoParaAbrir != -1) {
    int numeroFinal = nichoParaAbrir + 1;
    
    lcd.setCursor(0, 0);
    lcd.print("Liberando Vaga");
    lcd.setCursor(0, 1);
    lcd.print("Nicho " + String(numeroFinal) + " !!!      ");
    
    servos[nichoParaAbrir].write(90); 
    delay(4000); //tempo que a trava fica aberta 4s 

    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Trancando Vaga " + String(numeroFinal));
    servos[nichoParaAbrir].write(0);
    delay(1500);

  } else {
    lcd.setCursor(0, 0);
    lcd.print("Armario Cheio! ");
    lcd.setCursor(0, 1);
    lcd.print("Nenhum Kit Disp.");
    delay(3000);
  }

  for (int i = 0; i < 4; i++) {
    estadosAnteriores[i] = !estadosAnteriores[i];
  }

  rfid.PICC_HaltA();
}