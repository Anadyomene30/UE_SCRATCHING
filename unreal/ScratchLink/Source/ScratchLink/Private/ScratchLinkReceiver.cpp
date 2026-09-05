#include "ScratchLinkReceiver.h"

#include "Common/UdpSocketBuilder.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

namespace
{
constexpr uint32 kMagic = 0x314A5653;  // "SVJ1" little-endian
constexpr uint16 kVersion = 1;
constexpr uint16 kKindState = 1;
constexpr uint16 kKindSchema = 2;
constexpr int32 kRingCapacity = 512;  // more than a second at the stream's rate

// Little-endian readers. Assembled by shifts on purpose; casting a struct over
// the buffer would put the compiler's alignment opinion on the wire.
uint16 ReadU16(const uint8* Data)
{
    return static_cast<uint16>(Data[0] | (Data[1] << 8));
}

uint32 ReadU32(const uint8* Data)
{
    return static_cast<uint32>(Data[0]) | (static_cast<uint32>(Data[1]) << 8) |
           (static_cast<uint32>(Data[2]) << 16) | (static_cast<uint32>(Data[3]) << 24);
}

uint64 ReadU64(const uint8* Data)
{
    return static_cast<uint64>(ReadU32(Data)) |
           (static_cast<uint64>(ReadU32(Data + 4)) << 32);
}

float ReadF32(const uint8* Data)
{
    const uint32 Bits = ReadU32(Data);
    float Value;
    FMemory::Memcpy(&Value, &Bits, sizeof(Value));
    return Value;
}

void ReadDeck(const uint8* Data, FScratchDeckState& Deck)
{
    Deck.PositionSeconds = ReadF32(Data + 0);
    Deck.Velocity = ReadF32(Data + 4);
    Deck.Acceleration = ReadF32(Data + 8);
    Deck.ScratchRate = ReadF32(Data + 12);
    Deck.Confidence = ReadF32(Data + 16);
    // anchor_s and drift_s follow on the wire; follower-mode bookkeeping the
    // Unreal side has no use for yet.
}

// Cubic Hermite on one channel, t in [0,1], velocities already scaled to the
// span. The standard basis, written out once.
float Hermite(float P0, float V0, float P1, float V1, float T)
{
    const float T2 = T * T;
    const float T3 = T2 * T;
    return (2.0f * T3 - 3.0f * T2 + 1.0f) * P0 + (T3 - 2.0f * T2 + T) * V0 +
           (-2.0f * T3 + 3.0f * T2) * P1 + (T3 - T2) * V1;
}

}  // namespace

FScratchLinkReceiver::FScratchLinkReceiver(int32 InPort) : Port(InPort)
{
    Ring.SetNum(kRingCapacity);
}

FScratchLinkReceiver::~FScratchLinkReceiver()
{
    Shutdown();
}

bool FScratchLinkReceiver::Start()
{
    Socket = FUdpSocketBuilder(TEXT("ScratchLinkReceiver"))
                 .AsNonBlocking()
                 .BoundToPort(Port)
                 .WithReceiveBufferSize(1 << 20);
    if (Socket == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("ScratchLink: cannot bind UDP port %d"), Port);
        return false;
    }
    Thread = FRunnableThread::Create(this, TEXT("ScratchLinkReceiver"),
                                     0, TPri_AboveNormal);
    return Thread != nullptr;
}

void FScratchLinkReceiver::Shutdown()
{
    bStopping = true;
    if (Thread != nullptr)
    {
        Thread->WaitForCompletion();
        delete Thread;
        Thread = nullptr;
    }
    if (Socket != nullptr)
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        Socket = nullptr;
    }
}

uint32 FScratchLinkReceiver::Run()
{
    TArray<uint8> Buffer;
    Buffer.SetNum(65536);

    while (!bStopping)
    {
        // A short wait rather than a blocking read, so Shutdown() is never
        // stuck behind a silent sender.
        if (!Socket->Wait(ESocketWaitConditions::WaitForRead,
                          FTimespan::FromMilliseconds(100)))
        {
            continue;
        }
        uint32 Pending = 0;
        while (!bStopping && Socket->HasPendingData(Pending))
        {
            int32 Read = 0;
            if (!Socket->Recv(Buffer.GetData(), Buffer.Num(), Read))
            {
                break;
            }
            if (Read > 0)
            {
                ParseDatagram(Buffer.GetData(), Read);
            }
        }
    }
    return 0;
}

void FScratchLinkReceiver::ParseDatagram(const uint8* Data, int32 Size)
{
    if (Size < 8) return;
    if (ReadU32(Data) != kMagic) return;
    if (ReadU16(Data + 4) != kVersion) return;

    const uint16 Kind = ReadU16(Data + 6);
    bool Parsed = false;
    if (Kind == kKindState)
    {
        Parsed = ParseState(Data + 8, Size - 8);
    }
    else if (Kind == kKindSchema)
    {
        Parsed = ParseSchema(Data + 8, Size - 8);
    }
    if (Parsed)
    {
        ++PacketCount;
    }
}

