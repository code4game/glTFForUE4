// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTFFactory.h"

#include "glTF/glTFImportOptions.h"
#include "glTF/glTFImportOptionsWindowEd.h"
#include "glTF/glTFImporterEd.h"

#include "libgltf/libgltf.h"

#include "Misc/FeedbackContext.h"
#include "Misc/Paths.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"

#include "ObjectTools.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4EdModule"

UglTFFactory::UglTFFactory()
    : Super()
    , ImportClass(nullptr)
{
    //
}

UglTFFactory::UglTFFactory(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
    , ImportClass(nullptr)
{
    if (Formats.Num() > 0) Formats.Empty();
    Formats.Add(TEXT("gltf;glTF 2.0"));

    bCreateNew = false;
    bText = true;
    bEditorImport = true;
}

bool UglTFFactory::DoesSupportClass(UClass* InClass)
{
    return (InClass == UStaticMesh::StaticClass()
        || InClass == USkeletalMesh::StaticClass());
}

UClass* UglTFFactory::ResolveSupportedClass()
{
    if (ImportClass == nullptr) ImportClass = UStaticMesh::StaticClass();
    return ImportClass;
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

UObject* UglTFFactory::FactoryCreate(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, const TCHAR* InType, FFeedbackContext* InWarn, const FString& InglTFJson, TSharedPtr<FglTFBuffers> InglTFBuffers /*= nullptr*/)
{
    const FString& FilePathInOS = UFactory::GetCurrentFilename();
    if (!ObjectTools::SanitizeObjectName(FPaths::GetBaseFilename(FilePathInOS)).Equals(InName.ToString()))
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
    if (glTFImportOptions.IsValid())
    {
        if (glTFImportOptions->ImportType == EglTFImportType::StaticMesh)
        {
            ImportClass = UStaticMesh::StaticClass();
        }
        else if (glTFImportOptions->ImportType == EglTFImportType::SkeletalMesh)
        {
            ImportClass = USkeletalMesh::StaticClass();
        }
        else if (glTFImportOptions->ImportType == EglTFImportType::Actor)
        {
            ImportClass = AActor::StaticClass();
        }
        else if (glTFImportOptions->ImportType == EglTFImportType::Level)
        {
            ImportClass = ULevel::StaticClass();
        }
        else
        {
            //TODO:
            ImportClass = UStaticMesh::StaticClass();
        }
    }
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

    if (!InglTFBuffers.IsValid())
    {
        InglTFBuffers = MakeShareable(new FglTFBuffers);
    }
    const FString FolderPathInOS = FPaths::GetPath(glTFImportOptions->FilePathInOS);
    InglTFBuffers->Cache(FolderPathInOS, GlTF);

    return FglTFImporterEd::Get(this, ImportClass, InParent, InName, InFlags, InWarn)->Create(glTFImportOptions, GlTF, *InglTFBuffers);
}

#undef LOCTEXT_NAMESPACE
