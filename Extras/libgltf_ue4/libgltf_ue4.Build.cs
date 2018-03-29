// Copyright (o) 2016-2018 Code 4 Game <develop@c4g.io>

using UnrealBuildTool;

public class libgltf_ue4 : ModuleRules
{
    public libgltf_ue4(TargetInfo Target)
    {
        Type = ModuleType.External;

        string glTFPath = System.IO.Path.Combine(ModuleDirectory, "libgltf-0.1.0");
        string IncludePath = System.IO.Path.Combine(glTFPath, "include");
        string LibPath = "";

        if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
        {
            string PlatformName = "";
            switch (Target.Platform)
            {
            case UnrealTargetPlatform.Win32:
                PlatformName = "win32";
                break;
            case UnrealTargetPlatform.Win64:
                PlatformName = "win64";
                break;
            }

            string VSName = "vs" + WindowsPlatform.GetVisualStudioCompilerVersionName();

            LibPath = System.IO.Path.Combine(glTFPath, "lib", PlatformName, VSName);

            PublicAdditionalLibraries.Add("libgltf.lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            LibPath = System.IO.Path.Combine(glTFPath, "lib", "linux");

            PublicAdditionalLibraries.Add("libgltf.a");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            LibPath = System.IO.Path.Combine(glTFPath, "lib", "macos");

            PublicAdditionalLibraries.Add("libgltf.a");
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            LibPath = System.IO.Path.Combine(glTFPath, "lib", "ios");

            PublicAdditionalLibraries.Add("libgltf.a");
        }

        PublicIncludePaths.Add(IncludePath);
        PublicLibraryPaths.Add(LibPath);
    }
}
