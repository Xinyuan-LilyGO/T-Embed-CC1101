#include <NfcTag.h>

NfcTag::NfcTag()
{
    _uid = 0;
    _uidLength = 0;
    _tagType = "Unknown";
    _ndefMessage = (NdefMessage*)NULL;
}

NfcTag::NfcTag(byte *uid, unsigned int uidLength)
{
    _uid = uid;
    _uidLength = uidLength;
    _tagType = "Unknown";
    _ndefMessage = (NdefMessage*)NULL;
}

NfcTag::NfcTag(byte *uid, unsigned int  uidLength, String tagType)
{
    _uid = uid;
    _uidLength = uidLength;
    _tagType = tagType;
    _ndefMessage = (NdefMessage*)NULL;
}

NfcTag::NfcTag(byte *uid, unsigned int  uidLength, String tagType, NdefMessage& ndefMessage)
{
    _uid = uid;
    _uidLength = uidLength;
    _tagType = tagType;
    _ndefMessage = new NdefMessage(ndefMessage);
}

// I don't like this version, but it will use less memory
NfcTag::NfcTag(byte *uid, unsigned int uidLength, String tagType, const byte *ndefData, const int ndefDataLength)
{
    _uid = uid;
    _uidLength = uidLength;
    _tagType = tagType;
    _ndefMessage = new NdefMessage(ndefData, ndefDataLength);
}

NfcTag::~NfcTag()
{
    delete _ndefMessage;
}

NfcTag& NfcTag::operator=(const NfcTag& rhs)
{
    if (this != &rhs)
    {
        delete _ndefMessage;
        _uid = rhs._uid;
        _uidLength = rhs._uidLength;
        _tagType = rhs._tagType;
        // TODO do I need a copy here?
        _ndefMessage = rhs._ndefMessage;
    }
    return *this;
}

uint8_t NfcTag::getUidLength()
{
    return _uidLength;
}

void NfcTag::getUid(byte *uid, unsigned int uidLength)
{
    memcpy(uid, _uid, _uidLength < uidLength ? _uidLength : uidLength);
}

String NfcTag::getUidString()
{
    String uidString = "";
    for (unsigned int i = 0; i < _uidLength; i++)
    {
        if (i > 0)
        {
            uidString += " ";
        }

        if (_uid[i] < 0xF)
        {
            uidString += "0";
        }

        uidString += String((unsigned int)_uid[i], (unsigned char)HEX);
    }
    uidString.toUpperCase();
    return uidString;
}

String NfcTag::getTagType()
{
    return _tagType;
}

boolean NfcTag::hasNdefMessage()
{
    return (_ndefMessage != NULL);
}

NdefMessage NfcTag::getNdefMessage()
{
    return *_ndefMessage;
}
#ifdef NDEF_USE_SERIAL

void NfcTag::print()
{
    Serial.print(F("NFC Tag - "));Serial.println(_tagType);
    Serial.print(F("UID "));Serial.println(getUidString());
    if (_ndefMessage == NULL)
    {
        Serial.println(F("\nNo NDEF Message"));
    }
    else
    {
        _ndefMessage->print();
    }
}
#endif
