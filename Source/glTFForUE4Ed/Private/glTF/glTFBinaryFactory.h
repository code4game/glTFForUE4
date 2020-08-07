// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTFFactory.h"
#include "glTFBinaryFactory.generated.h"

UCLASS(hidecategories=Object)
class UglTFBinaryFactory : public UglTFFactory
{
    GENERATED_UCLASS_BODY()

public:
    // Begin UFactory Interface
    virtual bool FactoryCanImport(const FString& InFilename) override;
    // End UFactory Interface

public:
    virtual UObject* FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, const TCHAR* InType, const uint8*& InBuffer, const uint8* InBufferEnd, FFeedbackContext* InWarn);
};