bool FScratchLinkReceiver::ParseState(const uint8* Data, int32 Size)
{
    // u64 t_us | u32 hash | 7f deck A | 7f deck B | u32 gestures | u16 count
    constexpr int32 kFixed = 8 + 4 + 28 + 28 + 4 + 2;
    if (Size < kFixed) return false;

    const uint16 Count = ReadU16(Data + kFixed - 2);
    const int32 KnownBytes = (Count + 7) / 8;
    // The size check comes BEFORE any allocation: a corrupt count must fail
    // here, not reserve gigabytes first.
    if (Size < kFixed + Count * 4 + KnownBytes) return false;

    FScratchLinkState State;
    State.TimeSeconds = static_cast<double>(ReadU64(Data)) * 1e-6;
    const uint32 Hash = ReadU32(Data + 8);
    ReadDeck(Data + 12, State.DeckA);
    ReadDeck(Data + 40, State.DeckB);

    const uint32 Gestures = ReadU32(Data + 68);
    State.DeckA.bScratching = (Gestures & (1u << 0)) != 0;
    State.DeckB.bScratching = (Gestures & (1u << 1)) != 0;
    State.DeckA.bBackspin = (Gestures & (1u << 2)) != 0;
    State.DeckB.bBackspin = (Gestures & (1u << 3)) != 0;
    State.DeckA.bHolding = (Gestures & (1u << 4)) != 0;
    State.DeckB.bHolding = (Gestures & (1u << 5)) != 0;
    State.bTransform = (Gestures & (1u << 6)) != 0;

    State.Values.SetNum(Count);
    const uint8* Floats = Data + kFixed;
    for (int32 i = 0; i < Count; ++i)
    {
        State.Values[i] = ReadF32(Floats + i * 4);
    }
    State.Known.SetNum(Count);
    const uint8* Bits = Floats + Count * 4;
    for (int32 i = 0; i < Count; ++i)
    {
        State.Known[i] = (Bits[i / 8] & (1u << (i % 8))) != 0;
    }

    FScopeLock Lock(&Guard);
    // A state referencing a schema we have not seen is still stored: positions
    // and gestures are self-describing, only Values need names.
    (void)Hash;
    Ring[RingNext] = MoveTemp(State);
    RingNext = (RingNext + 1) % kRingCapacity;
    RingCount = FMath::Min(RingCount + 1, kRingCapacity);
    return true;
}

bool FScratchLinkReceiver::ParseSchema(const uint8* Data, int32 Size)
{
    if (Size < 6) return false;
    const uint32 Hash = ReadU32(Data);
    const uint16 Count = ReadU16(Data + 4);

    TArray<FScratchSchemaEntry> Entries;
    Entries.Reserve(Count);
    int32 Offset = 6;
    for (uint16 i = 0; i < Count; ++i)
    {
        if (Size < Offset + 2) return false;
        FScratchSchemaEntry Entry;
        Entry.Kind = Data[Offset];
        const int32 IdLength = Data[Offset + 1];
        Offset += 2;
        if (Size < Offset + IdLength) return false;
        Entry.Id = FString(IdLength, reinterpret_cast<const ANSICHAR*>(Data + Offset));
        Offset += IdLength;
        Entries.Add(MoveTemp(Entry));
    }

    FScopeLock Lock(&Guard);
    Schema = MoveTemp(Entries);
    SchemaHash = Hash;
    return true;
}

bool FScratchLinkReceiver::GetLatest(FScratchLinkState& Out) const
{
    FScopeLock Lock(&Guard);
    if (RingCount == 0) return false;
    const int32 Newest = (RingNext + kRingCapacity - 1) % kRingCapacity;
    Out = Ring[Newest];
    return true;
}

bool FScratchLinkReceiver::SampleAt(double TimeSeconds, FScratchLinkState& Out) const
{
    FScopeLock Lock(&Guard);
    if (RingCount == 0) return false;

    // Walk back from the newest sample to the pair bracketing the target. The
    // ring holds about a second, and the target is a few frames behind live, so
    // this loop ends almost immediately.
    const int32 Newest = (RingNext + kRingCapacity - 1) % kRingCapacity;
    int32 After = Newest;
    for (int32 Step = 0; Step < RingCount - 1; ++Step)
    {
        const int32 Before = (After + kRingCapacity - 1) % kRingCapacity;
        if (Ring[Before].TimeSeconds <= TimeSeconds || Step == RingCount - 2)
        {
            const FScratchLinkState& S0 = Ring[Before];
            const FScratchLinkState& S1 = Ring[After];
            const double Span = S1.TimeSeconds - S0.TimeSeconds;
            if (Span <= 0.0)
            {
                Out = S1;
                return true;
            }
            const float T = static_cast<float>(
                FMath::Clamp((TimeSeconds - S0.TimeSeconds) / Span, 0.0, 1.0));

            // Everything is stepped or lerped except the platter positions,
            // which get the Hermite treatment: the wire carries velocity in
            // record-seconds per wall-second, exactly the tangent the basis
            // wants once scaled by the span.
            Out = T < 0.5f ? S0 : S1;
            Out.TimeSeconds = TimeSeconds;
            const float SpanF = static_cast<float>(Span);
            Out.DeckA.PositionSeconds =
                Hermite(S0.DeckA.PositionSeconds, S0.DeckA.Velocity * SpanF,
                        S1.DeckA.PositionSeconds, S1.DeckA.Velocity * SpanF, T);
            Out.DeckB.PositionSeconds =
                Hermite(S0.DeckB.PositionSeconds, S0.DeckB.Velocity * SpanF,
                        S1.DeckB.PositionSeconds, S1.DeckB.Velocity * SpanF, T);
            Out.DeckA.Velocity = FMath::Lerp(S0.DeckA.Velocity, S1.DeckA.Velocity, T);
            Out.DeckB.Velocity = FMath::Lerp(S0.DeckB.Velocity, S1.DeckB.Velocity, T);
            return true;
        }
        After = Before;
    }
    Out = Ring[Newest];
    return true;
}

TArray<FScratchSchemaEntry> FScratchLinkReceiver::GetSchema() const
{
    FScopeLock Lock(&Guard);
    return Schema;
}
