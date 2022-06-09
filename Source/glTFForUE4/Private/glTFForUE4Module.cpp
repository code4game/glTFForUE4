// Copyright(c) 2016 - 2022 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTFForUE4Module.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "glTFForUE4Module"

class FglTFForUE4Module : public IglTFForUE4Module
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule()
    {
        //
    }

    virtual void ShutdownModule()
    {
        //
    }
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FglTFForUE4Module, glTFForUE4)
