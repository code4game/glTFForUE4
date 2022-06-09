// Copyright(c) 2016 - 2022 Code 4 Game, Org. All Rights Reserved.

using UnrealBuildTool;

public class glTFForUE4 : ModuleRules
{
    public glTFForUE4(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseSharedPCHs;
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
