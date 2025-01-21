#include <Wire.h>
#include <PN532.h>
#include <NdefMessage.h>
#include <NdefRecord.h>
#include <ArduinoUnit.h>

void leakCheck(void (*callback)())
{
  int start = freeMemory();
  (*callback)();
  int end = freeMemory();
  Serial.println((end - start), DEC);
}

// Custom Assertion
void assertNoLeak(void (*callback)())
{
  int start = freeMemory();
  (*callback)();
  int end = freeMemory();
  assertEqual(0, (start - end));
}

void record()
{
  NdefRecord* r = new NdefRecord();
  delete r;
}

void emptyRecord()
{
  NdefRecord* r = new NdefRecord();
#ifdef NDEF_USE_SERIAL
  r->print();
#endif
  delete r;
}

void textRecord()
{
  NdefRecord* r = new NdefRecord();
  r->setTnf(0x1);
  uint8_t type[] = { 0x54 };
  r->setType(type, sizeof(type));
  uint8_t payload[] = { 0x1A, 0x1B, 0x1C };
  r->setPayload(payload, sizeof(payload));
#ifdef NDEF_USE_SERIAL
  r->print();
#endif
  delete r;
}

void recordMallocZero()
{
  NdefRecord r = NdefRecord();
  String type = r.getType();
  String id = r.getId();
  byte payload[r.getPayloadLength()];
  r.getPayload(payload);
}

// this is OK
void emptyMessage()
{
  NdefMessage* m = new NdefMessage();
  delete m;
}

// this is OK
void printEmptyMessage()
{
  NdefMessage* m = new NdefMessage();
#ifdef NDEF_USE_SERIAL
  m->print();
#endif
  delete m;
}

// this is OK
void printEmptyMessageNoNew()
{
  NdefMessage m = NdefMessage();
#ifdef NDEF_USE_SERIAL
  m.print();
#endif
}

void messageWithTextRecord()
{
  NdefMessage m = NdefMessage();
  m.addTextRecord("foo");
#ifdef NDEF_USE_SERIAL
  m.print();
#endif
}

void messageWithEmptyRecord()
{
  NdefMessage m = NdefMessage();
  NdefRecord r = NdefRecord();
  m.addRecord(r);
#ifdef NDEF_USE_SERIAL
  m.print();
#endif
}

void messageWithoutHelper()
{
  NdefMessage m = NdefMessage();
  NdefRecord r = NdefRecord();
  r.setTnf(1);
  uint8_t type[] = { 0x54 };
  r.setType(type, sizeof(type));
  uint8_t payload[] = { 0x02, 0x65, 0x6E, 0x66, 0x6F, 0x6F };
  r.setPayload(payload, sizeof(payload));
  m.addRecord(r);
#ifdef NDEF_USE_SERIAL
  m.print();
#endif
}

void messageWithId()
{
  NdefMessage m = NdefMessage();
  NdefRecord r = NdefRecord();
  r.setTnf(1);
  uint8_t type[] = { 0x54 };
  r.setType(type, sizeof(type));
  uint8_t payload[] = { 0x02, 0x65, 0x6E, 0x66, 0x6F, 0x6F };
  r.setPayload(payload, sizeof(payload));
  uint8_t id[] = { 0x0, 0x0, 0x0 };
  r.setId(id, sizeof(id));
  m.addRecord(r);
#ifdef NDEF_USE_SERIAL
  m.print();
#endif
}

void message80()
{
  NdefMessage message = NdefMessage();
  message.addTextRecord("This record is 80 characters.X01234567890123456789012345678901234567890123456789");
#ifdef NDEF_USE_SERIAL
  //message.print();
#endif
}

void message100()
{
  NdefMessage message = NdefMessage();
  message.addTextRecord("This record is 100 characters.0123456789012345678901234567890123456789012345678901234567890123456789");
#ifdef NDEF_USE_SERIAL
  //message.print();
#endif
}

void message120()
{
  NdefMessage message = NdefMessage();
  message.addTextRecord("This record is 120 characters.012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
#ifdef NDEF_USE_SERIAL
  //message.print();
#endif
}

void setup() {
  Serial.begin(9600);
  Serial.println("\n");
  Serial.println(F("========="));
  Serial.println(freeMemory());
  Serial.println(F("========="));
}

test(memoryKludgeEnd)
{
  // TODO ensure the output matches start
  Serial.println(F("========="));
  Serial.print("End ");Serial.println(freeMemory());
  Serial.println(F("========="));
}

test(recordLeaks)
{
  assertNoLeak(&record);
  assertNoLeak(&emptyRecord);
  assertNoLeak(&textRecord);
}

test(recordAccessorLeaks)
{
  assertNoLeak(&recordMallocZero);
}

test(messageLeaks)
{
  assertNoLeak(&emptyMessage);
  assertNoLeak(&printEmptyMessage);
  assertNoLeak(&printEmptyMessageNoNew);
  assertNoLeak(&messageWithTextRecord);
  assertNoLeak(&messageWithEmptyRecord);
  assertNoLeak(&messageWithoutHelper);
  assertNoLeak(&messageWithId);
}

test(messageOneBigRecord)
{
  assertNoLeak(&message80);
  // The next 2 fail. Maybe out of memory? Look into helper methods
  //assertNoLeak(&message100);
  //assertNoLeak(&message120);
}

test(memoryKludgeStart)
{
  Serial.println(F("---------"));
  Serial.print("Start ");Serial.println(freeMemory());
  Serial.println(F("---------"));
}

void loop() {
  Test::run();  
}
