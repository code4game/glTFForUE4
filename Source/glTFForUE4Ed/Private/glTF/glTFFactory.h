// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"
#include "glTFFactory.generated.h"

UCLASS(hidecategories=Object)
class UglTFFactory : public UFactory
{
    GENERATED_UCLASS_BODY()

public:
    // Begin UFactory Interface
    virtual bool DoesSupportClass(UClass* InClass) override;
    virtual UClass* ResolveSupportedClass() override;
    virtual bool FactoryCanImport(const FString& InSystemFilePath) override;
    // End UFactory Interface

public:
    virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, const TCHAR* InType, const TCHAR*& InBuffer, const TCHAR* InBufferEnd, FFeedbackContext* InWarn);

protected:
    virtual UObject* FactoryCreate(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, const TCHAR* InType, FFeedbackContext* InWarn, const FString& InglTFJson, TSharedPtr<class FglTFBuffers> InglTFBuffers = nullptr);

private:
    UClass* ImportClass;
};
