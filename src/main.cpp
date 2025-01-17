#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Arduino.h>
#include "apwifieeprommode.h"
#include <EEPROM.h>
#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Matriz Led 64x32 P4
#define PANEL_RES_X 64 // Number of pixels wide of each INDIVIDUAL panel module.
#define PANEL_RES_Y 32 // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1 // Total number of panels chained one to another

#define R1 25
#define G1 26
#define BL1 27
#define R2 14
#define G2 12
#define BL2 2
#define CH_A 23
#define CH_B 19
#define CH_C 5
#define CH_D 17
#define CH_E -1 // assign to any available pin if using two panels or 64x64 panels with 1/32 scan
#define CLK 16
#define LAT 4
#define OE 15

MatrixPanel_I2S_DMA *dma_display = nullptr;

//DFPlayer Mini MP3
HardwareSerial mySerial(1); // Usar UART1 de la ESP32
DFRobotDFPlayerMini myDFPlayer;

// Pines de joystick
const int X1_pin = 34;
const int Y1_pin = 35;
const int X2_pin = 32;
const int Y2_pin = 33;
const int SW_pin = 22;

// Variables de juego
int foodX, foodY;
int snakeX = PANEL_RES_X / 2, snakeY = PANEL_RES_Y / 2;
int snakeSize = 1;
bool isGameOver = false;
char direction = ' ';
int tailX[100], tailY[100]; // Colas del snake
int velocidad = 250;

//Variables del jugador
String username = "";
int score = 0;
unsigned long startTime = 0; // Variable para almacenar el tiempo inicial
unsigned long gameTime = 0; // Variable para almacenar el tiempo transcurrido

//Variables para 2 jugadores
int snakeX1 = 15, snakeY1 = 10, snakeSize1 = 1;
int snakeX2 = 50, snakeY2 = 10, snakeSize2 = 1;
int tailX1[100], tailY1[100];
int tailX2[100], tailY2[100];
int foodX1, foodY1;
int foodX2, foodY2;
char direction1 = 'r', direction2 = 'r';
int score1 = 0, score2 = 0;
bool isGameOver1 = false, isGameOver2 = false;
int velocidad_2P = 100;

// Variables para el menú
int gameOption = 0; // 0 = Menu principal, 1 = Game Over
int menuOption = 0; // 0 = Go, 1 = Set, 2 = Info (Se instancia con un numero fuera del rango para evitar ingresar a la primera opcion sin seleccionarla)
int configOption = 0; // 0 = Brillo, 1 = Volumen
int playerOption = 0; // 0 = 1P, 1 = 2P

bool inMenu = true;
bool isInConfigMenu = false; // Indicador de si estamos en el menú de configuración
bool isInInfo = false;
bool isInConfigName = true;
bool onePlayer = false;
bool twoPlayers = false;
bool isInChoosePlayer = false;
bool inGame = false;

int brightness = 80; // Nivel de brillo inicial (0-255)
int volume = 15; // Nivel de volumen inicial (0-30)

// URL de la API REST
const char* serverUrl = "https://eduisant.pythonanywhere.com/snake/jugadores/"; // Cambia por tu endpoint
const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n"
"MIIEVzCCAj+gAwIBAgIRAIOPbGPOsTmMYgZigxXJ/d4wDQYJKoZIhvcNAQELBQAwTzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2VhcmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAwWhcNMjcwMzEyMjM1OTU5WjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3MgRW5jcnlwdDELMAkGA1UEAxMCRTUwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAAQNCzqKa2GOtu/cX1jnxkJFVKtj9mZhSAouWXW0gQI3ULc/FnncmOyhKJdyIBwsz9V8UiBOVHhbhBRrwJCuhezAUUE8Wod/Bk3U/mDR+mwt4X2VEIiiCFQPmRpM5uoKrNijgfgwgfUwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQGCCsGAQUFBwMCBggrBgEFBQcDATASBgNVHRMBAf8ECDAGAQH/AgEAMB0GA1UdDgQWBBSfK1/PPCFPnQS37SssxMZwi9LXDTAfBgNVHSMEGDAWgBR5tFnme7bl5AFzgAiIyBpY9umbbjAyBggrBgEFBQcBAQQmMCQwIgYIKwYBBQUHMAKGFmh0dHA6Ly94MS5pLmxlbmNyLm9yZy8wEwYDVR0gBAwwCjAIBgZngQwBAgEwJwYDVR0fBCAwHjAcoBqgGIYWaHR0cDovL3gxLmMubGVuY3Iub3JnLzANBgkqhkiG9w0BAQsFAAOCAgEAH3KdNEVCQdqk0LKyuNImTKdRJY1C2uw2SJajuhqkyGPY8C+zzsufZ+mgnhnq1A2KVQOSykOEnUbx1cy637rBAihx97r+bcwbZM6sTDIaEriR/PLk6LKs9Be0uoVxgOKDcpG9svD33J+G9Lcfv1K9luDmSTgG6XNFIN5vfI5gs/lMPyojEMdIzK9blcl2/1vKxO8WGCcjvsQ1nJ/Pwt8LQZBfOFyVXP8ubAp/au3dc4EKWG9MO5zcx1qT9+NXRGdVWxGvmBFRAajciMfXME1ZuGmk3/GOkoAM7ZkjZmleyokP1LGzmfJcUd9s7eeu1/9/eg5XlXd/55GtYjAM+C4DG5i7eaNqcm2F+yxYIPt6cbbtYVNJCGfHWqHEQ4FYStUyFnv8sjyqU8ypgZaNJ9aVcWSICLOIE1/Qv/7oKsnZCWJ926wU6RqG1OYPGOi1zuABhLw61cuPVDT28nQS/e6z95cJXq0eK1BcaJ6fJZsmbjRgD5p3mvEf5vdQM7MCEvU0tHbsx2I5mHHJoABHb8KVBgWp/lcXGWiWaeOyB7RP+OfDtvi2OsapxXiV7vNVs7fMlrRjY1joKaqmmycnBvAq14AEbtyLsVfOS66B8apkeFX2NY4XPEYV4ZSCe8VHPrdrERk2wILG3T/EGmSIkCYVUMSnjmJdVQD9F6Na/+zmXCc=\n"
"-----END CERTIFICATE-----\n";

