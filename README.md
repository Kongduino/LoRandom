# LoRandom

A library using Semtech's sx1276/7/8/9's `RegRssiWideband` register properly to generate random numbers. See [this issue](https://github.com/sandeepmistry/arduino-LoRa/issues/394) for context.

This was written for Sandeep Mistry's library, but is easily adaptable to other platforms.

See [ESP32_Random_Test](https://github.com/Kongduino/ESP32_Random_Test) for an example of how it works.

## UPDATE 2021/07/22

There was a bug in `getLoRandomByte()`, whereas only even numbers were returned. This bug was pointed out [here](https://github.com/sandeepmistry/arduino-LoRa/pull/496#issuecomment-882019560), for which I am thankful...

