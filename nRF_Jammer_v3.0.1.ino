// ======================= INCLUDES =======================
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>
#include "RF24.h"
#include "esp_bt.h"
#include "esp_wifi.h"
#include "icon.h"

// ======================= DEFINES ========================
#define initScreen   1   // boot text on/off 0=off 1=on
#define initLogo     1   // boot logo on/off 0=off 1=on
#define BatProcent   0   // 1 = battery percent, 0 = voltage
#define maxVoltage   4.2 // max battery voltage
#define minVoltage   3.3 // minimal batery voltage
#define DataRate     0   // 0=250KBPS 1=1MBPS 2=2MBPS

// ======================= BUTTON PINS ====================
#define BTN_LEFT   33
#define BTN_OK      5
#define BTN_RIGHT  32

//====================== i2c esp =========================
#define SLAVE_ADDR 0x08
bool slavePresent = false;

// ======================= GLOBAL FLAGS ===================
bool nRF1 = false;
bool nRF2 = false;
bool nRF3 = false;
bool nRF4 = false;
bool jammersActive = false;

// ======================= ADC ============================
int   adcPin   = 34;
int   adcValue = 0;
float Vin      = 0.0;

// ======================= UI / MODE ======================
int mode = 1;

// ======================= DISPLAY ========================
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

// ======================= CHANNEL LISTS ==================
byte bluetooth_even_channels[] = {
  2, 4, 6, 8, 10, 12, 14, 16, 18, 20,
  22, 24, 26, 28, 30, 32, 34, 36, 38, 40,
  42, 44, 46, 48, 50, 52, 54, 56, 58, 60,
  62, 64, 66, 68, 70, 72, 74, 76, 78, 80
};

byte bluetooth_odd_channels[] = {
  1, 3, 5, 7, 9, 11, 13, 15, 17, 19,
  21, 23, 25, 27, 29, 31, 33, 35, 37, 39,
  41, 43, 45, 47, 49, 51, 53, 55, 57, 59,
  61, 63, 65, 67, 69, 71, 73, 75, 77, 79
};

byte wifi_channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

byte ble_channels[]   = {1, 2, 3, 25, 26, 27, 79, 80, 81};
byte usb_channels[]   = {40, 50, 60};
byte video_channels[] = {70, 75, 80};
byte rc_channels[]    = {1, 3, 5, 7};

byte full_channels[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
  51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
  71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
  81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
  91, 92, 93, 94, 95, 96, 97, 98, 99, 100
};

byte ch[4] = {45, 45, 45, 45};

// ======================= COUNTS =========================
const int num_bluetooth_even = sizeof(bluetooth_even_channels) / sizeof(byte);
const int num_bluetooth_odd  = sizeof(bluetooth_odd_channels)  / sizeof(byte);
const int num_wifi           = sizeof(wifi_channels)           / sizeof(byte);
const int num_ble            = sizeof(ble_channels)            / sizeof(byte);
const int num_usb            = sizeof(usb_channels)            / sizeof(byte);
const int num_video          = sizeof(video_channels)          / sizeof(byte);
const int num_rc             = sizeof(rc_channels)             / sizeof(byte);
const int num_full           = sizeof(full_channels)           / sizeof(byte);

// ======================= NRF MODULES ====================
RF24 radio_hspi1(26, 15);
RF24 radio_hspi2(27, 25);
RF24 radio_vspi1(4, 2);
RF24 radio_vspi2(16, 17);

// ======================= SETUP ==========================
void setup() {
  Serial.begin(115200);
  
  Wire.begin(21, 22);

  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  analogSetAttenuation(ADC_11db);

  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);

  if (initLogo) {
    u8g2.clearBuffer();
    u8g2.drawXBMP(56, 24, 16, 16, icon_play);
    u8g2.sendBuffer();
    delay(300);
  }

  if (initScreen) {
    u8g2.clearBuffer();
    u8g2.drawHLine(0, 15, 128);

    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(2, 12, "V3.0.1");

    u8g2.setFont(u8g2_font_t0_14b_tr);
    u8g2.drawStr(27, 30, "nRF_Jammer");

    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(40, 41, "By EvilESP");

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(103, 11, "100%");

    u8g2.drawStr(2, 63, "Status: off");
    u8g2.sendBuffer();
  }
  Wire.setTimeOut(5);

  Wire.beginTransmission(SLAVE_ADDR);
  slavePresent = (Wire.endTransmission() == 0);

  delay(600);
  drawUI();
}
//=========== esp to esp comunication ===================