WiFiClientSecure client;
// Prototipo de la función
void sendDataToAPI(String username, int score, unsigned long gameTime);

bool isButtonPressed() {
    static unsigned long lastPressTime = 0;
    unsigned long currentTime = millis();

    if (digitalRead(SW_pin) == LOW && (currentTime - lastPressTime > 200)) {
        lastPressTime = currentTime;
        return true;
    }
    return false;
}

//----------------------------------------------------- **CONTROL DE TIEMPO** ---------------------------------------------------------//
// Función para inicializar el temporizador al inicio del juego
void startGameTimer() {
  startTime = millis(); // Guardar el tiempo inicial
}
// Función para verificar y guardar el tiempo transcurrido al perder
void checkTimeGameOver() {
  if (isGameOver) {
    // Calcular el tiempo transcurrido
    gameTime = (millis() - startTime) / 1000; // Tiempo en segundos
  }
}

// Función para comparar puntajes
void checkScore() {
  if ((millis() - startTime) > 60000) {
    if(score1>score2) {
      isGameOver2 = true;
    } 
    else if (score1<score2) {
      isGameOver1 = true;
    }
  }
}

//----------------------------------------------------- **CONTROL DFPLAYER MINI MP3** -----------------------------------------------------------//

// Función para iniciar el DFPlayer y reproducir música
void startGameMusic(int numCancion) {
  Serial.println("Reproduciendo música...");
  myDFPlayer.play(numCancion); // Reproducir la pista indicada por la variable numCancion
}

// Función para detener la música
void stopGameMusic() {
  Serial.println("Deteniendo música...");
  myDFPlayer.stop();
}

//----------------------------------------------------- **CONTROL PARA JUEGO** -----------------------------------------------------------//

// Coordinación de la cola del snake
void manageSnakeTailCoordinates() {
  int previousX = tailX[0], previousY = tailY[0];
  tailX[0] = snakeX;
  tailY[0] = snakeY;

  for (int i = 1; i < snakeSize; i++) {
    int tempX = tailX[i], tempY = tailY[i];
    tailX[i] = previousX;
    tailY[i] = previousY;
    previousX = tempX;
    previousY = tempY;
  }
}

// Posición aleatoria de la comida
void setupFoodPosition() {
  bool isValidPosition;
  do {
    // Generar una posición aleatoria dentro de los límites (evitando paredes)
    foodX = random(1, PANEL_RES_X - 1);
    foodY = random(1, PANEL_RES_Y - 1);

    // Verificar si la posición es válida
    isValidPosition = true;

    // Comprobar que la comida no esté sobre la serpiente
    for (int i = 0; i < snakeSize; i++) {
      if (foodX == tailX[i] && foodY == tailY[i]) {
        isValidPosition = false;
        break;
      }
    }
  } while (!isValidPosition); // Repetir mientras la posición no sea válida
}

// Verificar si el snake ha comido la comida
void manageEatenFood() {
  if (snakeX == foodX && snakeY == foodY) {
    score++;
    snakeSize++;
    setupFoodPosition();
    if(velocidad > 50){
      velocidad = velocidad - 10;
    }
  }
}

