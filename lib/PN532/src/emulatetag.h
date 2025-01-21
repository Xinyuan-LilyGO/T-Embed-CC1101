/**************************************************************************/
/*!
    @file     emulatetag.h
    @author   Armin Wieser
    @license  BSD

    Implemented using NFC forum documents & library of libnfc
*/
/**************************************************************************/

#ifndef __EMULATETAG_H__
#define __EMULATETAG_H__

#include "PN532.h"

#define NDEF_MAX_LENGTH 128 // altough ndef can handle up to 0xfffe in size, arduino cannot.
typedef enum
{
  COMMAND_COMPLETE,
  TAG_NOT_FOUND,
  FUNCTION_NOT_SUPPORTED,
  MEMORY_FAILURE,
  END_OF_FILE_BEFORE_REACHED_LE_BYTES
} responseCommand;

class EmulateTag
{

public:
  EmulateTag(PN532Interface &interface) : pn532(interface), uidPtr(0), tagWrittenByInitiator(false), tagWriteable(true), updateNdefCallback(0) {}

  bool init();

  bool emulate(const uint16_t tgInitAsTargetTimeout = 0);

  /*
   * @param uid pointer to byte array of length 3 (uid is 4 bytes - first byte is fixed) or zero for uid 
   */
  void setUid(uint8_t *uid = 0);

  void setNdefFile(const uint8_t *ndef, const int16_t ndefLength);

  void getContent(uint8_t **buf, uint16_t *length)
  {
    *buf = ndef_file + 2; // first 2 bytes = length
    *length = (ndef_file[0] << 8) + ndef_file[1];
  }

  bool writeOccured()
  {
    return tagWrittenByInitiator;
  }

  void setTagWriteable(bool setWriteable)
  {
    tagWriteable = setWriteable;
  }

  uint8_t *getNdefFilePtr()
  {
    return ndef_file;
  }

  uint8_t getNdefMaxLength()
  {
    return NDEF_MAX_LENGTH;
  }

  void attach(void (*func)(uint8_t *buf, uint16_t length))
  {
    updateNdefCallback = func;
  };

private:
  PN532 pn532;
  uint8_t ndef_file[NDEF_MAX_LENGTH];
  uint8_t *uidPtr;
  bool tagWrittenByInitiator;
  bool tagWriteable;
  void (*updateNdefCallback)(uint8_t *ndef, uint16_t length);

  void setResponse(responseCommand cmd, uint8_t *buf, uint8_t *sendlen, uint8_t sendlenOffset = 0);
};

#endif
