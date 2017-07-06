#include <printf.h>
#include <RF24.h>
#include <WhandallCRC16.h>

const byte adr[] = "M-moc" "J-moc" "2-moc" "3-moc";

const byte radioCEPin = 9;
const byte radioCSNPin = 10;
const byte adrBytes = 5;
const byte packetSize = 32;

const byte numAdrs = min(sizeof(adr) / adrBytes, 6);

RF24 radio(radioCEPin, radioCSNPin);

CRC16 mCRC;

void setup() {
  printf_begin();
  Serial.begin(250000);
  printAddresses(adr, numAdrs);
  radio.begin();
  radio.setChannel(76);
  radio.setDataRate(RF24_2MBPS);
  radio.setAddressWidth(adrBytes);
  radio.setPayloadSize(packetSize);
  radio.setAutoAck(false);
  radio.disableCRC();
  for (byte i = 0; i < 6; i++) {
    if (i < numAdrs) {
      radio.openReadingPipe(i, adr + adrBytes * i);
    } else {
      radio.closeReadingPipe(i);  // cleanup for a restart with different number of pipes
    }
  }
  radio.startListening();
  radio.printDetails();
}

void loop() {
  byte pipe = 0;
  byte rawData[packetSize];
  if (radio.available(&pipe)) {
    unsigned long thisPacket = micros();
    radio.read(rawData, packetSize);
    printPacket(pipe, rawData);
    printDeltaMicros(thisPacket);
    Serial.println();
  }
}

void printAddresses(const byte * table, byte number) {
  Serial.println(F("listen to"));
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

void printPacket(byte pip, byte * ptr) {
  mCRC.init();
  mCRC.feedReverse(adr + adrBytes * pip, adrBytes); // address bytes in reverse order
  mCRC.feed(ptr, 1);                    // 8 bit of control field
  mCRC.feed(ptr[1]);                    // 9th bit of control field
  Serial.write('P');
  Serial.print(pip);
  Serial.write('(');
  pAsc(adr + adrBytes * pip, adrBytes);
  Serial.write(')');
  Serial.write(' ');
  // Coding: 000000 = 0 byte (only used in empty ACK packets.) 100000 = 32 byte, 100001 = Don’t care.
  byte plen = (ptr[0] & 0xFC) >> 2;   // isolate packet len
  pDec2(plen);
  if (plen <= 32) {
    Serial.write('-');
  } else if (plen == 33) {
    Serial.write('x');
  } else {
    plen = 32;
    Serial.write('*');
  }
  Serial.print(ptr[0] & 3);           // isolate packet id
  Serial.write('-');
  Serial.print((ptr[1] & 0x80) >> 7); // isolate noAck bit
  if (plen == 0) {                    // Acknowledge, no Payload
    Serial.print(F(" Ack"));
  } else if (plen == 33) {
    Serial.print(F(" Don't care"));
  } else {
    // because the Packet Control Field is 9 bits,
    // we have to shift the whole buffer (including crc) up one bit
    byte i = 0;
    for (i = 1; i <= plen + 2; i++) {
      ptr[i] <<= 1;
      if (ptr[i + 1] & 0x80) {
        ptr[i]++;
      }
    }
    mCRC.feed(ptr + 1, plen);
    pHex(ptr + 1, plen);
    Serial.print(F(" C("));
    pHex(ptr[plen + 1]);
    pHex(ptr[plen + 2]);
    uint16_t packCRC = (ptr[plen + 1] << 8) | ptr[plen + 2];
    if (packCRC != mCRC.getCRC()) {
      Serial.print(F(" BAD "));
      Serial.print(mCRC.getCRC(), HEX);
    } else {
      Serial.print(F(" ok"));
    }
    Serial.print(") ");
    pAsc(ptr + 1, plen, '"');
    static uint16_t lastCRC;
    if (packCRC == lastCRC) {
      Serial.print(F(" retry"));
    }
    lastCRC = packCRC;
  }
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

