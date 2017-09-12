// Copyright 2017 Code 4 Game, Inc. All Rights Reserved.

using UnrealBuildTool;

public class glTFForUE4Ed : ModuleRules
{
	public glTFForUE4Ed(TargetInfo Target)
	{
		
		PublicIncludePaths.AddRange(
			new string[] {
				"glTFForUE4Ed/Public"
				// ... add public include paths required here ...
			}
			);

		PrivateIncludePaths.AddRange(
			new string[] {
				"glTFForUE4Ed/Private",
				// ... add other private include paths required here ...
			}
			);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
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
				"UnrealEd",
				"LevelEditor",
                "glTFForUE4",
				// ... add private dependencies that you statically link with here ...	
			}
			);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

        // libgltf
        string LibName = "libgltf";
        string LibPathRoot = System.IO.Path.Combine(ModuleDirectory, "..", "..", "Extras", LibName);

        string LibPathInclude = System.IO.Path.Combine(LibPathRoot, "include");
        PublicIncludePaths.Add(LibPathInclude);

        string LibPathBinary = System.IO.Path.Combine(LibPathRoot, "bin");
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            LibPathBinary = System.IO.Path.Combine(LibPathBinary, "win64");
            if (Target.Configuration == UnrealTargetConfiguration.Debug
                || Target.Configuration == UnrealTargetConfiguration.DebugGame)
            {
                LibPathBinary = System.IO.Path.Combine(LibPathBinary, "Debug");
                LibName = LibName + "d";
            }
            else
            {
                LibPathBinary = System.IO.Path.Combine(LibPathBinary, "Release");
            }
            PublicLibraryPaths.Add(LibPathBinary);
            PublicAdditionalLibraries.Add(LibName + ".lib");
        }
    }
}
