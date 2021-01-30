// Copyright(c) 2016 - 2021 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTFFactory.h"
#include <EditorReimportHandler.h>
#include "glTFReimportFactory.generated.h"

UCLASS(hidecategories = Object)
class UglTFReimportFactory : public UglTFFactory, public FReimportHandler
{
    GENERATED_UCLASS_BODY()

public:
    //~ Begin FReimportHandler Interface
    virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
    virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
    virtual EReimportResult::Type Reimport(UObject* Obj) override;
    virtual int32 GetPriority() const override;
    //~ End FReimportHandler Interface

public:
    //~ Begin UFactory Interface
    virtual bool FactoryCanImport(const FString& Filename) override;
    //~ End UFactory Interface
};

UCLASS(hidecategories = Object)
class UglTFSkeletalMeshReimportFactory : public UglTFReimportFactory
{
    GENERATED_UCLASS_BODY()

public:
};
