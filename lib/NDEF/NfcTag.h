#ifndef NfcTag_h
#define NfcTag_h

#include <inttypes.h>
#include <Arduino.h>
#include <NdefMessage.h>

class NfcTag
{
    public:
        NfcTag();
        NfcTag(byte *uid, unsigned int uidLength);
        NfcTag(byte *uid, unsigned int uidLength, String tagType);
        NfcTag(byte *uid, unsigned int uidLength, String tagType, NdefMessage& ndefMessage);
        NfcTag(byte *uid, unsigned int uidLength, String tagType, const byte *ndefData, const int ndefDataLength);
        ~NfcTag(void);
        NfcTag& operator=(const NfcTag& rhs);
        uint8_t getUidLength();
        void getUid(byte *uid, unsigned int uidLength);
        String getUidString();
        String getTagType();
        boolean hasNdefMessage();
        NdefMessage getNdefMessage();
#ifdef NDEF_USE_SERIAL
        void print();
#endif
    private:
        byte *_uid;
        unsigned int _uidLength;
        String _tagType; // Mifare Classic, NFC Forum Type {1,2,3,4}, Unknown
        NdefMessage* _ndefMessage;
        // TODO capacity
        // TODO isFormatted
};

#endif