// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

using UnrealBuildTool;

public class glTFForUE4 : ModuleRules
{
    public glTFForUE4(TargetInfo Target)
    {

        PublicIncludePaths.AddRange(
            new string[] {
                "glTFForUE4/Public"
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "glTFForUE4/Private",
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
            }
            );

        // libgltf
        string LibName = "libgltf";
        string LibExtension = "";
        string LibPathRoot = System.IO.Path.Combine(ModuleDirectory, "..", "..", "Extras", LibName);

        string LibPathInclude = System.IO.Path.Combine(LibPathRoot, "include");
        PublicIncludePaths.Add(LibPathInclude);

        string LibPathLibrary = System.IO.Path.Combine(LibPathRoot, "lib");
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            LibPathLibrary = System.IO.Path.Combine(LibPathLibrary, "win64");
            LibExtension = "lib";
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            LibPathLibrary = System.IO.Path.Combine(LibPathLibrary, "win32");
            LibExtension = "lib";
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            LibPathLibrary = System.IO.Path.Combine(LibPathLibrary, "macos");
            LibExtension = "a";
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            LibPathLibrary = System.IO.Path.Combine(LibPathLibrary, "linux");
            LibExtension = "a";
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            LibPathLibrary = System.IO.Path.Combine(LibPathLibrary, "android");
            LibExtension = "a";
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            LibPathLibrary = System.IO.Path.Combine(LibPathLibrary, "ios");
            LibExtension = "a";
        }
        if (System.IO.Directory.Exists(System.IO.Path.Combine(LibPathLibrary, "Release")))
        {
            LibPathLibrary = System.IO.Path.Combine(LibPathLibrary, "Release");
        }
        LibName = LibName + "." + LibExtension;

        PublicLibraryPaths.Add(LibPathLibrary);
        PublicAdditionalLibraries.Add(LibName);
    }
}
