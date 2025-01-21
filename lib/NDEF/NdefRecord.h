#ifndef NdefRecord_h
#define NdefRecord_h

#include <Due.h>
#include <Arduino.h>
#include <Ndef.h>

#define TNF_EMPTY 0x0
#define TNF_WELL_KNOWN 0x01
#define TNF_MIME_MEDIA 0x02
#define TNF_ABSOLUTE_URI 0x03
#define TNF_EXTERNAL_TYPE 0x04
#define TNF_UNKNOWN 0x05
#define TNF_UNCHANGED 0x06
#define TNF_RESERVED 0x07

class NdefRecord
{
    public:
        NdefRecord();
        NdefRecord(const NdefRecord& rhs);
        ~NdefRecord();
        NdefRecord& operator=(const NdefRecord& rhs);

        int getEncodedSize();
        void encode(byte *data, bool firstRecord, bool lastRecord);

        unsigned int getTypeLength();
        int getPayloadLength();
        unsigned int getIdLength();

        byte getTnf();
        void getType(byte *type);
        void getPayload(byte *payload);
        void getId(byte *id);

        // convenience methods
        String getType();
        String getId();

        void setTnf(byte tnf);
        void setType(const byte *type, const unsigned int numBytes);
        void setPayload(const byte *payload, const int numBytes);
        void setId(const byte *id, const unsigned int numBytes);

#ifdef NDEF_USE_SERIAL
        void print();
#endif
    private:
        byte getTnfByte(bool firstRecord, bool lastRecord);
        byte _tnf; // 3 bit
        unsigned int _typeLength;
        int _payloadLength;
        unsigned int _idLength;
        byte *_type;
        byte *_payload;
        byte *_id;
};

#endif