// Copyright(c) 2016 - 2023 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTFBinaryFactory.h"
#include <EditorReimportHandler.h>
#include "glTFBinaryReimportFactory.generated.h"

UCLASS(hidecategories = Object)
class UglTFBinaryReimportFactory : public UglTFBinaryFactory, public FReimportHandler
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
class UglTFBinarySkeletalMeshReimportFactory : public UglTFBinaryReimportFactory
{
    GENERATED_UCLASS_BODY()

public:
};
