#include "ScratchDilationComponent.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "ScratchLinkSubsystem.h"

UScratchDilationComponent::UScratchDilationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UScratchDilationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const UGameInstance* Game = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
    UScratchLinkSubsystem* Link =
        Game != nullptr ? Game->GetSubsystem<UScratchLinkSubsystem>() : nullptr;
    if (Link == nullptr) return;

    FScratchLinkState State;
    if (!Link->SampleState(SampleDelay, State)) return;
    const FScratchDeckState& Deck = bUseDeckA ? State.DeckA : State.DeckB;

    // A lost timecode lock FREEZES the world -- it never teleports it. Holding
    // the last dilation would keep the scene drifting on a platter that may not
    // be moving at all; pinning to near-zero is the visual equivalent of the
    // frozen frame the video side shows. (True zero is rejected by the engine.)
    if (Deck.bHolding)
    {
        UGameplayStatics::SetGlobalTimeDilation(this, 0.0001f);
        return;
    }

    // |velocity|: dilation has no direction. Running the record backwards makes
    // the WORLD run slow-motion-forward at the same rate; actual rewind is the
    // time machine component's job, not a sign flip here.
    const float Dilation = FMath::Clamp(FMath::Abs(Deck.Velocity), 0.0001f, MaxDilation);
    UGameplayStatics::SetGlobalTimeDilation(this, Dilation);
}

void UScratchDilationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    // Leaving a stopped world behind would look like a crash in the editor.
    UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
    Super::EndPlay(Reason);
}
