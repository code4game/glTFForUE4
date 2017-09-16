// Copyright 2017 Code 4 Game, Inc. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTFForUE4EdModule.h"

/// From glTFForUE4 module
#include "glTFForUE4Settings.h"

/// From glTFForUE4Ed module
#include "glTF/glTFFactory.h"
#include "glTF/glTFBinaryFactory.h"

/// From engine
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4EdModule"

class FglTFForUE4EdModule : public IglTFForUE4EdModule
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule()
    {
        UE_LOG(LogglTFForUE4Ed, Display, TEXT("FglTFForUE4EdModule::StartupModule!"));

        if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
        {
            SettingsModule->RegisterSettings("Project", "Plugins", "glTF for UE4",
                LOCTEXT("Title", "glTF for UE4"),
                LOCTEXT("Description", "glTF for UE4"),
                GetMutableDefault<UglTFForUE4Settings>());
        }

        UglTFFactory::StaticClass();
        UglTFBinaryFactory::StaticClass();
    }

    virtual void ShutdownModule()
    {
        if (!UObjectInitialized())
        {
            return;
        }

        if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
        {
            SettingsModule->UnregisterSettings("Project", "Plugins", "glTF for UE4");
        }
    }
};

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FglTFForUE4EdModule, glTFForUE4Ed)
