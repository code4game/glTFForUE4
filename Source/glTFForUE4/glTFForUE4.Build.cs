// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

using UnrealBuildTool;

public class glTFForUE4 : ModuleRules
{
    public glTFForUE4(TargetInfo Target)
    {
        PublicIncludePaths.AddRange(new [] {
                "glTFForUE4/Public"
            });

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
