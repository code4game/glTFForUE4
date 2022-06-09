// Copyright (o) 2016-2020 Code 4 Game <develop@c4g.io>

using UnrealBuildTool;
using System.Collections.Generic;

public class libdraco_ue4 : ModuleRules
{
    public libdraco_ue4(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string ExtrasPath = System.IO.Path.Combine(ModuleDirectory, "..", "..", "..", "Extras");
        string DracoPath = System.IO.Path.Combine(ExtrasPath, "libdraco-1.3.0");
        string IncludePath = System.IO.Path.Combine(DracoPath, "include");
        List<string> LibPaths = new List<string>();
        List<string> LibFilePaths = new List<string>();

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
            string LibPath = System.IO.Path.Combine(DracoPath, "lib", PlatformName, VSFolderName, "Release");
            LibPaths.Add(LibPath);

            LibFilePaths.Add(System.IO.Path.Combine(LibPath, "dracodec.lib"));
            LibFilePaths.Add(System.IO.Path.Combine(LibPath, "dracoenc.lib"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string LibPath = System.IO.Path.Combine(DracoPath, "lib", "macos");
            LibPaths.Add(LibPath);

            LibFilePaths.Add(System.IO.Path.Combine(LibPath, "dracodec.a"));
            LibFilePaths.Add(System.IO.Path.Combine(LibPath, "dracoenc.a"));
        }

        PublicIncludePaths.Add(IncludePath);
#if UE_4_24_OR_LATER
        PublicSystemLibraryPaths.AddRange(LibPaths);
#else
        PublicLibraryPaths.AddRange(LibPaths);
#endif
        PublicAdditionalLibraries.AddRange(LibFilePaths);
    }
}
