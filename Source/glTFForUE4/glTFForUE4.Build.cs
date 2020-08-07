// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

using UnrealBuildTool;

public class glTFForUE4 : ModuleRules
{
    public glTFForUE4(ReadOnlyTargetRules Target) : base(Target)
    {
#if UE_4_25_OR_LATER
        PCHUsage = PCHUsageMode.UseSharedPCHs;
#elif UE_4_24_OR_LATER
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
#else
        PCHUsage = PCHUsageMode.UseSharedPCHs;
#endif
        PrivatePCHHeaderFile = "Private/glTFForUE4PrivatePCH.h";

        PrivateIncludePaths.AddRange(new [] {
                "glTFForUE4/Private",
            });

        PublicDependencyModuleNames.AddRange(new [] {
                "Core",
            });

        PrivateDependencyModuleNames.AddRange(new [] {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "AssetRegistry",
                "libgltf_ue4",
                "libdraco_ue4",
            });
    }
}
