// ScratchLink — the poor man's time machine, wired first on purpose.
//
// The real ScratchTimeMachineComponent (a 120 Hz ring buffer with a SCRUB mode
// that reinjects derived velocities) needs a scene to be tuned against, so it
// waits for one. This component is the version the roadmap says to cable
// FIRST to validate the whole chain: platter velocity -> global time dilation,
// clamped. Drop it on any actor, scratch the record, and the world slows,
// stops and runs with the hand. If this feels right, the chain is right, and
// the real time machine is an upgrade rather than a debug session.
#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "ScratchDilationComponent.generated.h"

UCLASS(ClassGroup = (ScratchLink), meta = (BlueprintSpawnableComponent))
class SCRATCHLINK_API UScratchDilationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UScratchDilationComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    // Which deck drives the world.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScratchLink")
    bool bUseDeckA = true;

    // Dilation ceiling. Unreal clamps hard above ~20; 4 already feels violent.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScratchLink",
              meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float MaxDilation = 4.0f;

    // Seconds behind live to sample, hiding network jitter.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScratchLink",
              meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float SampleDelay = 0.03f;
};
