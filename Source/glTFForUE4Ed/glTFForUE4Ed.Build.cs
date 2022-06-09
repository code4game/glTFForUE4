// Copyright(c) 2016 - 2022 Code 4 Game, Org. All Rights Reserved.

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
#if UE_4_24_OR_LATER
                "MeshBuilder",
                "SkeletalMeshUtilitiesCommon",
#endif
#if UE_5_0_OR_LATER
                "AnimationBlueprintLibrary",
                "PhysicsUtilities",
#endif
                "AnimationModifiers",
                "glTFForUE4",
                "libgltf_ue4",
                "libdraco_ue4",
            });
    }
}
