// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

using UnrealBuildTool;

public class glTFForUE4Ed : ModuleRules
{
    public glTFForUE4Ed(TargetInfo Target)
    {
        PublicIncludePaths.AddRange(new [] {
                "glTFForUE4Ed/Public"
            });

        PrivateIncludePaths.AddRange(new [] {
                "glTFForUE4Ed/Private",
            });

        PublicDependencyModuleNames.AddRange(new [] {
                "Core",
            });

        PrivateDependencyModuleNames.AddRange(new [] {
                "CoreUObject",
                "Engine",
                "RHI",
                "InputCore",
                "RenderCore",
                "SlateCore",
                "Slate",
                "ImageWrapper",
                "UnrealEd",
                "MainFrame",
                "Documentation",
                "PropertyEditor",
                "EditorStyle",
                "RawMesh",
                "glTFForUE4",
            });

        string ExtraPathRoot = System.IO.Path.Combine(ModuleDirectory, "..", "..", "Extras");

        // libgltf
        {
            string glTFPath = System.IO.Path.Combine(ExtraPathRoot, "libgltf_ue4", "libgltf-0.1.1");
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

        // libdraco
        {
            string DracoPath = System.IO.Path.Combine(ExtraPathRoot, "libdraco_ue4", "libdraco-1.2.5");
            string IncludePath = System.IO.Path.Combine(DracoPath, "include");
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

                LibPath = System.IO.Path.Combine(DracoPath, "lib", PlatformName, VSName);

                PublicAdditionalLibraries.Add("dracodec.lib");
                PublicAdditionalLibraries.Add("dracoenc.lib");
            }
            else if (Target.Platform == UnrealTargetPlatform.Linux)
            {
                LibPath = System.IO.Path.Combine(DracoPath, "lib", "linux");

                PublicAdditionalLibraries.Add("libdracodec.a");
                PublicAdditionalLibraries.Add("libdracoenc.a");
            }
            else if (Target.Platform == UnrealTargetPlatform.Mac)
            {
                LibPath = System.IO.Path.Combine(DracoPath, "lib", "macos");

                PublicAdditionalLibraries.Add("libdracodec.a");
                PublicAdditionalLibraries.Add("libdracoenc.a");
            }

            PublicIncludePaths.Add(IncludePath);
            PublicLibraryPaths.Add(LibPath);
        }
    }
}
