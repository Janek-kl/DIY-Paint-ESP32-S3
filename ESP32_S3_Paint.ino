#include <SD.h>
#include <SPI.h>
#include <ESP32Encoder.h>
#include <TFT_eSPI.h>
#include <FS.h>
#include "LittleFS.h"
#include <TJpg_Decoder.h>
//#include "icons.h" //w przyszłości lepsze ikony do menu

TFT_eSPI tft = TFT_eSPI();
#define TOUCH_IRQ 7

/* // ESP32
#define ENCODER_SW 14
#define ENCODER_CLK 26
#define ENCODER_DT 27
*/

#define ENCODER_SW 4
#define ENCODER_CLK 6
#define ENCODER_DT 5
#define ENCODER_POWER 16
ESP32Encoder encoder;

unsigned long czasWcisniecia = 0;
bool przyciskBytWcisniety = false;

uint32_t aktywnySlot = 1;

#define SLOT_MAX_FS 5
#define SLOT_MAX_SD 999

int staryKolor = TFT_WHITE;

uint16_t kolor = TFT_WHITE;

int kolorH = 0;
int kolorS = 255;
int kolorV = 255;

bool menuChangeColorON = 0;

#define COLOR_MENU_SELECT_H 1
#define COLOR_MENU_SELECT_S 2
#define COLOR_MENU_SELECT_V 3

int menuChangeColorSelectHSV = 1;

bool menuChangeToolON = 0;



#define TOOL_SINGLE_DOT 1
#define TOOL_CIRCLE 2
#define TOOL_RECT 3
#define TOOL_ELLIPSE 4
#define TOOL_TRIANGLE 5

int activeToolID = TOOL_SINGLE_DOT;
int ToolSize = 1;

// ESP32 #define SD_CS 13
#define SD_SCLK 21
#define SD_MISO 47
#define SD_MOSI 20
#define SD_CS 17

#define SD_SPEED 10000000  //10MHz
SPIClass sdSPI(FSPI);


bool InternalStorageMode = 1;
bool StartInternalStorageMode = 1;

bool menuON = 0;

// Flaga informująca pętlę główną, że wykryto dotyk
volatile bool dotykWykryty = false;

// Funkcja obsługi przerwania (musi być bardzo krótka!)
void IRAM_ATTR obsługaDotyku() {
  dotykWykryty = true;
}

long aktualnaPozycja = 0;

void setup() {
  Serial.begin(115200);

  LittleFS.begin(true);  // inicjalizacja systemu do zapisywania plików w flash storage

  encoder.attachHalfQuad(ENCODER_CLK, ENCODER_DT);

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(TOUCH_IRQ, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(TOUCH_IRQ), obsługaDotyku, FALLING);  //przerwanie od dotyku

  esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_IRQ, 0);  //ustawienie wybadzania esp przez sygnał z wyświetlacza o dotknięciu

  pinMode(SD_CS, OUTPUT);
  pinMode(ENCODER_POWER, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(ENCODER_POWER, HIGH);


  encoder.setCount(0);  //zerujemy enkoder

  tft.begin();  //inicjalizacja wyświetlacza
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);  // wypełniamy wyświetlacz kolorem czarnym


  uint16_t calData[5] = { 286, 3589, 296, 3452, 7 };  // dane do kalibracji dotyku wyświetlacza
  tft.setTouch(calData);                              //kalibrowanie wyświetlacza na podstawie powyższych danych


  //--JPEG
  TJpgDec.setJpgScale(1);

  TJpgDec.setCallback(tft_output);  // ustawiamy jaka funkcja wświetla
  TJpgDec.setSwapBytes(true);

  //ustawiamy piny CS w stanie HIGH - Nieaktywny
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);  // tworzymy kolejną magistralę spi
  if (!SD.begin(SD_CS, sdSPI, SD_SPEED)) {        // jeśli inicjalizacja karty SD się nie powiodła
    tft.setTextColor(TFT_RED, TFT_DARKGREEN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(3);
    tft.drawString("BLAD KARTY SD", 160, 120, 2);
    InternalStorageMode = 1;       // zmienna informuje że zapisujemy i czytamy  pliki z FS
    StartInternalStorageMode = 1;  // zmienna informuje że nie mamy karty SD
    delay(2000);                   // przerwa by komunikat był widoczny
    wczytajZeSlotuFS(1);           // odczytujemy i wyświetlamy obraz z pamięci wewnętrznej
  } else {
    InternalStorageMode = 0;
    StartInternalStorageMode = 0;

    wczytajZeSlotuSD(1);  // odczytujemy i wyświetlamy obraz z karty SD
  }
  tft.setTextSize(1);
}


