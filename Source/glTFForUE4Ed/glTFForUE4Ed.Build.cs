// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

using UnrealBuildTool;

public class glTFForUE4Ed : ModuleRules
{
    public glTFForUE4Ed(TargetInfo Target)
    {
        PublicIncludePaths.AddRange(new [] {
                "glTFForUE4Ed/Public"
            });

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
