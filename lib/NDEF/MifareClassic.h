#ifndef MifareClassic_h
#define MifareClassic_h

// Comment out next line to remove Mifare Classic and save memory
#define NDEF_SUPPORT_MIFARE_CLASSIC

#ifdef NDEF_SUPPORT_MIFARE_CLASSIC

#include <Due.h>
#include <PN532.h>
#include <Ndef.h>
#include <NfcTag.h>

class MifareClassic
{
    public:
        MifareClassic(PN532& nfcShield);
        ~MifareClassic();
        NfcTag read(byte *uid, unsigned int uidLength);
        boolean write(NdefMessage& ndefMessage, byte *uid, unsigned int uidLength);
        boolean formatNDEF(byte * uid, unsigned int uidLength);
        boolean formatMifare(byte * uid, unsigned int uidLength);
    private:
        PN532* _nfcShield;
        int getBufferSize(int messageLength);
        int getNdefStartIndex(byte *data);
        bool decodeTlv(byte *data, int &messageLength, int &messageStartIndex);
};

#endif
#endif