// Dibujar el snake, la comida y las paredes
void drawGame() {
  dma_display->clearScreen();

  // Dibujar las paredes
  for (int x = 0; x < PANEL_RES_X; x++) {
    dma_display->drawPixel(x, 0, dma_display->color565(255, 255, 0)); // Pared superior
    dma_display->drawPixel(x, PANEL_RES_Y - 1, dma_display->color565(255, 255, 0)); // Pared inferior
  }
  for (int y = 0; y < PANEL_RES_Y; y++) {
    dma_display->drawPixel(0, y, dma_display->color565(255, 255, 0)); // Pared izquierda
    dma_display->drawPixel(PANEL_RES_X - 1, y, dma_display->color565(255, 255, 0)); // Pared derecha
  }

  // Dibujar la comida
  dma_display->drawPixel(foodX,foodY, dma_display->color565(255, 0, 0));

  // Dibujar el snake
  for (int i = 0; i < snakeSize; i++) {
    dma_display->drawPixel(tailX[i], tailY[i], dma_display->color565(0, 255, 0));
  }
}

// Dirección del joystick
void setJoystickDirection() {
  int xValue = analogRead(X1_pin);
  int yValue = analogRead(Y1_pin);

  // Cambiar dirección solo si el joystick se mueve significativamente
  if (xValue > 3000 && direction != 'u') direction = 'd'; // Abajo
  else if (xValue < 1000 && direction != 'd') direction = 'u'; // Arrriba
  else if (yValue > 3000 && direction != 'r') direction = 'l'; // Izquierda
  else if (yValue < 1000 && direction != 'l') direction = 'r'; // Derecha
}

// Cambiar la dirección del snake
void changeSnakeDirection() {
  switch (direction) {
    case 'l': snakeX--; break;
    case 'r': snakeX++; break;
    case 'u': snakeY--; break;
    case 'd': snakeY++; break;
  }
}

// Control de los límites del snake
void manageSnakeOutOfBounds() {
  if (snakeX >= PANEL_RES_X) snakeX = 0;
  else if (snakeX < 0) snakeX = PANEL_RES_X - 1;

  if (snakeY >= PANEL_RES_Y) snakeY = 0;
  else if (snakeY < 0) snakeY = PANEL_RES_Y - 1;
}

// Función de finalización del juego
void manageGameOver() {
  // Colisión con la cola de la serpiente
  for (int i = 1; i < snakeSize; i++) {
    if (tailX[i] == snakeX && tailY[i] == snakeY) {
      isGameOver = true;
      return;
    }
    }
    // Colisión con las paredes
    if (snakeX == 0 || snakeX == PANEL_RES_X - 1 || snakeY == 0 || snakeY == PANEL_RES_Y - 1) {
      isGameOver = true;
  }
}

// Mostrar la pantalla de "Game Over"
void showGameOverScreen() {
  dma_display->clearScreen();
  dma_display->fillRect(0, 0, PANEL_RES_X, PANEL_RES_Y, dma_display->color565(255, 0, 0)); // Fondo rojo
  checkTimeGameOver();
  // Resetear variables del juego
  snakeX = PANEL_RES_X / 2;
  snakeY = PANEL_RES_Y / 2;
  setupFoodPosition();
  direction = ' ';
  isGameOver = false;
  isInConfigName = true;
  onePlayer = false;
  isInChoosePlayer = true;
  //Envío de datos a API
  if (username.length() > 0 && score >= 0 && gameTime >= 0) {
    sendDataToAPI(username, score, gameTime);
  } else {
    Serial.println("Datos inválidos. Intenta nuevamente.");
  }
  username = "";
  score = 0;
  gameTime = 0;
  snakeSize = 1;
  velocidad = 250;
}

//---------------------------------------------------- **CONTROL PARA JUEGO 2 JUGADORES** -----------------------------------------------------//
// Coordinación de la cola del snake para ambos jugadores
void manageSnakeTailCoordinates_2P(int player) {
  int *tailX = (player == 1) ? tailX1 : tailX2;
  int *tailY = (player == 1) ? tailY1 : tailY2;
  int snakeX = (player == 1) ? snakeX1 : snakeX2;
  int snakeY = (player == 1) ? snakeY1 : snakeY2;
  int snakeSize = (player == 1) ? snakeSize1 : snakeSize2;

  int previousX = tailX[0], previousY = tailY[0];
  tailX[0] = snakeX;
  tailY[0] = snakeY;
  
  for (int i = 1; i < snakeSize; i++) {
    int tempX = tailX[i], tempY = tailY[i];
    tailX[i] = previousX;
    tailY[i] = previousY;
    previousX = tempX;
    previousY = tempY;
  }
}

