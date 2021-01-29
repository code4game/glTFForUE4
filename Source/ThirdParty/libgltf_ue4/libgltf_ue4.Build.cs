// Copyright (o) 2016-2020 Code 4 Game <develop@c4g.io>

using UnrealBuildTool;
using System.Collections.Generic;

public class libgltf_ue4 : ModuleRules
{
    public libgltf_ue4(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string glTFPath = System.IO.Path.Combine(ModuleDirectory, "libgltf-0.1.7");
        string IncludePath = System.IO.Path.Combine(glTFPath, "include");
        List<string> LibPaths = new List<string>();
        string LibFilePath = "";

        if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
        {
            string PlatformName = "";
#if UE_4_23_OR_LATER
            if (Target.Platform == UnrealTargetPlatform.Win32)
            {
                PlatformName = "win32";
            }
            else if (Target.Platform == UnrealTargetPlatform.Win64)
            {
                PlatformName = "win64";
            }
#else
            switch (Target.Platform)
            {
            case UnrealTargetPlatform.Win32:
                PlatformName = "win32";
                break;
            case UnrealTargetPlatform.Win64:
                PlatformName = "win64";
                break;
            }
#endif

            string LibPath = System.IO.Path.Combine(glTFPath, "lib", PlatformName, "vs2019", "Release");
            LibPaths.Add(LibPath);

            LibFilePath = System.IO.Path.Combine(LibPath, "libgltf.lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string LibPath = System.IO.Path.Combine(glTFPath, "lib", "macos");
            LibPaths.Add(LibPath);

            LibFilePath = System.IO.Path.Combine(LibPath, "libgltf.a");
        }

        PublicIncludePaths.Add(IncludePath);
#if UE_4_24_OR_LATER
        PublicSystemLibraryPaths.AddRange(LibPaths);
#else
        PublicLibraryPaths.AddRange(LibPaths);
#endif
        PublicAdditionalLibraries.Add(LibFilePath);
        PublicDefinitions.Add("LIBGLTF_CHARACTOR_ENCODING_IS_UTF8");
    }
}
