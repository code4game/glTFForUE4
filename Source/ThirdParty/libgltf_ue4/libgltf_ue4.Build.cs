// Copyright (o) 2016-2020 Code 4 Game <develop@c4g.io>

using UnrealBuildTool;
using System.Collections.Generic;

public class libgltf_ue4 : ModuleRules
{
    public libgltf_ue4(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string ExtrasPath = System.IO.Path.Combine(ModuleDirectory, "..", "..", "..", "Extras");
        string glTFPath = System.IO.Path.Combine(ExtrasPath, "libgltf-0.1.8");
        string IncludePath = System.IO.Path.Combine(glTFPath, "include");
        List<string> LibPaths = new List<string>();
        string LibFilePath = "";

#if UE_5_0_OR_LATER
        if (Target.Platform == UnrealTargetPlatform.Win64)
#else
        if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
#endif
        {
            string PlatformName = "";
#if UE_5_0_OR_LATER
            PlatformName = "win64";
#elif UE_4_23_OR_LATER
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

            string VSFolderName = (Target.WindowsPlatform.GetVisualStudioCompilerVersionName() == "2019") ? "vs2019" : "vs2017";
            string LibPath = System.IO.Path.Combine(glTFPath, "lib", PlatformName, VSFolderName, "Release");
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
