#include <Arduino.h>
#include <WhandallCRC16.h>

void CRC16::init(uint16_t with, uint16_t poly) {
  crc = with;
	polynom = poly;
}

void CRC16::feed(uint8_t topBit) {
  crc ^= (((uint16_t)topBit) << 8) & 0x8000;
  if (crc & 0x8000) {
    crc <<= 1;
    crc ^= polynom;
  } else {
    crc <<= 1;
  }
}

void CRC16::feed(const uint8_t* ptr, uint8_t len) {
  while (len--) {
    uint8_t bits = *ptr++;
    for (uint8_t i = 0; i < 8; i++, bits <<= 1) {
      feed(bits);
    }
  }
}

void CRC16::feedReverse(const uint8_t* ptr, uint8_t len) {
  ptr += len - 1;
  while (len--) {
    uint8_t bits = *ptr--;
    for (uint8_t i = 0; i < 8; i++, bits <<= 1) {
      feed(bits);
    }
  }
}

uint16_t CRC16::getCRC() {
  return crc;
}