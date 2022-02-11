#ifndef WIREGUARDTYPES_H
#define WIREGUARDTYPES_H

#include <QtCore>

enum class WireGuardState
{
    NONE,       // WireGuard is not started.
    FAILURE,    // WireGuard is in error state.
    STARTING,   // WireGuard is initializing/starting up.
    LISTENING,  // WireGuard is listening for UAPI commands, but not connected.
    CONNECTING, // WireGuard is configured and awaits for a handshake.
    ACTIVE,     // WireGuard is connected.
};

struct WireGuardStatus
{
    WireGuardState state;
    quint64 lastHandshake;
    quint64 bytesTransmitted;
    quint64 bytesReceived;
};

#endif // WIREGUARDTYPES_H
