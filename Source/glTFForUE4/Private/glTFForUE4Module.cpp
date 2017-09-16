// Copyright 2017 Code 4 Game, Inc. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTFForUE4Module.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4Module"

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