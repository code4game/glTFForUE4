// Copyright (o) 2016-2020 Code 4 Game <develop@c4g.io>

using UnrealBuildTool;
using System.Collections.Generic;

public class libdraco_ue4 : ModuleRules
{
    public libdraco_ue4(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string DracoPath = ModuleDirectory;
        string IncludePath = System.IO.Path.Combine(DracoPath, "include");
        List<string> LibPaths = new List<string>();
        List<string> LibFilePaths = new List<string>();

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

            LibPaths.Add(System.IO.Path.Combine(DracoPath, "lib", PlatformName, "vs2019", "Release"));

            LibFilePaths.Add("dracodec.lib");
            LibFilePaths.Add("dracoenc.lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string LibPath = System.IO.Path.Combine(DracoPath, "lib", "macos");
            LibPaths.Add(LibPath);

            LibFilePaths.Add(System.IO.Path.Combine(LibPath, "libdracodec.a"));
            LibFilePaths.Add(System.IO.Path.Combine(LibPath, "libdracoenc.a"));
        }

        PublicIncludePaths.Add(IncludePath);
        PublicLibraryPaths.AddRange(LibPaths);
        PublicAdditionalLibraries.AddRange(LibFilePaths);
    }
}