// Posición aleatoria de la comida 1
void setupFoodPosition1() {
  bool isValidPosition;
  do {
    foodX1 = random(1, (PANEL_RES_X/2) - 1);
    foodY1 = random(1, PANEL_RES_Y - 1);

    isValidPosition = true;
    for (int i = 0; i < snakeSize1; i++) {
      if (foodX1 == tailX1[i] && foodY1 == tailY1[i]) {
        isValidPosition = false;
        break;
      }
    }
  } while (!isValidPosition);
}

// Posición aleatoria de la comida 1
void setupFoodPosition2() {
  bool isValidPosition;
  do {
    foodX2 = random(33, PANEL_RES_X - 1);
    foodY2 = random(1, PANEL_RES_Y - 1);

    isValidPosition = true;
    for (int i = 0; i < snakeSize2; i++) {
      if (foodX2 == tailX2[i] && foodY2 == tailY2[i]) {
        isValidPosition = false;
        break;
      }
    }
  } while (!isValidPosition);
}

// Verificar si el snake ha comido la comida para ambos jugadores
void manageEatenFood_2P(int player) {
  int &snakeX = (player == 1) ? snakeX1 : snakeX2;
  int &snakeY = (player == 1) ? snakeY1 : snakeY2;
  int &snakeSize = (player == 1) ? snakeSize1 : snakeSize2;
  int &score = (player == 1) ? score1 : score2;

  if ((snakeX == foodX1 && snakeY == foodY1) || (snakeX == foodX2 && snakeY == foodY2)){
    score++;
    snakeSize++;
    if (player == 1) setupFoodPosition1(); else setupFoodPosition2();
    //if (velocidad > 50) velocidad -= 10;
  }
}

// Dibujar el juego
void drawGame_2P() {
  dma_display->clearScreen();

  for (int x = 0; x < PANEL_RES_X; x++) {
    dma_display->drawPixel(x, 0, dma_display->color565(255, 255, 0));
    dma_display->drawPixel(x, PANEL_RES_Y - 1, dma_display->color565(255, 255, 0));
  }
  for (int y = 0; y < PANEL_RES_Y; y++) {
    dma_display->drawPixel(0, y, dma_display->color565(255, 255, 0));
    dma_display->drawPixel(31, y, dma_display->color565(255, 255, 0));
    dma_display->drawPixel(32, y, dma_display->color565(255, 255, 0));
    dma_display->drawPixel(PANEL_RES_X - 1, y, dma_display->color565(255, 255, 0));
  }

  dma_display->drawPixel(foodX1, foodY1, dma_display->color565(255, 0, 0));
  dma_display->drawPixel(foodX2, foodY2, dma_display->color565(255, 0, 0));

  for (int i = 0; i < snakeSize1; i++) {
    dma_display->drawPixel(tailX1[i], tailY1[i], dma_display->color565(0, 255, 0));
  }
  for (int i = 0; i < snakeSize2; i++) {
    dma_display->drawPixel(tailX2[i], tailY2[i], dma_display->color565(255, 150, 20));
  }
}

// Dirección del joystick para el jugador 1
void setJoystickDirection1() {
  int xValue = analogRead(X1_pin);
  int yValue = analogRead(Y1_pin);
  char &direction = direction1;

  if (xValue > 3000 && direction != 'u') direction = 'd';
  else if (xValue < 1000 && direction != 'd') direction = 'u';
  else if (yValue > 3000 && direction != 'r') direction = 'l';
  else if (yValue < 1000 && direction != 'l') direction = 'r';
}

// Dirección del joystick para el jugador 2
void setJoystickDirection2() {
  int xValue = analogRead(X2_pin);
  int yValue = analogRead(Y2_pin);
  char &direction = direction2;

  if (xValue > 3000 && direction != 'u') direction = 'd';
  else if (xValue < 1000 && direction != 'd') direction = 'u';
  else if (yValue > 3000 && direction != 'r') direction = 'l';
  else if (yValue < 1000 && direction != 'l') direction = 'r';
}
// Cambiar la dirección del snake para el jugador 1
void changeSnakeDirection1() {
  char &direction = direction1;
  int &snakeX = snakeX1;
  int &snakeY = snakeY1;

  switch (direction) {
    case 'l': snakeX--; break;
    case 'r': snakeX++; break;
    case 'u': snakeY--; break;
    case 'd': snakeY++; break;
  }
}

// Cambiar la dirección del snake para el jugador 2
void changeSnakeDirection2() {
  char &direction = direction2;
  int &snakeX = snakeX2;
  int &snakeY = snakeY2;

  switch (direction) {
    case 'l': snakeX--; break;
    case 'r': snakeX++; break;
    case 'u': snakeY--; break;
    case 'd': snakeY++; break;
  }
}


