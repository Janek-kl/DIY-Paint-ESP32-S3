# DIY-Paint-ESP32-S3

Aplikacja do rysowania na wyświetlaczu TFT 2.8" z ESP32-S3.

## Sprzęt

- **ESP32-S3 DevKitC-1 N16R8**
- **Wyświetlacz dotykowy TFT LCD 2,8" 240×320 px** z czytnikiem kart SD
  - Kontroler obrazu: **ILI9341**
  - Kontroler dotyku: **XPT2046**
- **Enkoder obrotowy z przyciskiem**

## Połączenia

### Wyświetlacz TFT

| Sygnał | Pin ESP32-S3 |
|--------|:------------:|
| TFT_MISO | 13 |
| TFT_MOSI | 11 |
| TFT_SCLK | 12 |
| TFT_CS | 10 |
| TFT_DC | 9 |
| TFT_RST | 14 |
| TOUCH_CS | 15 |
| TOUCH_IRQ | 7 |

### Karta SD

| Sygnał | Pin ESP32-S3 |
|--------|:------------:|
| SD_SCLK | 21 |
| SD_MISO | 47 |
| SD_MOSI | 20 |
| SD_CS | 17 |

### Enkoder obrotowy

| Sygnał | Pin ESP32-S3 |
|--------|:------------:|
| ENCODER_SW | 4 |
| ENCODER_CLK | 6 |
| ENCODER_DT | 5 |
| ENCODER_POWER | 16 |