void loop() {

  if (menuON == 0) { // jeśli nie jest aktywne menu pozwalamy rysować
    uint16_t x = 0, y = 0;

    if (tft.getTouch(&x, &y)) {
      drawWithTool(activeToolID, x, y, kolor, ToolSize);
      dotykWykryty = 0;
    }
  }

  bool stanPrzycisku = (digitalRead(ENCODER_SW) == LOW);

  if (stanPrzycisku == 1) { //jeśli wciśnięto przycisk na enkoderze zmień stan otwarcia menu
    dotykWykryty = 0;
    zmienMenu();
    delay(100);
  }

  if (menuON == 1) { //jeśli menu otwarte obsługujemy menu
    obslozDotyk();
  }

  /*
      InternalStorageMode = !InternalStorageMode;
      if (InternalStorageMode == 1) {
        tft.drawString("IM", 0, 0, 2);
      } else {
        tft.drawString("EM", 0, 0, 2);
      }
  
  */
}

void drawWithTool(int tool, int x, int y, int kolor1, int ToolSize1) {
  if (tool == TOOL_SINGLE_DOT) {
    tft.drawPixel(x, y, kolor1);
  } else if (tool == TOOL_CIRCLE) {
    tft.fillCircle(x, y, ToolSize1, kolor1);
  } else if (tool == TOOL_ELLIPSE) {
    if (ToolSize1 == 1) ToolSize1 = 2;
    tft.fillEllipse(x, y, ToolSize1 / 2, ToolSize1, kolor1);
  } else if (tool == TOOL_RECT) {
    int s = ToolSize1;
    tft.fillRect(x - s / 2, y - s / 2, s, s, kolor1);
  } else if (tool == TOOL_TRIANGLE) {
    int s = ToolSize1;
    tft.fillTriangle(
      x - s / 2, y + s / 2,  // lewy dolny
      x, y - s / 2,          // górny wierzchołek (środek góry)
      x + s / 2, y + s / 2,  // prawy doln
      kolor1);
  }
}

void menuAllExit() {
  menuON = 0;
  menuChangeColorON = 0;
  menuChangeToolON = 0;
}

void menuPageNext() {
  aktywnySlot++;
  if ((aktywnySlot > SLOT_MAX_FS && InternalStorageMode == 1) || (aktywnySlot > SLOT_MAX_SD && InternalStorageMode == 0)) aktywnySlot = 1;

  tft.setTextSize(8);
  tft.drawNumber(aktywnySlot, 0, 0, 2);  //Poprzednia
  delay(750);

  if (InternalStorageMode == 1) {
    wczytajZeSlotuFS(aktywnySlot);
  } else {
    wczytajZeSlotuSD(aktywnySlot);
  }

  menuON = 0;
}

void menuPagePrevious() {
  aktywnySlot--;
  if (aktywnySlot < 1) {
    if (InternalStorageMode == 1) {
      aktywnySlot = SLOT_MAX_FS;  // Maksymalny slot dla pamięci wewnętrznej
    } else {
      aktywnySlot = SLOT_MAX_SD;  // Maksymalny slot dla karty SD
    }
  }
  tft.setTextSize(8);
  tft.drawNumber(aktywnySlot, 0, 0, 2);  // Wyświetlenie nowego numeru slotu
  delay(750);

  // Wczytanie danych z nowego, cofniętego slotu
  if (InternalStorageMode == 1) {
    wczytajZeSlotuFS(aktywnySlot);
  } else {
    wczytajZeSlotuSD(aktywnySlot);
  }
  menuON = 0;  // wyjście z menu
}