// Control de límites del snake para ambos jugadores
void manageSnakeOutOfBounds_2P(int player) {
  int &snakeX = (player == 1) ? snakeX1 : snakeX2;
  int &snakeY = (player == 1) ? snakeY1 : snakeY2;

  if (player == 1) {
    if (snakeX < 0) snakeX = PANEL_RES_X / 2 - 1;
    else if (snakeX >= PANEL_RES_X / 2) snakeX = 0;
  } else {
    if (snakeX < PANEL_RES_X / 2) snakeX = PANEL_RES_X - 1;
    else if (snakeX >= PANEL_RES_X) snakeX = PANEL_RES_X / 2;
  }

  if (snakeY >= PANEL_RES_Y) snakeY = 0;
  else if (snakeY < 0) snakeY = PANEL_RES_Y - 1;
}

// Colisión con la cola de cada jugador y entre jugadores
void manageGameOver_2P() {
  for (int i = 1; i < snakeSize1; i++) {
    if (tailX1[i] == snakeX1 && tailY1[i] == snakeY1){
      isGameOver1 = true;
      isGameOver = true;
    }
  }

  for (int i = 1; i < snakeSize2; i++) {
    if (tailX2[i] == snakeX2 && tailY2[i] == snakeY2){
      isGameOver2 = true;
      isGameOver = true;
    }
  }
}

void showGameOverScreen_2P(){
  dma_display->clearScreen();
  if (isGameOver1 && isGameOver2){
    dma_display->setCursor(5, 10);
    dma_display->setTextColor(dma_display->color565(255, 255, 255));
    dma_display->setTextSize(1);
    dma_display->print("DRAW!");
  }else if (isGameOver1) {
    dma_display->setCursor(5, 10);
    dma_display->setTextColor(dma_display->color565(255, 255, 255));
    dma_display->setTextSize(1);
    dma_display->print("Player 1 Loses!");
  }else if (isGameOver2) {
    dma_display->setCursor(5, 10);
    dma_display->setTextColor(dma_display->color565(255, 255, 255));
    dma_display->setTextSize(1);
    dma_display->print("Player 2 Loses!");
  }
  //Resetear las variables
  snakeX1 = 15;
  snakeX2 = 50;
  snakeY1 = 10;
  snakeY2 = 10;
  setupFoodPosition1();
  setupFoodPosition2();
  direction1 = 'r';
  direction2 = 'r';
  twoPlayers = false;
  isGameOver1 = false;
  isGameOver2 = false;
  isInChoosePlayer = true;
  snakeSize1 = 1;
  snakeSize2 = 1;
  delay(1500);
}

//----------------------------------------------------- **PANTALLA MENU PRINCIPAL** -----------------------------------------------------------//

// Función para dibujar el menú principal
void drawMenu() {
    dma_display->clearScreen();

    // Dibujar el título "SNAKE"
    dma_display->setTextColor(dma_display->color565(255, 255, 0)); // Amarillo
    dma_display->setTextSize(1);
    dma_display->setCursor(18, 2);
    dma_display->print("SNAKE");

    // Dibujar las opciones del menú
    dma_display->setTextSize(1); // Tamaño pequeño para las opciones

    // Opción "Go"
    if (menuOption == 0) {
        dma_display->setTextColor(dma_display->color565(255, 0, 0)); // Rojo
        dma_display->setCursor(2, 10);
        dma_display->print("Go");
    } else {
        dma_display->setTextColor(dma_display->color565(0, 255, 0)); // Verde
        dma_display->setCursor(2, 10);
        dma_display->print("Go");
    }

    // Opción "Set"
    if (menuOption == 1) {
        dma_display->setTextColor(dma_display->color565(255, 0, 0)); // Rojo
        dma_display->setCursor(2, 18);
        dma_display->print("Set");
    } else {
        dma_display->setTextColor(dma_display->color565(0, 255, 0)); // Verde
        dma_display->setCursor(2, 18);
        dma_display->print("Set");
    }

    // Opción "Info"
    if (menuOption == 2) {
        dma_display->setTextColor(dma_display->color565(255, 0, 0)); // Rojo
        dma_display->setCursor(2, 25);
        dma_display->print("Info");
    } else {
        dma_display->setTextColor(dma_display->color565(0, 255, 0)); // Verde
        dma_display->setCursor(2, 25);
        dma_display->print("Info");
    }
}

// Función para cambiar la opción seleccionada dentro del menu principal
void changeMenuOption() {
  int xValue = analogRead(X1_pin);

  if (xValue > 3000 && menuOption < 2) {
    menuOption++; // Mover hacia abajo
    delay(200); // Espera para evitar cambios rápidos
  }
  if (xValue < 1000 && menuOption > 0) {
    menuOption--; // Mover hacia arriba
    delay(200); // Espera para evitar cambios rápidos
  }
}

