#include <Wire.h>
#include <PN532.h>
#include <NfcTag.h>
#include <ArduinoUnit.h>

void setup() {
    Serial.begin(9600);
}

// Test for pull requests #14 and #16
test(getUid)
{
  byte uid[4] = { 0x00, 0xFF, 0xAA, 0x17 };
  byte uidFromTag[sizeof(uid)]; 

  NfcTag tag = NfcTag(uid, sizeof(uid));

  assertEqual(sizeof(uid), tag.getUidLength());

  tag.getUid(uidFromTag, sizeof(uidFromTag));

  // make sure the 2 uids are the same
  for (int i = 0; i < sizeof(uid); i++) {
    assertEqual(uid[i], uidFromTag[i]);
  }

  // check contents, to ensure the original uid wasn't overwritten
  assertEqual(0x00, uid[0]);
  assertEqual(0xFF, uid[1]);
  assertEqual(0xAA, uid[2]);
  assertEqual(0x17, uid[3]);
}

void loop() {
  Test::run();
}

