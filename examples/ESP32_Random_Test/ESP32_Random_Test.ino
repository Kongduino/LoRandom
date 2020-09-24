#include <LoRa.h>
#include <LoRandom.h>

#define SS      18
#define RST     14
#define DI0     26
#define BAND 431E6
#define REG_OCP 0x0B
#define REG_PA_CONFIG 0x09
#define REG_LNA 0x0c
#define REG_OP_MODE 0x01
#define REG_MODEM_CONFIG_1 0x1d
#define REG_MODEM_CONFIG_2 0x1e
#define REG_MODEM_CONFIG_3 0x26
#define REG_PA_DAC 0x4D
#define PA_DAC_HIGH 0x87
#define REG_PA_DAC 0x4D
#define MODE_LONG_RANGE_MODE 0x80
#define MODE_SLEEP 0x00
#define MODE_STDBY 0x01
#define MODE_TX 0x03
#define MODE_RX_CONTINUOUS 0x05
#define MODE_RX_SINGLE 0x06

/*
  Note: I have "customized" the LoRa library by moving
  uint8_t readRegister(uint8_t address);
  void writeRegister(uint8_t address, uint8_t value);
  to public: in LoRa.h – as we need access to the registers, obviously.
*/

void writeRegister(uint8_t reg, uint8_t value) {
  LoRa.writeRegister(reg, value);
}
uint8_t readRegister(uint8_t reg) {
  return LoRa.readRegister(reg);
}

unsigned char pkt[256];

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.flush();
  Serial.print(F("\n\n\n[SX1278] Initializing ... "));
  delay(1000);
  SPI.begin(5, 19, 27, 18);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setPreambleLength(8);
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setPreambleLength(8);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(250E3);
  LoRa.setCodingRate4(5);

  // BW = 8: 250 kHz, CR = 1: 4/5, HM = 0
  uint8_t reg1 = 0x82;
  // SF = 12: 12, CRC = 1
  uint8_t reg2 = 0xC4;
  // LDRO = 1, AGCAutoOn = 0
  uint8_t reg3 = 0x08;
  // PaSelect = 1, MaxPower = 7: 15 dBm, OutputPower = 15: 17 dBm
  uint8_t regpaconfig = 0xFF;
  // 7:5 LnaGain 001 -> G1 = maximum gain
  // 1:0 LnaBoostHf 11 -> Boost on, 150% LNA current
  uint8_t reglna = 0b00100011;

  Serial.print("\nSetting up LoRa for Transmission ");
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
  delay(10);
  writeRegister(REG_PA_CONFIG, 0xFF);
  writeRegister(REG_MODEM_CONFIG_1, reg1);
  writeRegister(REG_MODEM_CONFIG_2, reg2);
  writeRegister(REG_MODEM_CONFIG_3, reg3);
  writeRegister(REG_LNA, reglna);
  writeRegister(REG_PA_DAC, REG_PA_DAC); // That's for the receiver
  writeRegister(REG_OCP, 0b00111111);
  delay(10);
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
  delay(10);
  Serial.println("LoRa [" + String(BAND / 1E6) + "]");

  uint8_t x = readRegister(0x01);
  Serial.print("RegOpMode: 0x");
  if (x < 16) Serial.write('0');
  Serial.println(x, HEX);
  x = readRegister(0x1D);
  Serial.print("RegModemConfig1: 0x");
  if (x < 16) Serial.write('0');
  Serial.println(x, HEX);
  x = readRegister(0x1E);
  Serial.print("RegModemConfig2: 0x");
  if (x < 16) Serial.write('0');
  Serial.println(x, HEX);
}

void loop() {
  Serial.println("\n   Generating Random Numbers with LoRandom");
  fillRandom(pkt, 256);
  hexDump(pkt, 256);
  delay(3000);
}
