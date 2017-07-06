#include <printf.h>
#include <RF24.h>
#include <WhandallCRC16.h>

byte adr[] = {
  'a', 0x55,
};

const byte radioCEPin = 9;
const byte radioCSNPin = 10;
const byte adrBytes = 2;
const byte packetSize = 32;
const unsigned int scanDuration = 1000;

const byte numAdrs = min(sizeof(adr) / adrBytes, 6);

RF24 radio(radioCEPin, radioCSNPin);

CRC16 mCRC;

void setup() {
  printf_begin();
  Serial.begin(250000);
  radio.begin();
  radio.setChannel(76);
  radio.setDataRate(RF24_2MBPS);
  radio.setAddressWidth(adrBytes);
  radio.setPayloadSize(packetSize);
  radio.setAutoAck(false);
  radio.disableCRC();
  setupAddresses();
  radio.startListening();
  radio.printDetails();
}

void loop() {
  static unsigned long lastListenStart;
  unsigned long topLoop = millis();
  byte pipe = 0;
  byte rawData[packetSize];
  if (radio.available(&pipe)) {
    unsigned long thisPacket = micros();
    radio.read(rawData, packetSize);
    if (printPacket(pipe, rawData)) {
      printDeltaMicros(thisPacket);
      Serial.println();
    }
  }
  if (topLoop - lastListenStart >= scanDuration) {
    radio.stopListening();
    adr[0]++;
    adr[1] = (adr[0] & 0x80 ? 0xAA : 0x55);
    setupAddresses();
    radio.startListening();
    lastListenStart = topLoop;
  }
}

void setupAddresses() {
  for (byte i = 0; i < 6; i++) {
    if (i < numAdrs) {
      radio.openReadingPipe(i, adr + adrBytes * i);
    } else {
      radio.closeReadingPipe(i);  // cleanup for a restart with different number of pipes
    }
  }
  printAddresses(adr, numAdrs);
}

void printAddresses(const byte * table, byte number) {
  Serial.print(F("listen to "));
  if (number != 1) {
    Serial.println();
  }
  for (byte i = 0; i < number; i++) {
    pAsc(table + i * adrBytes, adrBytes, '"');
    pHex(table + i * adrBytes, adrBytes);
    if (i != number - 1) {
      Serial.println(F(" and"));
    }
  }
  Serial.println();
}

unsigned long printDeltaMicros(unsigned long current) {
  static unsigned long lastPacket;
  unsigned long delta = current - lastPacket;
  Serial.print(F(" (+"));
  Serial.print(delta);
  Serial.write(')');
  lastPacket = current;
  return delta;
}

bool printPacket(byte pip, byte * ptr) {
  byte packStart = 1 + 5 + 1 - adrBytes;
  mCRC.init();
  mCRC.feed(adr, 1);      // one byte, upper part of address is preamble
  mCRC.feed(ptr, 4 + 1);  // rest of address and 8 bit of control field
  mCRC.feed(ptr[packStart]);      // 9th bit of control field
  // Coding: 000000 = 0 byte (only used in empty ACK packets.) 100000 = 32 byte, 100001 = Don’t care.
  byte plen = (ptr[packStart - 1] & 0xFC) >> 2; // isolate packet len
  if (plen > 32) {
    return false;
  }
  // because the Packet Control Field is 9 bits,
  // we have to shift the whole buffer (including crc) up one bit
  byte i = 0;
  for (i = packStart; i <= packStart + plen + 2; i++) {
    ptr[i] <<= 1;
    if (ptr[i + 1] & 0x80) {
      ptr[i]++;
    }
  }
  uint16_t packCRC = (ptr[packStart + plen + 0] << 8) | ptr[packStart + plen + 1];
  mCRC.feed(ptr + packStart, plen);
  if (packCRC != mCRC.getCRC()) {
    return false;
  }
  Serial.write('P');
  Serial.print(pip);
  Serial.write('(');
  pAsc(adr + adrBytes * pip, 1);
  pAsc(ptr, 4);
  Serial.write(')');
  Serial.write(' ');
  pDec2(plen);
  Serial.write('-');
  Serial.print(ptr[packStart - 1] & 3);         // isolate packet id
  Serial.write('-');
  Serial.print((ptr[packStart - 1] & 0x80) >> 7); // isolate noAck bit
  //pHex(ptr, packStart);
  pHex(ptr + packStart, plen);
  Serial.print(F(" C("));
  pHex(ptr[packStart + plen + 0]);
  pHex(ptr[packStart + plen + 1]);
  if (packCRC != mCRC.getCRC()) {
    Serial.print(F(" BAD "));
    Serial.print(mCRC.getCRC(), HEX);
  } else {
    Serial.print(F(" ok"));
  }
  Serial.print(") ");
  if (plen) {
    pAsc(ptr + packStart, 4, '"');
    Serial.write(' ');
    pAsc(ptr + packStart + 4, plen - 4, '"');
  } else {
    Serial.print(" Acknowledge");
  }
  static uint16_t lastCRC;
  if (packCRC == lastCRC) {
    Serial.print(F(" retry"));
  }
  lastCRC = packCRC;
  return true;
}

void pAsc(const byte * from, byte len, char dlm) {
  Serial.write(dlm);
  pAsc(from, len);
  Serial.write(dlm);
}

void pAsc(const byte * from, byte len) {
  while (len--) {
    pAsc(*from++);
  }
}

void pAsc(byte val) {
  Serial.write(val >= 0x20 ? (char)val : '.');
}

void pHex(const byte * from, byte len) {
  while (len--) {
    Serial.write(' ');
    pHex(*from++);
  }
}

void pHex(byte val) {
  if (val < 16) {
    Serial.write('0');
  }
  Serial.print(val, HEX);
}

void pDec2(byte val) {
  if (val < 10) {
    Serial.write('0');
  }
  Serial.print(val);
}