void menuSave(bool komunikat) {
  if (komunikat == 1) {
    tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Zapisywanie Obrazu", 160, 120, 2);
    delay(750);
  }
  zmienMenu();
  if (InternalStorageMode == 0) {
    zapiszDoSlotuSD(aktywnySlot);
  } else {
    zapiszDoSlotuFS(aktywnySlot);
  }
}

void menuLoad(bool komunikat) {

  if (komunikat == 1) {
    tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Wczytywanie Obrazu", 160, 120, 2);
    delay(750);
  }
  menuON = 0;
  if (InternalStorageMode == 0) {
    wczytajZeSlotuSD(aktywnySlot);
  } else {
    wczytajZeSlotuFS(aktywnySlot);
  }
}

void menuDisplayClear(bool komunikat) {
  if (komunikat == 1) {
    tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Wyczyszczono Ekran", 160, 120, 2);
    delay(750);
  }
  menuON = 0;
  tft.fillScreen(TFT_BLACK);
}

void drawHSVLine(int lineType, int xs, int y, int h) {
  int xe = xs + 255;
  if (lineType == 1) {  // Hue
    for (int i = xs; i <= xe; i++) {
      int x = constrain(i - xs, 0, 255);
      int H = map(x, 0, 255, 0, 359);
      int S = kolorS;
      int V = kolorV;
      for (int a = 1; a <= h; a++) {
        tft.drawPixel(i, (y + a), hsv2rgb565(H, S, V));
      }
    }
  } else if (lineType == 2) {  //Saturation
    for (int i = xs; i <= xe; i++) {
      int x = constrain(i - xs, 0, 255);
      int H = kolorH;
      int S = x;
      int V = kolorV;
      for (int a = 1; a <= h; a++) {
        tft.drawPixel(i, (y + a), hsv2rgb565(H, S, V));
      }
    }
  } else if (lineType == 3) {  //brightnes
    for (int i = xs; i <= xe; i++) {
      int x = constrain(i - xs, 0, 255);
      int H = kolorH;
      int S = kolorS;
      int V = x;
      for (int a = 1; a <= h; a++) {
        tft.drawPixel(i, (y + a), hsv2rgb565(H, S, V));
      }
    }
  } else {
    tft.drawLine(xs, y, xe, y, TFT_WHITE);
  }
}

