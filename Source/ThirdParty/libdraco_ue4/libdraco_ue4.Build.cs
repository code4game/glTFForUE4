// Copyright 2016 - 2020 Code 4 Game <develop@c4g.io>

using UnrealBuildTool;

public class libdraco_ue4 : ModuleRules
{
    public libdraco_ue4(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string DracoPath = System.IO.Path.Combine(ModuleDirectory, "libdraco-1.2.5");
        string IncludePath = System.IO.Path.Combine(DracoPath, "include");
        string LibPath = "";
        string LibFilePath1 = "";
        string LibFilePath2 = "";

        if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
        {
            string PlatformName = "";
            if (Target.Platform == UnrealTargetPlatform.Win32)
            {
                PlatformName = "win32";
            }
            else if (Target.Platform == UnrealTargetPlatform.Win64)
            {
                PlatformName = "win64";
            }

            string VSName = "vs" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName();

            LibPath = System.IO.Path.Combine(DracoPath, "lib", PlatformName, VSName);

            LibFilePath1 = System.IO.Path.Combine(LibPath, "dracodec.lib");
            LibFilePath2 = System.IO.Path.Combine(LibPath, "dracoenc.lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            LibPath = System.IO.Path.Combine(DracoPath, "lib", "linux");

            LibFilePath1 = System.IO.Path.Combine(LibPath, "libdracodec.a");
            LibFilePath2 = System.IO.Path.Combine(LibPath, "libdracoenc.a");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            LibPath = System.IO.Path.Combine(DracoPath, "lib", "macos");

            LibFilePath1 = System.IO.Path.Combine(LibPath, "libdracodec.a");
            LibFilePath2 = System.IO.Path.Combine(LibPath, "libdracoenc.a");
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            LibPath = System.IO.Path.Combine(DracoPath, "lib", "ios");

            LibFilePath1 = System.IO.Path.Combine(LibPath, "libdracodec.a");
            LibFilePath2 = System.IO.Path.Combine(LibPath, "libdracoenc.a");
        }

        PublicIncludePaths.Add(IncludePath);
        PublicLibraryPaths.Add(LibPath);
        PublicAdditionalLibraries.Add(LibFilePath1);
        PublicAdditionalLibraries.Add(LibFilePath2);
    }
}