unsigned long lastSlaveCheck = 0;          // ostatni check
const unsigned long slaveCheckInterval = 1000;

void sendCmd(char type, int value) {
  if (!slavePresent) return;

  char buf[6]; // "M:7\0"
  snprintf(buf, sizeof(buf), "%c:%d", type, value);

  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write((uint8_t*)buf, strlen(buf));
  Wire.endTransmission();
}
//======================= init slave ======================
void checkSlaveNonBlocking() {
  if (jammersActive) return;

  if (millis() - lastSlaveCheck < slaveCheckInterval) return;
  lastSlaveCheck = millis();

  Wire.beginTransmission(SLAVE_ADDR);
  slavePresent = (Wire.endTransmission() == 0);
}

// ======================= UI DRAW ========================
void drawUI() {
  u8g2.clearBuffer();

  const char* modeNames[] = {
    "Bluetooth",
    "WIFI ",
    "BLE",
    "USB",
    "VIDEO",
    "RC",
    "FULL"
  };

  // -------- Battery ADC --------
  adcValue = analogRead(adcPin);
  float Vout = adcValue * (3.3 / 4095.0);
  Vin = Vout * 2.26666;

  // -------- Status --------
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2, 63, jammersActive ? "Status: ON" : "Status: Off");

  //---------- External Esp module -------------
if (slavePresent) {
u8g2.drawStr(51, 63, "esp ext");
} 
  // -------- Battery display --------
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setCursor(103, 11);

  if (BatProcent == 0) {
    u8g2.print(Vin, 2);
    u8g2.print("V");
  } else {
    int procent = (Vin - minVoltage) / (maxVoltage - minVoltage) * 100.0;
    procent = constrain(procent, 0, 100);
    u8g2.print(procent);
    u8g2.print("%");
  }

  // -------- Mode name centered --------
  u8g2.setFont(u8g2_font_t0_18b_tr);
  int w = u8g2.getStrWidth(modeNames[mode - 1]);
  int x = (u8g2.getDisplayWidth() - w) / 2;
  u8g2.drawStr(x, 40, modeNames[mode - 1]);

  // -------- Header --------

u8g2.setFontMode(1);
u8g2.setBitmapMode(1);
u8g2.drawHLine(-1, 15, 128);

u8g2.setFont(u8g2_font_6x12_tr);
u8g2.drawStr(2, 12, "V3.0.1");

u8g2.setFont(u8g2_font_4x6_tr);
u8g2.drawStr(86, 63, "By EvilESP");

u8g2.drawLine(84, 55, 126, 55);

u8g2.drawLine(81, 56, 81, 63);

u8g2.drawLine(47, 14, 47, 0);

u8g2.drawLine(85, 55, 0, 55);

u8g2.drawLine(87, 14, 87, 1);

u8g2.drawLine(48, 56, 48, 63);

u8g2.drawLine(124, 35, 118, 41);

u8g2.drawLine(124, 35, 118, 29);

u8g2.drawLine(3, 35, 9, 29);

u8g2.drawLine(3, 35, 9, 41);

u8g2.sendBuffer();

  // -------- NRF status dots --------
  if (nRF1) u8g2.drawEllipse(52, 8, 3, 3);
  if (nRF3) u8g2.drawEllipse(62, 8, 3, 3);
  if (nRF4) u8g2.drawEllipse(72, 8, 3, 3);
  if (nRF2) u8g2.drawEllipse(82, 8, 3, 3);

  u8g2.sendBuffer();
}
// ======================= BUTTON HANDLING =================
unsigned long lastBtnTime[3] = {0, 0, 0};
const unsigned long debounceTime = 200;

bool btnPressed(int pin, int idx) {
  if (digitalRead(pin) == LOW) {
    if (millis() - lastBtnTime[idx] > debounceTime) {
      lastBtnTime[idx] = millis();
      return true;
    }
  }
  return false;
}

// ======================= MAIN LOOP ======================
void loop() {
  checkSlaveNonBlocking();

  // -------- LEFT BUTTON --------
  if (btnPressed(BTN_LEFT, 0)) {
    if (mode > 1) mode--;
    sendCmd('M', mode);
    drawUI();
  }

  // -------- RIGHT BUTTON --------
  if (btnPressed(BTN_RIGHT, 1)) {
    if (mode < 7) mode++;
    sendCmd('M', mode);
    drawUI();
  }

  // -------- OK BUTTON --------
  if (btnPressed(BTN_OK, 2)) {
    jammersActive = !jammersActive;

    if (jammersActive) {
      initializeJammers();
     sendCmd('S', 1);
    } else {
      deactivateJammers();
    sendCmd('S', 0);
    }

    drawUI();
  }

  // -------- RUN MODE --------
  if (jammersActive) {
    runCurrentMode();
  }
}