void menuChangeColor() {
  static bool firstRun = true;
  uint16_t x1 = 0, y1 = 0;
  bool changed = false;

  // Pierwsze uruchomienie
  if (menuChangeColorON == 0) {
    tft.fillScreen(TFT_BLACK);
    encoder.setCount(map(kolorH, 0, 359, 0, 255));
  }

  // Odczyt encodera
  aktualnaPozycja = encoder.getCount();
  if (aktualnaPozycja > 255) {
    encoder.setCount(0);
    aktualnaPozycja = 0;
  }
  if (aktualnaPozycja < 0) {
    encoder.setCount(255);
    aktualnaPozycja = 255;
  }
  aktualnaPozycja = constrain(aktualnaPozycja, 0, 255);

  // === OBSŁUGA DOTYKU ===
  if (tft.getTouch(&x1, &y1)) {
    if (y1 < 80) {
      if (menuChangeColorSelectHSV != COLOR_MENU_SELECT_H) {
        menuChangeColorSelectHSV = COLOR_MENU_SELECT_H;
        encoder.setCount(map(kolorH, 0, 359, 0, 255));
        changed = true;
      }
    } else if (y1 < 160) {
      if (menuChangeColorSelectHSV != COLOR_MENU_SELECT_S) {
        menuChangeColorSelectHSV = COLOR_MENU_SELECT_S;
        encoder.setCount(kolorS);
        changed = true;
      }
    } else {
      if (menuChangeColorSelectHSV != COLOR_MENU_SELECT_V) {
        menuChangeColorSelectHSV = COLOR_MENU_SELECT_V;
        encoder.setCount(kolorV);
        changed = true;
      }
    }
    int touchValue = map(x1, 30, 285, 0, 255);
    touchValue = constrain(touchValue, 0, 255);

    if (y1 >= 55 && y1 <= 190) {
      aktualnaPozycja = touchValue;
      encoder.setCount(touchValue);
      changed = true;
    }

    delay(25);
  }

  // === AKTUALIZACJA WARTOŚCI HSV ===
  switch (menuChangeColorSelectHSV) {
    case COLOR_MENU_SELECT_H:
      {
        int newH = map(aktualnaPozycja, 0, 255, 0, 359);
        if (newH != kolorH) {
          kolorH = newH;
          changed = true;
        }
      }
      break;

    case COLOR_MENU_SELECT_S:
      if (aktualnaPozycja != kolorS) {
        kolorS = aktualnaPozycja;
        changed = true;
      }
      break;

    case COLOR_MENU_SELECT_V:
      if (aktualnaPozycja != kolorV) {
        kolorV = aktualnaPozycja;
        changed = true;
      }
      break;
  }

  // Zabezpieczenie zakresów
  kolorH = constrain(kolorH, 0, 359);
  kolorS = constrain(kolorS, 0, 255);
  kolorV = constrain(kolorV, 0, 255);

  // === RYSOWANIE ===
  if (changed || menuChangeColorON == 0 || firstRun) {
    kolor = hsv2rgb565(kolorH, kolorS, kolorV);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(BL_DATUM);
    tft.setTextSize(1.5);

    tft.fillRect(0, 0, 320, 20, kolor);

    // Hue
    int HPX = map(kolorH, 0, 359, 0, 255) + 30;
    tft.drawString("Kolor", 32, 50, 2);
    tft.fillRect(20, 55, 280, 15, TFT_BLACK);
    drawHSVLine(1, 30, 60, 3);
    tft.fillCircle(HPX, 61, 6, (menuChangeColorSelectHSV == COLOR_MENU_SELECT_H) ? TFT_LIGHTGREY : TFT_DARKGREY);

    // Saturation
    int SPX = kolorS + 30;
    tft.drawString("Nasycenie", 32, 110, 2);
    tft.fillRect(20, 115, 280, 15, TFT_BLACK);
    drawHSVLine(2, 30, 120, 3);
    tft.fillCircle(SPX, 121, 6, (menuChangeColorSelectHSV == COLOR_MENU_SELECT_S) ? TFT_LIGHTGREY : TFT_DARKGREY);

    // Value
    int VPX = kolorV + 30;
    tft.drawString("Jasnosc", 32, 170, 2);
    tft.fillRect(20, 175, 280, 15, TFT_BLACK);
    drawHSVLine(3, 30, 180, 3);
    tft.fillCircle(VPX, 181, 6, (menuChangeColorSelectHSV == COLOR_MENU_SELECT_V) ? TFT_LIGHTGREY : TFT_DARKGREY);

    firstRun = false;
    menuChangeColorON = 1;
  }
}

void menuToolDrawBlueBoxes(int tool) {
  tft.drawRect(0, 0, 64, 64, TFT_BLUE);    //TOOL_SINGLE_DOT
  tft.drawRect(64, 0, 64, 64, TFT_BLUE);   //TOOL_RECT
  tft.drawRect(128, 0, 64, 64, TFT_BLUE);  //TOOL_CIRCLE
  tft.drawRect(192, 0, 64, 64, TFT_BLUE);  //TOOL_ELLIPSE
  tft.drawRect(256, 0, 64, 64, TFT_BLUE);  //TOOL_TRIANGLE

  if (tool == TOOL_SINGLE_DOT) {
    tft.drawRect(0, 0, 64, 64, TFT_RED);  //TOOL_SINGLE_DOT
  } else if (tool == TOOL_RECT) {
    tft.drawRect(64, 0, 64, 64, TFT_RED);  //TOOL_RECT
  } else if (tool == TOOL_CIRCLE) {
    tft.drawRect(128, 0, 64, 64, TFT_RED);  //TOOL_CIRCLE
  } else if (tool == TOOL_ELLIPSE) {
    tft.drawRect(192, 0, 64, 64, TFT_RED);  //TOOL_ELLIPSE
  } else if (tool == TOOL_TRIANGLE) {
    tft.drawRect(256, 0, 64, 64, TFT_RED);  //TOOL_TRIANGLE
  }
}

