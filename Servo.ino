
#include <Wire.h>
#include <Servo.h>
#include "VL53L0X.h"
#include "SimpleOLED.h"
#include "DHT.h"

// ---------------- OLED ----------------
SimpleOLED display(0x3C, 128, 64);

// ---------------- SENSOR LASER ----------------
VL53L0X sensorLaser;
bool laserDisponivel = false;

// ---------------- DHT22 ----------------
#define DHTPIN A3
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

float temperatura = NAN;
float umidade = NAN;

unsigned long ultimaLeituraDHT = 0;
const unsigned long intervaloDHT = 2200;

// ---------------- SERVOS ----------------
Servo servox;  // PAN
Servo servoy;  // TILT

int posPan = 90;
int posTilt = 90;

// ---------------- JOYSTICK ----------------
const int potpinx = A1;
const int potpiny = A0;

int valx, valy;

const int centroX = 512;
const int centroY = 512;
const int zonaMorta = 50;
const int passo = 2;

// ---------------- LASER ----------------
uint16_t distancia = 0;
bool erroLaser = false;

unsigned long ultimaLeituraLaser = 0;
const unsigned long intervaloLaser = 100;

// ---------------- OLED ATUALIZACAO ----------------
unsigned long ultimaAtualizacaoOLED = 0;
const unsigned long intervaloOLED = 500;

// ---------------- LEITURA DHT22 ----------------
void lerDHT() {
  // Uma leitura de cada grandeza por ciclo
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("ERRO DHT22: leitura invalida!");
    Serial.println("Verifique DATA, VCC e GND.");
    return;
  }

  umidade = h;
  temperatura = t;

  Serial.print("Temperatura: ");
  Serial.print(temperatura, 1);
  Serial.print(" C | Umidade: ");
  Serial.print(umidade, 1);
  Serial.println(" %");
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // OLED
  display.begin();
  display.clear();
  display.setCursor(0, 10);
  display.print("Iniciando sistema");
  display.display();

  // Servos
  servox.attach(2);
  servoy.attach(3);

  servox.write(posPan);
  servoy.write(posTilt);

  // DHT22
  dht.begin();
  Serial.println("Inicializando DHT22...");

  // Aguarda estabilizacao e primeira leitura
  delay(2000);
  lerDHT();
  ultimaLeituraDHT = millis();

  // Sensor laser: nao bloqueia o restante se falhar
  sensorLaser.setTimeout(200);

  laserDisponivel = sensorLaser.init();

  if (laserDisponivel) {
    sensorLaser.startContinuous(intervaloLaser);
    Serial.println("VL53L0X OK");
  } else {
    Serial.println("VL53L0X indisponivel");
  }

  display.clear();
  display.setCursor(0, 10);
  display.print("Sistema iniciado");
  display.display();

  delay(500);
}

void loop() {
  unsigned long agora = millis();

  // ---------------- JOYSTICK ----------------
  valx = analogRead(potpinx);
  valy = analogRead(potpiny);

  // PAN: direita diminui; esquerda aumenta
  if (valx > centroX + zonaMorta) {
    posPan -= passo;
  } else if (valx < centroX - zonaMorta) {
    posPan += passo;
  }

  // TILT: frente sobe; tras desce
  if (valy < centroY - zonaMorta) {
    posTilt += passo;
  } else if (valy > centroY + zonaMorta) {
    posTilt -= passo;
  }

  posPan = constrain(posPan, 0, 180);
  posTilt = constrain(posTilt, 0, 180);

  servox.write(posPan);
  servoy.write(posTilt);

  // ---------------- DHT22 ----------------
  if (agora - ultimaLeituraDHT >= intervaloDHT) {
    ultimaLeituraDHT = agora;
    lerDHT();
  }

  // ---------------- SENSOR LASER ----------------
  if (laserDisponivel &&
      agora - ultimaLeituraLaser >= intervaloLaser) {

    ultimaLeituraLaser = agora;

    distancia = sensorLaser.readRangeContinuousMillimeters();
    erroLaser = sensorLaser.timeoutOccurred();
  }

  // ---------------- OLED ----------------
  if (agora - ultimaAtualizacaoOLED >= intervaloOLED) {
    ultimaAtualizacaoOLED = agora;

    display.clear();

    // Linha 1: Pan e Tilt
    display.setCursor(0, 0);
    display.print("P:");
    display.print(posPan);
    display.print(" T:");
    display.print(posTilt);

    // Linha 2: distancia
    display.setCursor(0, 16);
    display.print("Dist:");

    if (!laserDisponivel) {
      display.print(" sem sensor");
    } else if (erroLaser) {
      display.print(" TIMEOUT");
    } else {
      display.print((int)distancia);
      display.print(" mm");
    }

    // Linha 3: temperatura
    display.setCursor(0, 32);
    display.print("Temp:");

    if (isnan(temperatura)) {
      display.print(" Erro DHT");
    } else {
      display.print(temperatura, 1);
      display.print(" C");
    }

    // Linha 4: umidade
    display.setCursor(0, 48);
    display.print("Umid:");

    if (isnan(umidade)) {
      display.print(" Erro DHT");
    } else {
      display.print(umidade, 1);
      display.print(" %");
    }

    display.display();
  }

  delay(10);
}