// Función para seleccionar una opción
void selectMenuOption() {
  if (menuOption == 0) {
    inMenu = false; // Iniciar juego
    isInChoosePlayer = true; // Escoger jugadores
  }
  else if (menuOption == 1) {
    // Ir a configuración
    isInConfigMenu = true; // Ir al menú de configuración
    inMenu = false; // Salir del menú principal
  }
  else if (menuOption == 2) {
    // Ir a créditos
    isInInfo = true;
    inMenu = false;
  }
  Serial.println(menuOption);
}

//--------------------------------------------------- **PANTALLA MENU CONFIGURACIONES** ---------------------------------------------------------//

// Función para mostrar la pantalla de configuración
void drawConfigMenu() {
  dma_display->clearScreen();

  // Título "Config"
  dma_display->setTextColor(dma_display->color565(0, 255, 0)); // Amarillo
  dma_display->setTextSize(1);
  dma_display->setCursor(16, 2);
  dma_display->print("Config");

  // Opción "Brillo"
  if (configOption == 0) {
    dma_display->setTextColor(dma_display->color565(255, 0, 0)); // Rojo
    dma_display->setCursor(2, 10);
    dma_display->print("Brillo");
  } else {
    dma_display->setTextColor(dma_display->color565(255, 255, 0)); // Amarillo
    dma_display->setCursor(2, 10);
    dma_display->print("Brillo");
  }
  // Mostrar el nivel de brillo siempre al lado
  dma_display->print(":");
  dma_display->print(brightness);

  // Opción "Volumen"
  if (configOption == 1) {
    dma_display->setTextColor(dma_display->color565(255, 0, 0)); // Rojo
    dma_display->setCursor(2, 18);
    dma_display->print("Volumen");
  } else {
    dma_display->setTextColor(dma_display->color565(255, 255, 0)); // Amarillo
    dma_display->setCursor(2, 18);
    dma_display->print("Volumen");
  }
  // Mostrar el nivel de volumen siempre al lado
  dma_display->print(":");
  dma_display->print(volume);

  // Dibujar los tres puntos al final
  if (configOption == 2){
    dma_display->setTextColor(dma_display->color565(255, 0, 0)); // Amarillo
    dma_display->setCursor(2, 36);
    dma_display->print("...");
  }else{
    dma_display->setTextColor(dma_display->color565(255, 255, 0)); // Amarillo
    dma_display->setCursor(2, 36);
    dma_display->print("...");
  }
}


// Función para manejar la navegación en el menú de configuración
void changeConfigOption() {
  int xValue_CCO = analogRead(X1_pin);

  if (xValue_CCO > 3000 && configOption < 1) {
    configOption++; // Mover hacia abajo
    delay(200); // Espera para evitar cambios rápidos
  }
  if (xValue_CCO < 1000 && configOption > 0) {
    configOption--; // Mover hacia arriba
    delay(200); // Espera para evitar cambios rápidos
  }
}

// Función para ajustar el brillo o el volumen
void adjustConfigValue() {
  int yValue = analogRead(Y2_pin);

  if (configOption == 0) { // Ajustar brillo
    if (yValue > 3000 && brightness <= 255) {
      brightness -= 5; // Decrementar brillo
    } else if (yValue < 1000 && brightness >= 0) {
      brightness += 5; // Incrementar brillo
    }
    brightness = constrain(brightness, 0, 255); // Limitar el brillo entre 0 y 255
    dma_display->setBrightness8(brightness); // Aplicar el brillo
  } else if (configOption == 1) { // Ajustar volumen
    if (yValue > 3000 && volume <= 30) {
      volume--; // Decrementar volumen
    } else if (yValue < 1000 && volume >= 0) {
      volume++; // Incrementar volumen
    }
    volume = constrain(volume, 0, 30); // Limitar el volumen entre 0 y 30
    myDFPlayer.volume(volume); // Aplicar el volumen
  }
  delay(200); // Espera para evitar ajustes rápidos
}

//--------------------------------------------------- **PANTALLA PARA CREDITOS** ---------------------------------------------------------//

// Función para dibujar la pantalla de información
void drawInfoScreen() {
    dma_display->clearScreen();

    // Mensaje "Hecho por Miguel y Edu"
    dma_display->setTextColor(dma_display->color565(0, 255, 0)); // Verde
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 2);
    dma_display->print("Hecho por");
    dma_display->setTextColor(dma_display->color565(255, 255, 0)); // Verde
    dma_display->setCursor(2, 12);
    dma_display->print("Miguel y");
    dma_display->setCursor(2, 22);
    dma_display->print("Edu");
}

//--------------------------------------------------- **PANTALLA PARA NOMBRE** ---------------------------------------------------------//