void menuChangeTool() {
  uint16_t x1 = 0, y1 = 0;
  bool changed = false;
  if (menuChangeToolON == 0) {
    menuChangeToolON = 1;
    changed = true;

    encoder.setCount(ToolSize);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(BL_DATUM);
    tft.setTextSize(1.5);

    tft.drawString("Rozmiar", 22, 190, 2);

    menuToolDrawBlueBoxes(activeToolID);

    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("DOT", 32, 32, 2);
    tft.fillRect(74, 10, 44, 44, TFT_LIGHTGREY);
    tft.fillCircle(159, 32, 22, TFT_LIGHTGREY);
    tft.fillEllipse(223, 32, 11, 22, TFT_LIGHTGREY);
    tft.fillTriangle(266, 54, 288, 10, 310, 54, TFT_LIGHTGREY);
  }
  if (aktualnaPozycja != encoder.getCount()) {
    aktualnaPozycja = encoder.getCount();
    if (aktualnaPozycja > 61) {
      encoder.setCount(61);
      aktualnaPozycja = 61;
    }
    if (aktualnaPozycja < 1) {
      encoder.setCount(1);
      aktualnaPozycja = 1;
    }
    aktualnaPozycja = constrain(aktualnaPozycja, 1, 61);

    ToolSize = aktualnaPozycja;
    changed = true;
  }

  if (tft.getTouch(&x1, &y1)) {
    if (y1 <= 64) {
      if (x1 <= 64) {  // lewa
        tft.drawRect(0, 0, 64, 64, TFT_RED);
        activeToolID = TOOL_SINGLE_DOT;
      } else if (x1 <= 128) {
        tft.drawRect(64, 0, 64, 64, TFT_RED);
        activeToolID = TOOL_RECT;
      } else if (x1 <= 192) {
        tft.drawRect(128, 0, 64, 64, TFT_RED);
        activeToolID = TOOL_CIRCLE;
      } else if (x1 <= 256) {
        tft.drawRect(192, 0, 64, 64, TFT_RED);
        activeToolID = TOOL_ELLIPSE;
      } else {  //prawa
        tft.drawRect(256, 0, 64, 64, TFT_RED);
        activeToolID = TOOL_TRIANGLE;
      }
      changed = true;
      menuToolDrawBlueBoxes(activeToolID);
    } else if (y1 >= 160) {
      aktualnaPozycja = map(constrain(x1, 20, 140), 20, 140, 1, 61);
      encoder.setCount(aktualnaPozycja);
      ToolSize = constrain(aktualnaPozycja, 1, 61);
      changed = true;
    }
  }

  if (changed) {

    tft.fillRect(10, 190, 150, 20, TFT_BLACK);   //zamalowywanie suwaka rozmiaru
    tft.fillRect(150, 65, 190, 150, TFT_BLACK);  //zamalowywanie przykładu koła

    tft.drawLine(20, 200, 140, 200, TFT_LIGHTGREY);
    tft.drawLine(20, 201, 140, 201, TFT_LIGHTGREY);
    tft.drawLine(20, 202, 140, 202, TFT_LIGHTGREY);
    tft.fillCircle((ToolSize * 2 + 19), 201, 6, TFT_WHITE);

    drawWithTool(activeToolID, 240, 140, kolor, ToolSize);
  }
}

