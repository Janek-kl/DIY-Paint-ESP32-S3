//zamieniamy pliki w folderze biblioteki TFT_eSPI
#define ILI9341_DRIVER    // Twój sterownik
#define USE_HSPI_PORT
#define TFT_RGB_ORDER TFT_BGR

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define LOAD_GLCD
#define LOAD_GFXFF
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_ROBOTO

// SPI pins
/* //ESP 32
#define TFT_MISO 19       // Standardowy pin MISO dla ESP32 (potrzebny do dotyku!)
#define TFT_MOSI 23       // Standardowy pin MOSI dla ESP32
#define TFT_SCLK 18       // Standardowy pin SCLK (Zegar) dla ESP32
#define TFT_CS   5        // Pin CS ekranu
#define TFT_DC   2        // Bezpieczny pin danych/komend (zamiast 16)
#define TFT_RST  4        // Bezpieczny pin Reset ekranu (zamiast 17)
*/ 

//ESP32 S3

#define TFT_MISO   13    // Wspólny MISO dla ekranu i dotyku
#define TFT_MOSI   11    // Wspólny MOSI
#define TFT_SCLK   12    // Wspólny Zegar (SCLK)
#define TFT_CS     10    // Chip Select ekranu
#define TFT_DC      9    // Data/Command
#define TFT_RST    14    // Reset ekranu

// Touch Screen (Ważne dla TFT_eSPI)

// ESP32 #define TOUCH_CS 33       // Pin CS dla dotyku XPT2046

#define TOUCH_CS    15    // Chip Select dla dotyku

// Backlight (Podświetlenie)
// UWAGA: Nie wpisujemy tutaj "3.3V". Jeśli podłączasz podświetlenie na stałe 
// do linii zasilania 3.3V, po prostu zakomentuj linię TFT_BL.
// Jeśli używasz tranzystora do sterowania, wpisz tu numer pinu (np. 14).
//#define TFT_BL 14 
#define TFT_BACKLIGHT_ON HIGH
//#define SUPPORT_TRANSACTIONS

// SPI frequencies
#define SPI_FREQUENCY       40000000  // Przyspieszamy ekran do stabilnych 40 MHz
#define SPI_READ_FREQUENCY  20000000  // Częstotliwość odczytu z ekranu (opcjonalna)
#define SPI_TOUCH_FREQUENCY  2000000  // Zwalniamy dotyk XPT do bezpiecznych 2 MHz