// Función para configurar el nombre
void configureUsername() {
  dma_display->clearScreen();
  dma_display->setTextSize(1);
  char currentChar = 'A'; // Letra inicial
  int letterIndex = 0; // Índice de la letra (0 a 3)
  char tempUsername[5] = "AAAA"; // Temporal para mostrar el nombre en edición
  bool isConfirming = false;

  while (!isConfirming) {
    dma_display->clearScreen();
    dma_display->setCursor(5, 2);
    dma_display->setTextColor(dma_display->color565(255, 255, 255)); // Blanco

    // Mostrar el nombre en edición
    dma_display->setTextColor(dma_display->color565(255, 255, 0)); // Amarillo
    dma_display->print("NAME: ");
    dma_display->setCursor(36, 2);
    dma_display->setTextColor(dma_display->color565(255, 255, 255)); // Blanco
    dma_display->print(tempUsername);

    // Mostrar instrucción
    dma_display->setCursor(5, 10);
    dma_display->setTextColor(dma_display->color565(255, 255, 0)); // Amarillo
    dma_display->print("EDIT: ");
    dma_display->setTextColor(dma_display->color565(255, 255, 255)); // Blanco
    dma_display->print(tempUsername[letterIndex]);

    // Leer valores del joystick
    int yValue = analogRead(Y1_pin);

    // Cambiar letra con joystick hacia arriba/abajo
    if (yValue < 1000) { // Joystick hacia arriba
      currentChar++;
      if (currentChar > 'Z') currentChar = 'A'; // Ciclo de A a Z
    } else if (yValue > 3000) { // Joystick hacia abajo
      currentChar--;
      if (currentChar < 'A') currentChar = 'Z'; // Ciclo inverso
    }

    // Mostrar la letra actual en edición
    dma_display->setCursor(5, 22);
    dma_display->setTextColor(dma_display->color565(255, 255, 0)); // Amarillo
    dma_display->print("LETTER: ");
    dma_display->setTextColor(dma_display->color565(255, 255, 255)); // Blanco
    dma_display->print(currentChar);

    // Comprobar si se pulsa el botón
    if (isButtonPressed()) {
      tempUsername[letterIndex] = currentChar; // Guardar letra seleccionada
      letterIndex++; // Pasar a la siguiente letra
      delay(300); // Debounce
    }

    // Confirmar cuando se hayan configurado las 4 letras
    if (letterIndex >= 4) {
      isConfirming = true;
      tempUsername[4] = '\0'; // Asegurar el término de la cadena
    }

    delay(50); // Evitar lecturas repetitivas
  }

  // Guardar el nombre configurado en la variable global username
  username = String(tempUsername);

  // Mostrar mensaje de confirmación
  dma_display->clearScreen();
  dma_display->setCursor(5, 5);
  dma_display->print("NAME SET!");
  dma_display->setCursor(5, 20);
  dma_display->print(username);
  isInConfigName = false;
  delay(2000);
}

//--------------------------------------------------- **PANTALLA PARA ESCOGER JUGADORES** -------------------------------------------------------//

void drawChoosePlayer(){
    dma_display->clearScreen();
    // Dibujar las opciones del menú
    dma_display->setTextSize(1); // Tamaño pequeño para las opciones

    // Opción "1P"
    if (playerOption == 0) {
        dma_display->setTextColor(dma_display->color565(255, 0, 0)); // Rojo
        dma_display->setCursor(12, 12);
        dma_display->print("1P");
    } else {
        dma_display->setTextColor(dma_display->color565(0, 255, 0)); // Verde
        dma_display->setCursor(12, 12);
        dma_display->print("1P");
    }

    // Opción "2P"
    if (playerOption == 1) {
        dma_display->setTextColor(dma_display->color565(255, 0, 0)); // Rojo
        dma_display->setCursor(40, 12);
        dma_display->print("2P");
    } else {
        dma_display->setTextColor(dma_display->color565(0, 255, 0)); // Verde
        dma_display->setCursor(40, 12);
        dma_display->print("2P");
    }
}

// Función para cambiar la opción seleccionada
void changePlayerOption() {
  int yValue = analogRead(Y1_pin);

  if (yValue > 3000 && playerOption > 0) {
    playerOption--; // Mover hacia izquierda
    delay(200); // Espera para evitar cambios rápidos
  }
  if (yValue < 1000 && playerOption < 1) {
    playerOption++; // Mover hacia derecha
    delay(200); // Espera para evitar cambios rápidos
  }
}

// Función para seleccionar una opción
void selectPlayerOption() {
  if (playerOption == 0) {
    onePlayer = true;
    twoPlayers = false;
    isInChoosePlayer = false;
  }
  else if (playerOption == 1) {
    onePlayer = false;
    twoPlayers = true;
    isInChoosePlayer = false;
  }
}

