// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

using UnrealBuildTool;

public class glTFForUE4Ed : ModuleRules
{
    public glTFForUE4Ed(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                "glTFForUE4Ed/Public"
            }
            );

        PrivateIncludePaths.AddRange(
            new string[] {
                "glTFForUE4Ed/Private",
            }
            );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "RenderCore",
                "UnrealEd",
                "MainFrame",
                "Documentation",
                "PropertyEditor",
                "EditorStyle",
                "RawMesh",
                "glTFForUE4",
            }
            );
    }
}