// ======================= JAMMER INIT =====================
void initializeJammers() {

  esp_bt_controller_deinit();
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_wifi_disconnect();

  Serial.println("initialize_nRF");

  // -------- HSPI --------
  SPIClass *hspi = new SPIClass(HSPI);
  hspi->begin(14, 12, 13, -1);

  if (radio_hspi1.begin(hspi)) {
    nRF1 = true;
    configureRadio(radio_hspi1, ch[0]);
  } else {
    nRF1 = false;
  }

  if (radio_hspi2.begin(hspi)) {
    nRF2 = true;
    configureRadio(radio_hspi2, ch[1]);
  } else {
    nRF2 = false;
  }

  // -------- VSPI --------
  SPIClass *vspi = new SPIClass(VSPI);
  vspi->begin(18, 19, 23, -1);

  if (radio_vspi1.begin(vspi)) {
    nRF3 = true;
    configureRadio(radio_vspi1, ch[2]);
  } else {
    nRF3 = false;
  }

  if (radio_vspi2.begin(vspi)) {
    nRF4 = true;
    configureRadio(radio_vspi2, ch[3]);
  } else {
    nRF4 = false;
  }
}

// ======================= JAMMER OFF ======================
void deactivateJammers() {

  radio_hspi1.stopConstCarrier();
  radio_hspi2.stopConstCarrier();
  radio_vspi1.stopConstCarrier();
  radio_vspi2.stopConstCarrier();

  Serial.println("deActive");

  nRF1 = false;
  nRF2 = false;
  nRF3 = false;
  nRF4 = false;
}

// ======================= MODE HANDLER ====================
void runCurrentMode() {

  byte newCh[4];

  switch (mode) {

    case 1: // Bluetooth
      newCh[0] = bluetooth_odd_channels[random(num_bluetooth_odd)];
      newCh[1] = bluetooth_even_channels[random(num_bluetooth_even)];
      newCh[2] = bluetooth_odd_channels[random(num_bluetooth_odd)];
      newCh[3] = bluetooth_even_channels[random(num_bluetooth_even)];
      break;

    case 2: // WiFi
      for (int i = 0; i < 4; i++)
        newCh[i] = wifi_channels[random(num_wifi)];
      break;

    case 3: // BLE
      for (int i = 0; i < 4; i++)
        newCh[i] = ble_channels[random(num_ble)];
      break;

    case 4: // USB
      for (int i = 0; i < 4; i++)
        newCh[i] = usb_channels[random(num_usb)];
      break;

    case 5: // Video
      for (int i = 0; i < 4; i++)
        newCh[i] = video_channels[random(num_video)];
      break;

    case 6: // RC
      for (int i = 0; i < 4; i++)
        newCh[i] = rc_channels[random(num_rc)];
      break;

    case 7: // Full
      for (int i = 0; i < 4; i++)
        newCh[i] = full_channels[random(num_full)];
      break;
  }

  updateChannels(newCh);
  delayMicroseconds(random(50));
}

// ======================= CHANNEL UPDATE ==================
void updateChannels(byte newCh[4]) {

  if (newCh[0] != ch[0]) {
    radio_hspi1.setChannel(newCh[0]);
    ch[0] = newCh[0];
  }

  if (newCh[1] != ch[1]) {
    radio_hspi2.setChannel(newCh[1]);
    ch[1] = newCh[1];
  }

  if (newCh[2] != ch[2]) {
    radio_vspi1.setChannel(newCh[2]);
    ch[2] = newCh[2];
  }

  if (newCh[3] != ch[3]) {
    radio_vspi2.setChannel(newCh[3]);
    ch[3] = newCh[3];
  }
}

// ======================= RADIO CONFIG ====================
void configureRadio(RF24& radio, byte channel) {

  radio.setAutoAck(false);
  radio.stopListening();
  radio.setRetries(0, 0);
  radio.setPALevel(RF24_PA_MAX, true);

  if (DataRate == 0) {
    radio.setDataRate(RF24_250KBPS);
  } else if (DataRate == 1) {
    radio.setDataRate(RF24_1MBPS);
  } else if (DataRate == 2) {
    radio.setDataRate(RF24_2MBPS);
  }

  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.startConstCarrier(RF24_PA_MAX, channel);
}