//--------------------------------------------------- **PETICIÓN POST** ---------------------------------------------------------//
void sendDataToAPI(String username, int score, unsigned long gameTime) {
  if (WiFi.status() == WL_CONNECTED) {
    client.setCACert(rootCACertificate); // Configura el certificado
    if (!client.connect("eduisant.pythonanywhere.com", 443)) {
      Serial.println("Fallo al conectar con el servidor.");
      return;
    }

    HTTPClient http;
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Crear objeto JSON
    StaticJsonDocument<200> jsonDoc; // Usar StaticJsonDocument sigue siendo válido aquí
    jsonDoc["username"] = username;
    jsonDoc["score"] = score;
    jsonDoc["game_time"] = String(gameTime); // Convierte el tiempo en formato string

    // Serializar el objeto JSON
    String jsonString;
    serializeJson(jsonDoc, jsonString);

    // Mostrar los datos a enviar
    Serial.println("Enviando los siguientes datos:");
    Serial.println(jsonString);

    // Enviar la solicitud POST
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode > 0) {
      Serial.print("Código de respuesta: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.print("Respuesta del servidor: ");
      Serial.println(response);
    } else {
      Serial.print("Error al enviar la solicitud: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end(); // Finaliza la conexión
  } else {
    Serial.println("WiFi desconectado. No se puede enviar la solicitud.");
  }
}

//--------------------------------------------------- **SET UP** ---------------------------------------------------------//

void setup(){
  Serial.begin(115200);
  intentoconexion("espgroup3", "12341243");
  // Module configuration
  HUB75_I2S_CFG::i2s_pins _pins={R1, G1, BL1, R2, G2, BL2, CH_A, CH_B, CH_C, CH_D, CH_E, LAT, OE, CLK};
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X, // module width
    PANEL_RES_Y, // module height
    PANEL_CHAIN , // Chain length
    _pins
  );

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(brightness); //0-255
  dma_display->clearScreen();
  setupFoodPosition();
  setupFoodPosition1();
  setupFoodPosition2();
  pinMode(SW_pin, INPUT_PULLUP);

  //Configuracion para DFPlayer mini
  Serial.println("Inicializando DFPlayer Mini...");
  mySerial.begin(9600, SERIAL_8N1, 2, 0); // RX en GPIO 2, TX en GPIO 0

  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("Error al inicializar DFPlayer Mini:");
    while (true);
  }
  Serial.println("DFPlayer Mini inicializado correctamente.");
  myDFPlayer.volume(volume);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  stopGameMusic();
  startGameMusic(1);

  // Inicializar posición del snake
  tailX[0] = snakeX;
  tailY[0] = snakeY;
  direction = ' '; // Comienza detenido
};

//--------------------------------------------------- **LOOP** ---------------------------------------------------------//

void loop(){
  loopAP();
  if (inMenu) {
    drawMenu();
    changeMenuOption();
    if ((isButtonPressed()) && (!isInConfigMenu) && (!isInInfo)){
      selectMenuOption();
      inGame = true;
    }
  } else if (isInConfigMenu) {
    drawConfigMenu();
    changeConfigOption();
    adjustConfigValue();
    if (isButtonPressed()) {
      inMenu = true; // Volver al menú principal
      isInConfigMenu = false; // Salir del menú de configuración
      delay(100);
    }
  } else if (isInInfo){
    drawInfoScreen();
    if (isButtonPressed()) {
      inMenu = true; // Volver al menú principal
      isInInfo= false; // Salir de creditos
    }
  }else if (inGame){
    if (isInChoosePlayer){
      drawChoosePlayer();
      changePlayerOption();
      if (isButtonPressed()){
        selectPlayerOption();
        startGameTimer();
      }
    }else if(onePlayer){
      if (!isGameOver) {
        if (isInConfigName) configureUsername();
        else{
          setJoystickDirection();
          changeSnakeDirection();
          manageSnakeTailCoordinates();
          manageEatenFood();
          manageSnakeOutOfBounds();
          checkTimeGameOver();
          manageGameOver();
          drawGame();
          delay(velocidad);
        }
      } else {
        showGameOverScreen();
        inGame = false;
        inMenu = true; // Volver al menú principal tras el "Game Over"
        delay(1000);
      }
    }else if (twoPlayers){
      if (!isGameOver1 && !isGameOver2){
        setJoystickDirection1();
        setJoystickDirection2();

        changeSnakeDirection1();
        changeSnakeDirection2();

        manageSnakeTailCoordinates_2P(1);
        manageSnakeTailCoordinates_2P(2);

        manageEatenFood_2P(1);
        manageEatenFood_2P(2);

        manageSnakeOutOfBounds_2P(1);
        manageSnakeOutOfBounds_2P(2);

        checkScore();
        manageGameOver_2P();

        drawGame_2P();
        delay(velocidad_2P);
      }else{
        showGameOverScreen_2P();
        inGame = false;
        inMenu = true;
        delay(1000);
      }
    }
  }
}