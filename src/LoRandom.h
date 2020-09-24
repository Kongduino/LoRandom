/*
  SX1276 Register (address)     Register bit field (bit #)      Values    Note
  RegOpMode (0x01)              LongRangeMode[7]                ‘1’       LoRa mode enable
                                Mode[2:0]                       ‘101’     Receive Continuous mode
  ------------------------------------------------------------------------------------------------------------------
  RegModemConfig1 (0x1D)        Bw[7:4]                         ‘0111’    ‘0111’ for 125kHz modulation Bandwidth
                                CodingRate[3:1]                 ‘001’     4/5 error coding rate
                                ImplicitHeaderModeOn[0]         ‘0’       Packets have up-front header
  ------------------------------------------------------------------------------------------------------------------
  RegModemConfig2 (0x1E)        SpreadingFactor[7:4]            ‘0111’    ‘0111’ (SF7) = 6kbit/s

  To generate an N bit random number, perform N read operation of the register RegRssiWideband (address 0x2c)
  and use the LSB of the fetched value. The value from RegRssiWideband is derived from a wideband (4MHz) signal strength
  at the receiver input and the LSB of this value constantly and randomly changes.
*/

#define RegOpMode 0x01
#define RegModemConfig1 0x1D
#define RegModemConfig2 0x1E
#define RegRssiWideband 0x2C

void writeRegister(uint8_t reg, uint8_t value);
uint8_t readRegister(uint8_t reg);
// Provide your own functions, which will depend on your library

uint8_t modemconf1;
uint8_t modemconf2;

void setupLoRandom() {
  modemconf1 = readRegister(RegModemConfig1);
  modemconf2 = readRegister(RegModemConfig2);
  writeRegister(RegOpMode, 0b10001101);
  // MODE_LONG_RANGE_MODE 0b1xxxxxxx || LowFrequencyModeOn 0bxxxx1xxx || MODE_RX_CONTINUOUS 0bxxxxx101
  writeRegister(RegModemConfig1, 0b01110010);
  writeRegister(RegModemConfig2, 0b01110000);
}

void resetLoRa() {
  writeRegister(RegOpMode, 0b10000001);
  // MODE_LONG_RANGE_MODE 0b1xxxxxxx || MODE_STDBY 0bxxxxxxx1
  writeRegister(RegModemConfig1, modemconf1);
  writeRegister(RegModemConfig2, modemconf2);
}

uint8_t getLoRandomByte() {
  uint8_t x = 0;
  for (uint8_t j = 0; j < 8; j++) {
    x += (readRegister(RegRssiWideband) & 0b00000001);
    x = x << 1;
    //delay(1);
  }
  return x;
}

void fillRandomMultiple(unsigned char *x, size_t len, uint8_t multiple) {
  // Fill up a buffer with multiple instances of the same random byte.
  // Yes, I have a specific use for this.
  setupLoRandom();
  size_t i;
  for (i = 0; i < len; i += multiple) {
    uint8_t rng =  getLoRandomByte();
    for (uint8_t j = 0; j < multiple; j++) {
      x[i + j] = rng;
    }
  }
  resetLoRa();
}

void fillRandom(unsigned char *x, size_t len) {
  setupLoRandom();
  size_t i;
  for (i = 0; i < len; i++) {
    x[i] = getLoRandomByte();
  }
  resetLoRa();
}

void fillRandom(unsigned char *x, size_t len, uint8_t except) {
  // used, for instance, when you want a non-zero value.
  // fillRandom(UUID, 16, 0);
  // get a UUID with 16 bytes, all non-zero
  setupLoRandom();
  size_t i;
  for (i = 0; i < len; i++) {
    unsigned char c = getLoRandomByte();
    while (c == except)c = getLoRandomByte();
    x[i] = c;
  }
  resetLoRa();
}

void fillRandom(unsigned char *x, size_t len, uint8_t minValue, uint8_t maxValue) {
  // used, for instance, when you want a value within a specified range.
  // fillRandom(x, 16, 0, 15);
  // get a bank of random numbers, between 0 and 15.
  setupLoRandom();
  size_t i;
  for (i = 0; i < len; i++) {
    unsigned char c = getLoRandomByte();
    while (c < minValue || c > maxValue)c = getLoRandomByte();
    x[i] = c;
  }
  resetLoRa();
}

void hexDump(unsigned char *buf, uint16_t len) {
  String s = "|", t = "| |";
  Serial.println(F("  |.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .a .b .c .d .e .f |"));
  Serial.println(F("  +------------------------------------------------+ +----------------+"));
  for (uint16_t i = 0; i < len; i += 16) {
    for (uint8_t j = 0; j < 16; j++) {
      if (i + j >= len) {
        s = s + "   "; t = t + " ";
      } else {
        char c = buf[i + j];
        if (c < 16) s = s + "0";
        s = s + String(c, HEX) + " ";
        if (c < 32 || c > 127) t = t + ".";
        else t = t + (String(c));
      }
    }
    uint8_t index = i / 16;
    Serial.print(index, HEX); Serial.write('.');
    Serial.println(s + t + "|");
    s = "|"; t = "| |";
  }
  Serial.println(F("  +------------------------------------------------+ +----------------+"));
}