void obslozDotyk() {

  if (menuChangeColorON == 1) {
    menuChangeColor();
    return;
  } else if (menuChangeToolON == 1) {
    menuChangeTool();
    return;
  }

  uint16_t x1 = 0, y1 = 0;

  if (tft.getTouch(&x1, &y1) && dotykWykryty == 1) {
    if (x1 > 240) {    // prawo
      if (y1 > 120) {  //dół
        menuLoad(1);
      } else {  //góra
        menuSave(1);
      }
    } else if (x1 > 160) {  //środek-prawy
      if (y1 > 120) {       //dół
        menuPagePrevious();
      } else {  //góra
        menuPageNext();
      }
    } else if (x1 > 80) {  //środek-lewy
      if (y1 > 120) {      //dół
        menuDisplayClear(1);
      } else {  //góra
        tft.fillScreen(TFT_WHITE);
        delay(1000);
        esp_deep_sleep_start();
      }
    } else {           //lewo
      if (y1 > 120) {  //dół
        menuChangeTool();
      } else {  //góra
        menuChangeColor();
      }
    }
  }
}

void wypiszOpcje() {
  tft.setTextColor(TFT_RED, TFT_DARKGREEN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(4);
  tft.drawString("Sa", 240, 0, 2);    //Save
  tft.drawString("Re", 240, 121, 2);  //Read
  tft.drawString("P+", 160, 0, 2);    //Next
  tft.drawString("P-", 160, 121, 2);  //Poprzednia
  tft.drawString("OF", 80, 0, 2);     //Show COlor
  tft.drawString("CL", 80, 121, 2);   //wyczyść

  tft.drawString("CC", 0, 0, 2);    //Change Color
  tft.drawString("CT", 0, 121, 2);  //Change Tool
}

void zmienMenu() {
  if (menuON == 0) {
    menuON = 1;
    if (InternalStorageMode == 0) {
      zapiszDoSlotuSD(0);
    } else {
      zapiszDoSlotuFS(0);
    }
    wypiszOpcje();
    return;
  }

  if (menuON == 1) {
    if (InternalStorageMode == 0) {
      wczytajZeSlotuSD(0);
    } else {
      wczytajZeSlotuFS(0);
    }
    menuAllExit();
  }
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return false;  // Zabezpieczenie przed rysowaniem poza ekranem
  tft.pushImage(x, y, w, h, bitmap);    // Wysłanie bloku pikseli na ekran TFT
  return true;
}


//-- INTERNAL STORAGE --

// Funkcja zapisu ekranu do pamięci Flash (Sloty 1-6)
void zapiszDoSlotuFS(int slot) {
  String sciezka = "/slot" + String(slot) + ".raw";

  if (LittleFS.exists(sciezka))
    LittleFS.remove(sciezka);

  File plik = LittleFS.open(sciezka, FILE_WRITE);
  if (!plik) {
    return;
  }

  // Bufor na jedną linię ekranu (320 pikseli * 2 bajty = 640 bajtów w RAM)
  uint16_t linia[320];

  // Czytamy ekran linia po linii (wysokość ekranu to 240)
  for (int y = 0; y < 240; y++) {
    tft.readRect(0, y, 320, 1, linia);     // Pobiera 320 pikseli z linii 'y'
    plik.write((uint8_t*)linia, 320 * 2);  // Zapisuje te bajty do pamięci Flash
  }

  plik.close();
}

// Funkcja odczytu z pamięci Flash i rysowania na ekranie
void wczytajZeSlotuFS(int slot) {
  String sciezka = "/slot" + String(slot) + ".raw";
  if (!LittleFS.exists(sciezka)) {
    tft.fillScreen(kolor);
    tft.fillRect(0, 0, 16, 16, kolor);
    return;
  }

  File plik = LittleFS.open(sciezka, FILE_READ);
  if (!plik) {
    tft.fillScreen(kolor);
    tft.fillRect(0, 0, 16, 16, kolor);
    return;
  }

  uint16_t linia[320];

  // Rysujemy ekran linia po linii
  for (int y = 0; y < 240; y++) {
    plik.read((uint8_t*)linia, 320 * 2);  // Wczytuje linię z Flasha do RAMu
    tft.pushImage(0, y, 320, 1, linia);   // Wrzuca linię bezpośrednio na ekran
  }

  plik.close();
}

//-- INTERNAL SSTORAGE END --

//-- EXTERNAL STORAGE (SD) --

// Funkcja zapisu ekranu na kartę SD (sloty 1-99)
void zapiszDoSlotuSD(int slot) {
  String sciezka = "/s" + String(slot) + ".raw";

  // 1. Obliczamy rozmiar całego ekranu w bajtach
  // 320 pikseli szerokości * 240 wysokości * 2 bajty na piksel (RGB565)
  size_t rozmiarObrazu = 320 * 240 * 2;

  // 2. Alokujemy bufor w pamięci OPI PSRAM
  uint16_t* buforPSRAM = (uint16_t*)ps_malloc(rozmiarObrazu);

  if (buforPSRAM == NULL) {
    return;
  }

  // 3. Odczytujemy cały ekran linia po linii bezpośrednio do PSRAM
  // Przesuwamy wskaźnik w pamięci o 320 pikseli z każdą linią
  for (int y = 0; y < 240; y++) {
    tft.readRect(0, y, 320, 1, buforPSRAM + (y * 320));
  }

  // 4. Zarządzanie plikiem na karcie SD
  if (SD.exists(sciezka.c_str())) {
    SD.remove(sciezka.c_str());
  }

  File plik1 = SD.open(sciezka.c_str(), FILE_WRITE);
  if (!plik1) {
    free(buforPSRAM);  // PAMIĘTAJ: musimy zwolnić PSRAM, jeśli otwieranie pliku się nie udało
    return;
  }

  // 5. Zapisujemy CAŁY bufor z PSRAM na kartę SD jednym wywołaniem
  size_t zapisaneBajty = plik1.write((uint8_t*)buforPSRAM, rozmiarObrazu);

  if (zapisaneBajty == rozmiarObrazu) {
  } else {
  }

  // 6. Sprzątanie
  plik1.close();
  free(buforPSRAM);  // Zwalniamy pamięć w PSRAM, żeby była dostępna dla innych funkcji
}

// Funkcja odczytu z karty SD i rysowania na ekranie
void wczytajZeSlotuSD(int slot) {
  String sciezka = "/s" + String(slot) + ".raw";

  if (!SD.exists(sciezka)) {
    tft.fillScreen(kolor);
    tft.fillRect(0, 0, 16, 16, kolor);
    return;
  }

  File plik = SD.open(sciezka, FILE_READ);
  if (!plik) {
    tft.fillScreen(kolor);
    tft.fillRect(0, 0, 16, 16, kolor);
    return;
  }

  uint16_t linia[320];

  for (int y = 0; y < 240; y++) {
    plik.read((uint8_t*)linia, 320 * 2);
    tft.pushImage(0, y, 320, 1, linia);
  }

  plik.close();
}

//-- EXTERNAL STORAGE (SD) END --

uint16_t hsv2rgb565(uint16_t h, uint8_t s, uint8_t v) {
  uint8_t r, g, b;
  uint8_t base;

  if (s == 0) {
    r = v;
    g = v;
    b = v;
  } else {
    base = ((255 - s) * v) >> 8;
    switch (h / 60) {
      case 0:
        r = v;
        g = base + (((v - base) * (h % 60)) / 60);
        b = base;
        break;
      case 1:
        r = v - (((v - base) * (h % 60)) / 60);
        g = v;
        b = base;
        break;
      case 2:
        r = base;
        g = v;
        b = base + (((v - base) * (h % 60)) / 60);
        break;
      case 3:
        r = base;
        g = v - (((v - base) * (h % 60)) / 60);
        b = v;
        break;
      case 4:
        r = base + (((v - base) * (h % 60)) / 60);
        g = base;
        b = v;
        break;
      default:
        r = v;
        g = base;
        b = v - (((v - base) * (h % 60)) / 60);
        break;
    }
  }
  return tft.color565(r, g, b);
}

//not used

/*
int policzPlikiWFolderze(fs::FS& fs, const char* sciezka) {
  int liczbaPlikow = 0;

  // Otwieramy wskazany katalog
  File root = fs.open(sciezka);
  if (!root) {
    return 0;
  }
  if (!root.isDirectory()) {
    return 0;
  }

  // Przeszukujemy katalog plik po pliku
  File plik = root.openNextFile();
  while (plik) {
    // Sprawdzamy, czy to zwykły plik (a nie podfolder)
    if (!plik.isDirectory()) {
      liczbaPlikow++;
    }
    plik.close();                // Zamykamy plik, aby zwolnić pamięć RAM
    plik = root.openNextFile();  // Pobieramy kolejny plik
  }

  return liczbaPlikow;
}


void kopiujSlotNaSD(int slot) {
  // 1. Sprawdzenie poprawności numeru slotu
  if (slot < 1 || slot > 5) {
    return;
  }

  // 2. Budowanie ścieżki źródłowej (LittleFS) i docelowej (SD)
  String sciezkaZrodlo = "/slot" + String(slot) + ".raw";
  String sciezkaDocelowa = "/s" + String(slot) + ".raw";

  // 3. Sprawdzenie czy plik istnieje w LittleFS
  if (!LittleFS.exists(sciezkaZrodlo)) {
    tft.fillScreen(kolor);
    tft.fillRect(0, 0, 16, 16, kolor);
    return;
  }

  // 4. Otwarcie pliku źródłowego z LittleFS
  File plikZrodlo = LittleFS.open(sciezkaZrodlo, "r");  // "r" oznacza FILE_READ
  if (!plikZrodlo) {
    return;
  }

  // 5. Otwarcie (lub utworzenie) pliku docelowego na karcie SD
  // FILE_WRITE w SD.h domyślnie dopisuje na końcu,
  // dlatego jeśli plik istniał, warto go najpierw usunąć, aby nadpisać go od nowa.
  if (SD.exists(sciezkaDocelowa)) {
    SD.remove(sciezkaDocelowa);
  }

  File plikDocelowy = SD.open(sciezkaDocelowa, FILE_WRITE);
  if (!plikDocelowy) {
    plikZrodlo.close();
    return;
  }

  // 6. Kopiowanie zawartości za pomocą bufora (wydajne rozwiązanie)
  uint8_t bufor[64];  // Bufor 64-bajtowy (możesz zwiększyć np. do 128 lub 256, jeśli masz RAM)
  while (plikZrodlo.available()) {
    int bajtyPrzeczytane = plikZrodlo.read(bufor, sizeof(bufor));
    plikDocelowy.write(bufor, bajtyPrzeczytane);
  }

  // 7. Zamknięcie obu plików
  plikDocelowy.close();
  plikZrodlo.close();
}

void odtworzVideo(int slot) {
  char sciezkaFolderu[16];
  sprintf(sciezkaFolderu, "/v%d", slot);
  int liczbaKlatek = policzPlikiWFolderze(SD, sciezkaFolderu);

  char adresKlatki[32];  // Statyczny bufor na tekst, brak problemów z RAMem

  for (int i = 1; i <= liczbaKlatek; i++) {
    unsigned long czasWys = millis();

    // Bezpieczne i szybkie formatowanie ścieżki (odpowiednik Twojego Stringa)
    sprintf(adresKlatki, "/v%d/%d.jpg", slot, i);

    // Próba narysowania klatki
    TJpgDec.drawSdJpg(0, 0, adresKlatki);

    unsigned long czasDekodowania = millis() - czasWys;

    if (czasDekodowania < 5) {
      delay(5 - czasDekodowania);
    }

    // KLUCZOWE: Karmienie Watchdoga, zapobiega resetom SW_CPU_RESET
    yield();
  }
}
*/