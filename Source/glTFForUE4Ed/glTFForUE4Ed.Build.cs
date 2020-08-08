// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

using UnrealBuildTool;

public class glTFForUE4Ed : ModuleRules
{
    public glTFForUE4Ed(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseSharedPCHs;
        PrivatePCHHeaderFile = "Private/glTFForUE4EdPrivatePCH.h";

        PrivateIncludePaths.AddRange(new [] {
                "glTFForUE4Ed/Private",
            });

        PublicDependencyModuleNames.AddRange(new [] {
                "Core",
            });

        PrivateDependencyModuleNames.AddRange(new [] {
                "CoreUObject",
                "Engine",
                "RHI",
                "InputCore",
                "RenderCore",
                "SlateCore",
                "Slate",
                "ImageWrapper",
                "AssetRegistry",
                "UnrealEd",
                "MainFrame",
                "Documentation",
                "PropertyEditor",
                "EditorStyle",
                "RawMesh",
                "MeshUtilities",
                "glTFForUE4",
                "libgltf_ue4",
                "libdraco_ue4",
            });
    }
}
