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
    virtual bool FactoryCanImport(const FString& InSystemFilePath) override;
    // End UFactory Interface

public:
    virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* InContext, const TCHAR* InType, const TCHAR*& InBuffer, const TCHAR* InBufferEnd, FFeedbackContext* InWarn);

private:
    //HACK: It is not safe
    FString FilePathInOS;
};
