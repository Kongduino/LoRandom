#ifndef LORANDOM_H
#define LORANDOM_H

#include <Arduino.h>

/*
  Semtech AN1200.24 "SX1276 Settings for LoRaWAN", chapter 4: Random Number Generation for Cryptography.

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
  The RegRssiValue register (at address 0x1b) should not be used for random number generation: with a constant CW
  input inside the receiver channel, its LSB may be constant or strongly biased.
*/

#define RegOpMode 0x01
#define RegModemConfig1 0x1D
#define RegModemConfig2 0x1E
#define RegRssiWideband 0x2C

void writeRegister(uint8_t reg, uint8_t value);
uint8_t readRegister(uint8_t reg);
// Provide your own functions, which will depend on your library

// Everything below is static inline, so this header can be included from several source files
// without duplicate definitions at link time.

// Settings saved by setupLoRandom() and restored by resetLoRa().
static inline uint8_t *loRandomSavedSettings() {
  static uint8_t saved[3];
  return saved;
}

static inline void setupLoRandom() {
  // First save current settings
  uint8_t *saved = loRandomSavedSettings();
  saved[0] = readRegister(RegOpMode);
  delay(10);
  saved[1] = readRegister(RegModemConfig1);
  delay(10);
  saved[2] = readRegister(RegModemConfig2);
  delay(10);

  // 1: LoRa mode
  // 0: Access LoRa registers page 0x0D: 0x3F
  // 00: reserved
  // x: Low Frequency Mode, kept as the radio library set it (on for 433 MHz, off for 868/915 MHz)
  // 101: Receive continuous (RXCONTINUOUS)
  writeRegister(RegOpMode, 0b10000101 | (saved[0] & 0b00001000));
  delay(10);

  // 0111: BW 125
  // 001: CR 4/5
  // 0: Explicit Header mode
  writeRegister(RegModemConfig1, 0b01110010);
  delay(10);
  
  // 0111: SF 7, as in the table above (Semtech AN1200.24, chapter 4)
  // 0: TxContinuousMode off (a transmit setting, not part of the procedure)
  // 0: disable CRC check on payload
  // 00: RX Time-Out MSB
  writeRegister(RegModemConfig2, 0b01110000);
  delay(10);
}

static inline void resetLoRa() {
  // reset settings
  uint8_t *saved = loRandomSavedSettings();
  writeRegister(RegOpMode, saved[0]);
  delay(10);
  writeRegister(RegModemConfig1, saved[1]);
  delay(10);
  writeRegister(RegModemConfig2, saved[2]);
  delay(10);
}

static inline uint8_t getLoRandomByte() {
  uint8_t x = 0, b;
  for (uint8_t j = 0; j < 8; j++) {
    b = readRegister(RegRssiWideband) & 0b00000001;
    while (b == (readRegister(RegRssiWideband) & 0b00000001)) {
      // von Neumann extractor: a pair of equal bits is discarded.
      b = readRegister(RegRssiWideband) & 0b00000001;
    }
    x = (x << 1) | b;
    //delay(1);
  }
  return x;
}

static inline void fillRandomMultiple(unsigned char *x, size_t len, uint8_t multiple) {
  // Fill up a buffer with multiple instances of the same random byte.
  // Yes, I have a specific use for this.
  if (multiple == 0) multiple = 1;
  setupLoRandom();
  size_t i;
  for (i = 0; i < len; i += multiple) {
    uint8_t rng =  getLoRandomByte();
    for (uint8_t j = 0; j < multiple && i + j < len; j++) {
      x[i + j] = rng;
    }
  }
  resetLoRa();
}

static inline void fillRandom(unsigned char *x, size_t len) {
  setupLoRandom();
  size_t i;
  for (i = 0; i < len; i++) {
    x[i] = getLoRandomByte();
  }
  resetLoRa();
}

static inline void fillRandom(unsigned char *x, size_t len, uint8_t except) {
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

static inline void fillRandom(unsigned char *x, size_t len, uint8_t minValue, uint8_t maxValue) {
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

#ifndef KONGDUINO_HEXDUMP
#define KONGDUINO_HEXDUMP
// Shared with Sx1262LoRandom: whichever header comes first defines it.
static inline void hexDump(unsigned char *buf, uint16_t len) {
  char alphabet[17]="0123456789abcdef";
  Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
  Serial.print(F("   |.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .a .b .c .d .e .f | |      ASCII     |\n"));
  for (uint16_t i = 0; i < len; i += 16) {
    if (i % 128 == 0)
      Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
    char s[]="|                                                | |                |\n";
    uint8_t ix = 1, iy = 52;
    for (uint8_t j = 0; j < 16; j++) {
      if (i + j < len) {
        uint8_t c = buf[i + j];
        s[ix++] = alphabet[(c >> 4) & 0x0F];
        s[ix++] = alphabet[c & 0x0F];
        ix++;
        if (c > 31 && c < 128) s[iy++] = c;
        else s[iy++] = '.';
      }
    }
    uint8_t index = i / 16;
    if(i<256) Serial.write(' ');
    Serial.print(index, HEX); Serial.write('.');
    Serial.print(s);
  }
  Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
}
#endif // KONGDUINO_HEXDUMP

#endif // LORANDOM_H
