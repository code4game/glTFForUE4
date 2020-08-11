// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTFBinaryReimportFactory.h"

UglTFBinaryReimportFactory::UglTFBinaryReimportFactory(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    ImportPriority = DefaultImportPriority - 1;
}

bool UglTFBinaryReimportFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    //
    return false;
}

void UglTFBinaryReimportFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    //
}

EReimportResult::Type UglTFBinaryReimportFactory::Reimport(UObject* Obj)
{
    //
    return EReimportResult::Failed;
}

int32 UglTFBinaryReimportFactory::GetPriority() const
{
    return ImportPriority;
}

bool UglTFBinaryReimportFactory::FactoryCanImport(const FString& Filename)
{
    return false;
}
