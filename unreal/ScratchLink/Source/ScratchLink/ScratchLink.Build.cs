// ScratchLink — build rules.
//
// Deliberately NOT the engine's OSC plugin: that allocates UObjects per message
// and dispatches on the game thread, which at ~375 Hz is garbage for nothing.
// This module reads a UDP socket on its own thread and parses a fixed-layout
// little-endian payload -- see docs/protocole.md in the scratchvj repository,
// which is the authoritative description this code mirrors.
//
// The plugin links nothing from scratchvj itself. That boundary is legal as
// well as architectural: the app will be GPL-3 through xwax's timecode decoder,
// and this plugin stays independent of it by construction.
using UnrealBuildTool;

public class ScratchLink : ModuleRules
{
    public ScratchLink(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Sockets",
            "Networking",
        });
    }
}
