#include "ScratchLinkSubsystem.h"

#include "ScratchLinkReceiver.h"

void UScratchLinkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Receiver = new FScratchLinkReceiver(DefaultPort);
    if (!Receiver->Start())
    {
        // A busy port is a normal way to run two clients on one machine; the
        // subsystem stays alive and simply never reports receiving.
        delete Receiver;
        Receiver = nullptr;
    }
}

void UScratchLinkSubsystem::Deinitialize()
{
    if (Receiver != nullptr)
    {
        Receiver->Shutdown();
        delete Receiver;
        Receiver = nullptr;
    }
    Super::Deinitialize();
}

bool UScratchLinkSubsystem::IsReceiving() const
{
    return Receiver != nullptr && Receiver->PacketsReceived() > 0;
}

bool UScratchLinkSubsystem::GetLatestState(FScratchLinkState& State) const
{
    return Receiver != nullptr && Receiver->GetLatest(State);
}

bool UScratchLinkSubsystem::SampleState(float DelaySeconds, FScratchLinkState& State) const
{
    if (Receiver == nullptr) return false;
    FScratchLinkState Latest;
    if (!Receiver->GetLatest(Latest)) return false;
    return Receiver->SampleAt(Latest.TimeSeconds - FMath::Max(0.0f, DelaySeconds), State);
}

bool UScratchLinkSubsystem::GetControl(const FString& ControlId, float& Value) const
{
    if (Receiver == nullptr) return false;

    FScratchLinkState Latest;
    if (!Receiver->GetLatest(Latest)) return false;

    const TArray<FScratchSchemaEntry> Schema = Receiver->GetSchema();
    for (int32 i = 0; i < Schema.Num(); ++i)
    {
        if (Schema[i].Id == ControlId)
        {
            if (!Latest.Values.IsValidIndex(i)) return false;
            if (!Latest.Known.IsValidIndex(i) || !Latest.Known[i]) return false;
            Value = Latest.Values[i];
            return true;
        }
    }
    return false;
}

TArray<FString> UScratchLinkSubsystem::GetControlIds() const
{
    TArray<FString> Ids;
    if (Receiver == nullptr) return Ids;
    for (const FScratchSchemaEntry& Entry : Receiver->GetSchema())
    {
        Ids.Add(Entry.Id);
    }
    return Ids;
}
