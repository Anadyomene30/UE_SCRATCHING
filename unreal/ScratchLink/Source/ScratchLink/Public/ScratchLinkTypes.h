// ScratchLink — what one moment of the instrument looks like from Unreal.
#pragma once

#include "CoreMinimal.h"

#include "ScratchLinkTypes.generated.h"

// One deck's platter, as the wire carries it.
USTRUCT(BlueprintType)
struct FScratchDeckState
{
    GENERATED_BODY()

    // Position on the control record, in seconds.
    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    float PositionSeconds = 0.0f;

    // Signed speed ratio, 1.0 at nominal forward speed, negative backwards.
    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    float Velocity = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    float Acceleration = 0.0f;

    // Direction reversals per second -- how hard the hand is working.
    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    float ScratchRate = 0.0f;

    // Timecode lock quality, 0..1.
    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    float Confidence = 0.0f;

    // The platter is being scratched, far from nominal speed.
    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    bool bScratching = false;

    // Fast, sustained reverse.
    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    bool bBackspin = false;

    // TIMECODE LOCK LOST, OUTPUT FROZEN. A client that ignores this bit will
    // teleport its camera on every radio stutter; freeze instead, always.
    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    bool bHolding = false;
};

// Everything the stream carries for one instant.
USTRUCT(BlueprintType)
struct FScratchLinkState
{
    GENERATED_BODY()

    // The sender's monotonic clock, in seconds. Timestamps compare with each
    // other, never with Unreal's clock.
    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    double TimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    FScratchDeckState DeckA;

    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    FScratchDeckState DeckB;

    // Fast crossfader cuts with the platter running: a transform or a crab.
    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    bool bTransform = false;

    // Control values in schema order; names live in the subsystem. A value can
    // be present but never touched (an absolute pot at startup) -- Known says
    // which, and an unknown value is a fabrication if displayed as a reading.
    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    TArray<float> Values;

    UPROPERTY(BlueprintReadOnly, Category = "ScratchLink")
    TArray<bool> Known;
};
