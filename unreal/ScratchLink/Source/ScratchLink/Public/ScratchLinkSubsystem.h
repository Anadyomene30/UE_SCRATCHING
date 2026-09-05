// ScratchLink — the subsystem a scene talks to.
//
// A UGameInstanceSubsystem because the stream outlives level changes: the DJ
// does not stop scratching while a map loads. The socket thread starts with the
// game instance and dies with it; Blueprints read state, never manage sockets.
//
// The one call that matters is SampleState. Unreal renders at 60-120 fps while
// the stream arrives near 375 Hz, so a frame samples the recent past --
// `latest - DelaySeconds` -- with Hermite interpolation on the platter
// positions. Reading only the newest packet would alias three or four stream
// samples into one frame and stutter exactly when the hand moves fastest.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "ScratchLinkTypes.h"

#include "ScratchLinkSubsystem.generated.h"

class FScratchLinkReceiver;

UCLASS(BlueprintType)
class SCRATCHLINK_API UScratchLinkSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // True once anything has arrived on the socket.
    UFUNCTION(BlueprintPure, Category = "ScratchLink")
    bool IsReceiving() const;

    // The freshest raw sample, no interpolation. For meters and debug.
    UFUNCTION(BlueprintPure, Category = "ScratchLink")
    bool GetLatestState(FScratchLinkState& State) const;

    // The state as of `DelaySeconds` behind the freshest sample, platter
    // positions Hermite-interpolated. 0.02-0.05 s hides network jitter without
    // a feelable lag. This is the call a camera rig or a time machine reads.
    UFUNCTION(BlueprintPure, Category = "ScratchLink")
    bool SampleState(float DelaySeconds, FScratchLinkState& State) const;

    // A control by its schema id ("xfader", "ch1.filter", ...). False when the
    // control is absent or has never been touched -- an untouched absolute pot
    // has NO value, and inventing 0.0 here would wrongly slam whatever the
    // control drives.
    UFUNCTION(BlueprintPure, Category = "ScratchLink")
    bool GetControl(const FString& ControlId, float& Value) const;

    // Every control id the sender has announced, in wire order.
    UFUNCTION(BlueprintPure, Category = "ScratchLink")
    TArray<FString> GetControlIds() const;

    // The UDP port the receiver listens on. Matches the sender's default.
    static constexpr int32 DefaultPort = 7331;

private:
    // A plain pointer rather than TUniquePtr: the UObject machinery instantiates
    // this class's destructor where only the forward declaration is visible, and
    // a smart pointer to an incomplete type fails exactly there. Lifetime is
    // Initialize/Deinitialize, which a subsystem guarantees are paired.
    FScratchLinkReceiver* Receiver = nullptr;
};
