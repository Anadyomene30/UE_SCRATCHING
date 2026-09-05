// ScratchLink — the socket thread and the wire parser.
//
// Mirrors docs/protocole.md from the scratchvj repository, byte for byte:
// little-endian, assembled by shifts rather than by casting structs, so no
// compiler's idea of alignment can disagree with another's. The decoder rejects
// an unknown magic or version, any truncation, and a count the datagram cannot
// satisfy -- that last check happens BEFORE any allocation, so a corrupt packet
// cannot request a monstrous reservation.
#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"

#include "ScratchLinkTypes.h"

class FSocket;

// A named control, cached from the rare schema packets so the 375 Hz state path
// never carries strings.
struct FScratchSchemaEntry
{
    FString Id;
    uint8 Kind = 0;
};

class FScratchLinkReceiver final : public FRunnable
{
public:
    explicit FScratchLinkReceiver(int32 InPort);
    virtual ~FScratchLinkReceiver() override;

    bool Start();
    void Shutdown();

    // FRunnable
    virtual uint32 Run() override;
    virtual void Stop() override { bStopping = true; }

    // Copies the freshest sample out. False until anything has arrived.
    bool GetLatest(FScratchLinkState& Out) const;

    // Samples the ring at the sender-clock instant `TimeSeconds`, interpolating
    // position with a Hermite step (velocity is on the wire; using it beats
    // pretending we only have positions). Clamps to the ring's ends.
    bool SampleAt(double TimeSeconds, FScratchLinkState& Out) const;

    // The schema as last announced. Empty until one arrives.
    TArray<FScratchSchemaEntry> GetSchema() const;

    uint64 PacketsReceived() const { return PacketCount.load(); }

private:
    void ParseDatagram(const uint8* Data, int32 Size);
    bool ParseState(const uint8* Data, int32 Size);
    bool ParseSchema(const uint8* Data, int32 Size);

    int32 Port = 7331;
    FSocket* Socket = nullptr;
    FRunnableThread* Thread = nullptr;
    std::atomic<bool> bStopping{false};
    std::atomic<uint64> PacketCount{0};

    // The last few hundred states -- about a second of stream -- guarded by one
    // lock. Writers touch it a few hundred times a second, readers once or
    // twice a frame; contention is not a concern at this scale.
    mutable FCriticalSection Guard;
    TArray<FScratchLinkState> Ring;
    int32 RingNext = 0;
    int32 RingCount = 0;
    TArray<FScratchSchemaEntry> Schema;
    uint32 SchemaHash = 0;
};
