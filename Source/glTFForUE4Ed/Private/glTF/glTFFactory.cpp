// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTFFactory.h"

#include "glTF/glTFImportOptions.h"
#include "glTF/glTFImportOptionsWindowEd.h"
#include "glTF/glTFImporterEd.h"

#include "libgltf/libgltf.h"

#include "Misc/Paths.h"
#include "Engine/StaticMesh.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4EdModule"

UglTFFactory::UglTFFactory(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    SupportedClass = UStaticMesh::StaticClass();
    if (Formats.Num() > 0) Formats.Empty();
    Formats.Add(TEXT("gltf;glTF 2.0"));

    bCreateNew = false;
    bText = true;
    bEditorImport = true;
}

bool UglTFFactory::DoesSupportClass(UClass* InClass)
{
    //TODO: support more mesh classes
    return (InClass == UStaticMesh::StaticClass());
}

bool UglTFFactory::FactoryCanImport(const FString& InFilePathInOS)
{
    return FPaths::GetExtension(InFilePathInOS).Equals(TEXT("gltf"), ESearchCase::IgnoreCase);
}

UObject* UglTFFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, const TCHAR* InType, const TCHAR*& InBuffer, const TCHAR* InBufferEnd, FFeedbackContext* InWarn)
{
    if (!InBuffer || !InBufferEnd || InBuffer >= InBufferEnd)
    {
        InWarn->Log(ELogVerbosity::Error, FText::Format(NSLOCTEXT("glTFForUE4Ed", "BufferHasErrorInFactoryCreateText", "Buffer has some errors when create the glTF file {0}"), FText::FromName(InName)).ToString());
        return nullptr;
    }

    uint64 BufferSize = InBufferEnd - InBuffer;

    FString glTFJson;
    glTFJson.Append(InBuffer, BufferSize);
    return FactoryCreate(InClass, InParent, InName, InFlags, InContext, InType, InWarn, glTFJson);
}

UObject* UglTFFactory::FactoryCreate(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, const TCHAR* InType, FFeedbackContext* InWarn, const FString& InglTFJson, const TSharedPtr<FglTFBufferBinaries>& InglTFBufferBinaries /*= nullptr*/)
{
    const FString& FilePathInOS = UFactory::GetCurrentFilename();
    if (!FPaths::GetBaseFilename(FilePathInOS).Equals(InName.ToString()))
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("It is different between current filename(%s) and name(%s)!!"), *FilePathInOS, *InName.ToString());
        return nullptr;
    }

    /// Parse and check the buffer
    std::shared_ptr<libgltf::SGlTF> GlTF;
    std::wstring GlTFString = *InglTFJson;
    if (!(GlTF << GlTFString))
    {
        InWarn->Log(ELogVerbosity::Error, FText::Format(NSLOCTEXT("glTFForUE4Ed", "FailedToParseTheglTFFile", "Failed to parse the glTF file {0}"), FText::FromName(InName)).ToString());
        return nullptr;
    }

    /// Open import window, allow to configure some options
    bool bCancel = false;
    TSharedPtr<FglTFImportOptions> glTFImportOptions = SglTFImportOptionsWindowEd::Open(FilePathInOS, InParent->GetPathName(), *GlTF, bCancel);
    if (bCancel)
    {
        UE_LOG(LogglTFForUE4Ed, Display, TEXT("Cancel to import the file - %s"), *FilePathInOS);
        return nullptr;
    }
    if (!glTFImportOptions.IsValid())
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to open import window"));
        return nullptr;
    }

    const FString FolderPathInOS = FPaths::GetPath(glTFImportOptions->FilePathInOS);
    FglTFBuffers glTFBuffers;
    glTFBuffers.Binaries = MakeShareable(new FglTFBufferBinaries(InglTFBufferBinaries));
    glTFBuffers.Files = MakeShareable(new FglTFBufferFiles(FolderPathInOS, GlTF->buffers));

    return FglTFImporterEd::Get(this, InClass, InParent, InName, InFlags, InWarn)->Create(glTFImportOptions, GlTF, glTFBuffers);
}

#undef LOCTEXT_NAMESPACE
