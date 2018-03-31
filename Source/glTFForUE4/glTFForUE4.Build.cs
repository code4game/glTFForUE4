// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

using UnrealBuildTool;

public class glTFForUE4 : ModuleRules
{
    public glTFForUE4(TargetInfo Target)
    {
        PublicIncludePaths.AddRange(new [] {
                "glTFForUE4/Public"
            });

        PrivateIncludePaths.AddRange(new [] {
                "glTFForUE4/Private",
            });

        PublicDependencyModuleNames.AddRange(new [] {
                "Core",
            });

        PrivateDependencyModuleNames.AddRange(new [] {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
            });

        string ExtraPathRoot = System.IO.Path.Combine(ModuleDirectory, "..", "..", "Extras");

        // libgltf
        {
            string glTFPath = System.IO.Path.Combine(ExtraPathRoot, "libgltf_ue4", "libgltf-0.1.3");
            string IncludePath = System.IO.Path.Combine(glTFPath, "include");
            string LibPath = "";
            string LibFilePath = "";

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

                LibFilePath = System.IO.Path.Combine(LibPath, "libgltf.lib");
            }
            else if (Target.Platform == UnrealTargetPlatform.Linux)
            {
                LibPath = System.IO.Path.Combine(glTFPath, "lib", "linux");

                LibFilePath = System.IO.Path.Combine(LibPath, "libgltf.a");
            }
            else if (Target.Platform == UnrealTargetPlatform.Mac)
            {
                LibPath = System.IO.Path.Combine(glTFPath, "lib", "macos");

                LibFilePath = System.IO.Path.Combine(LibPath, "libgltf.a");
            }
            else if (Target.Platform == UnrealTargetPlatform.IOS)
            {
                LibPath = System.IO.Path.Combine(glTFPath, "lib", "ios");

                LibFilePath = System.IO.Path.Combine(LibPath, "libgltf.a");
            }

            PublicIncludePaths.Add(IncludePath);
            PublicLibraryPaths.Add(LibPath);
            PublicAdditionalLibraries.Add(LibFilePath);
        }

        // libdraco
        {
            string DracoPath = System.IO.Path.Combine(ExtraPathRoot, "libdraco_ue4", "libdraco-1.2.5");
            string IncludePath = System.IO.Path.Combine(DracoPath, "include");
            string LibPath = "";
            string LibFilePath1 = "";
            string LibFilePath2 = "";

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

            PublicIncludePaths.Add(IncludePath);
            PublicLibraryPaths.Add(LibPath);
            PublicAdditionalLibraries.Add(LibFilePath1);
            PublicAdditionalLibraries.Add(LibFilePath2);
        }
    }
}
