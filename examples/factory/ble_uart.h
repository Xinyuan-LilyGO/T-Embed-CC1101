#pragma once

#include <stddef.h>
#include <stdint.h>

namespace factory_ble {

constexpr size_t kAddressTextLength = 18;
constexpr size_t kPayloadTextLength = 65;

enum Event : uint8_t {
    EventIdle = 0,
    EventAdvertising,
    EventPairing,
    EventBonded,
    EventConnected,
    EventDisconnected,
    EventAuthFailed,
    EventClearingBonds,
};

struct Status {
    uint32_t lastTickMs;
    uint32_t updateSeq;
    uint32_t passkey;
    uint32_t rxCount;
    uint32_t txCount;
    uint8_t bondCount;
    uint8_t lastEvent;
    bool initialized;
    bool connected;
    bool advertising;
    bool authInProgress;
    bool bonded;
    char localAddress[kAddressTextLength];
    char peerAddress[kAddressTextLength];
    char lastPayload[kPayloadTextLength];
};

void begin();
void service();
void stop();
bool clearBonds();
bool restartAdvertising();
void getStatus(Status* status);

}  // namespace factory_ble
