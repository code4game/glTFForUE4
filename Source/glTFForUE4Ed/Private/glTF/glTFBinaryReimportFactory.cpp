// Copyright(c) 2016 - 2023 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTFBinaryReimportFactory.h"

UglTFBinaryReimportFactory::UglTFBinaryReimportFactory(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    SupportedClass = UStaticMesh::StaticClass();
    ImportPriority = DefaultImportPriority + 1;
}

bool UglTFBinaryReimportFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    if (!Obj)
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("The `Obj` is nullptr when call `UglTFBinaryReimportFactory::CanReimport`!"));
        return false;
    }

    if (!Obj->IsA<UStaticMesh>() && !Obj->IsA<USkeletalMesh>())
    {
        UClass* ObjClass = Obj->GetClass();
        const FString ObjClassName = ObjClass ? ObjClass->GetName() : "None";
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("The class Is not supported! %s"), *ObjClassName);
        return false;
    }

    UAssetImportData* AssetImportData = FglTFImporterEd::GetAssetImportData(Obj);
    if (!AssetImportData)
    {
        const FString ObjPathName = Obj->GetPathName();
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Not support! %s"), *ObjPathName);
        return false;
    }

    OutFilenames = AssetImportData->ExtractFilenames();
    if (OutFilenames.Num() > 0)
    {
        return Super::FactoryCanImport(OutFilenames[0]);
    }
    return true;
}

void UglTFBinaryReimportFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    if (!Obj || NewReimportPaths.Num() <= 0) return;

    UAssetImportData* AssetImportData = FglTFImporterEd::GetAssetImportData(Obj);
    if (!AssetImportData) return;

    AssetImportData->Update(NewReimportPaths[0]);
}

EReimportResult::Type UglTFBinaryReimportFactory::Reimport(UObject* Obj)
{
    if (!Obj)
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("The `Obj` is nullptr when call `UglTFReimportFactory::Reimport`!"));
        return EReimportResult::Failed;
    }

    UAssetImportData* AssetImportData = FglTFImporterEd::GetAssetImportData(Obj);
    if (!AssetImportData)
    {
        const FString ObjPathName = Obj->GetPathName();
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Not support! %s"), *ObjPathName);
        return EReimportResult::Failed;
    }

    // get the import data
    if (UglTFImporterEdData* glTFImporterEdData = Cast<UglTFImporterEdData>(AssetImportData))
    {
        glTFReimporterOptions = MakeShared<FglTFImporterOptions>();
        *glTFReimporterOptions = glTFImporterEdData->glTFImporterOptions;
    }

    const FString AssetImportFilename = AssetImportData->GetFirstFilename();
    if (!FPaths::FileExists(AssetImportFilename))
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Source file is lost! %s"), *AssetImportFilename);
        return EReimportResult::Failed;
    }

    TArray<uint8> glTFBinary;
    if (!FFileHelper::LoadFileToArray(glTFBinary, *AssetImportFilename))
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to load the file: %s!"), *AssetImportFilename);
        return EReimportResult::Failed;
    }
    /// add `\0` as the end of the file
    glTFBinary.Emplace('\0');

    Super::CurrentFilename = AssetImportFilename;
    const FString BaseFilename = FglTFImporter::SanitizeObjectName(FPaths::GetBaseFilename(AssetImportFilename));
    const uint8* glTFBinaryBuffer = &glTFBinary[0];
    UObject* RenewObject = Super::FactoryCreateBinary(Obj->GetClass(), Obj->GetOuter(), FName(*BaseFilename), Obj->GetFlags(), Obj, nullptr, glTFBinaryBuffer, &glTFBinary.Last(), nullptr);
    if (RenewObject != Obj)
    {
        return EReimportResult::Failed;
    }
    return EReimportResult::Succeeded;
}

int32 UglTFBinaryReimportFactory::GetPriority() const
{
    return ImportPriority;
}

bool UglTFBinaryReimportFactory::FactoryCanImport(const FString& Filename)
{
    return false;
}

UglTFBinarySkeletalMeshReimportFactory::UglTFBinarySkeletalMeshReimportFactory(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    SupportedClass = USkeletalMesh::StaticClass();
}
