// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTFFactory.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporterOptionsWindowEd.h"
#include "glTF/glTFImporterEd.h"

#include <Engine/StaticMesh.h>
#include <Engine/SkeletalMesh.h>
#include <Misc/FeedbackContext.h>
#include <Misc/Paths.h>

#include <ObjectTools.h>

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

UglTFFactory::UglTFFactory(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
    , glTFReimporterOptions(nullptr)
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
    return UStaticMesh::StaticClass();
}

bool UglTFFactory::FactoryCanImport(const FString& InFilePathInOS)
{
    return FPaths::GetExtension(InFilePathInOS).Equals(TEXT("gltf"), ESearchCase::IgnoreCase);
}

UObject* UglTFFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, const TCHAR* InType, const TCHAR*& InBuffer, const TCHAR* InBufferEnd, FFeedbackContext* InWarn)
{
    if (!InBuffer || !InBufferEnd || InBuffer >= InBufferEnd)
    {
        if (InWarn)
        {
            InWarn->Log(ELogVerbosity::Error, FText::Format(NSLOCTEXT("glTFForUE4Ed", "BufferHasErrorInFactoryCreateText", "Buffer has some errors when create the glTF file {0}"), FText::FromName(InName)).ToString());
        }
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

    const FText TaskName = FText::Format(LOCTEXT("ImportAglTFFile", "Import a glTF file - {0}"), FText::FromName(InName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(InWarn, TaskName, true);

    /// Parse and check the buffer
    std::shared_ptr<libgltf::SGlTF> GlTF;
    const GLTFString GlTFString = GLTF_TCHAR_TO_GLTFSTRING(*InglTFJson);
    if (!(GlTF << GlTFString))
    {
        FeedbackTaskWrapper.Log(ELogVerbosity::Error, FText::Format(LOCTEXT("FailedToParseTheglTFFile", "Failed to parse the glTF file {0}"), FText::FromName(InName)));
        return nullptr;
    }

    /// Open the importer window, allow to configure some options when is not automated
    bool bCancel = false;
    TSharedPtr<FglTFImporterOptions> glTFImporterOptions = glTFReimporterOptions;
    if (!glTFImporterOptions)
    {
        glTFImporterOptions = IsAutomatedImport()
            ? MakeShared<FglTFImporterOptions>()
            : SglTFImporterOptionsWindowEd::Open(InContext, FilePathInOS, InParent->GetPathName(), *GlTF, bCancel);
    }
    if (glTFImporterOptions.IsValid() && !glTFImporterOptions->Details)
    {
        glTFImporterOptions->Details = GetMutableDefault<UglTFImporterOptionsDetails>();
    }

    if (bCancel)
    {
        UE_LOG(LogglTFForUE4Ed, Display, TEXT("Cancel to import the file - %s"), *FilePathInOS);
        return nullptr;
    }

    if (!glTFImporterOptions.IsValid())
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to open import window"));
        return nullptr;
    }

    if (!InglTFBuffers.IsValid())
    {
        InglTFBuffers = MakeShared<FglTFBuffers>();
    }

    const FString FolderPathInOS = FPaths::GetPath(glTFImporterOptions->FilePathInOS);
    InglTFBuffers->Cache(FolderPathInOS, GlTF);

    return FglTFImporterEd::Get(this, InParent, InName, InFlags, InWarn)->Create(glTFImporterOptions, GlTF, *InglTFBuffers, FeedbackTaskWrapper);
}

#undef LOCTEXT_NAMESPACE
