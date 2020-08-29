// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEd.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporterEdStaticMesh.h"
#include "glTF/glTFImporterEdSkeletalMesh.h"
#include "glTF/glTFImporterEdLevel.h"

#include <EditorFramework/AssetImportData.h>

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

FglTFImporterEd::FglTFMaterialInfo::FglTFMaterialInfo(int32 InId, FString InPrimitiveName)
    : Id(InId)
    , PrimitiveName(InPrimitiveName)
{
    //
}

TSharedPtr<FglTFImporterEd> FglTFImporterEd::Get(UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEd> glTFImporterEd = MakeShareable(new FglTFImporterEd);
    glTFImporterEd->Set(InClass, InParent, InName, InFlags, InFeedbackContext);
    glTFImporterEd->InputFactory = InFactory;
    return glTFImporterEd;
}

FglTFImporterEd::FglTFImporterEd()
    : Super()
    , InputFactory(nullptr)
{
    //
}

FglTFImporterEd::~FglTFImporterEd()
{
    //
}

UObject* FglTFImporterEd::Create(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InglTFBuffers) const
{
    if (!InGlTF)
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Invalid InGlTF!"));
        return nullptr;
    }

    if (!InGlTF->asset || InGlTF->asset->version != GLTF_TCHAR_TO_GLTFSTRING(TEXT("2.0")))
    {
        const FString AssetVersion = (InGlTF->asset != nullptr) ? GLTF_GLTFSTRING_TO_TCHAR(InGlTF->asset->version.c_str()) : TEXT("none");
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Invalid version: %s!"), *AssetVersion);
        return nullptr;
    }

    const TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();

    FlushRenderingCommands();

    std::vector<std::shared_ptr<libgltf::SScene>> Scenes;
    if (InGlTF->scene)
    {
        Scenes.push_back(InGlTF->scenes[(int32)(*InGlTF->scene)]);
    }
    else if (InGlTF->scenes.size() > 0)
    {
        for (const std::shared_ptr<libgltf::SScene>& Scene : InGlTF->scenes)
        {
            Scenes.push_back(Scene);
        }
    }

    UObject* CreatedObject = nullptr;
    switch (glTFImporterOptions->ImportType)
    {
    case EglTFImportType::StaticMesh:
        CreatedObject = FglTFImporterEdStaticMesh::Get(InputFactory, InputClass, InputParent, InputName, InputFlags, FeedbackContext)->CreateStaticMesh(InglTFImporterOptions, InGlTF, Scenes, InglTFBuffers);
        break;

    case EglTFImportType::SkeletalMesh:
        CreatedObject = FglTFImporterEdSkeletalMesh::Get(InputFactory, InputClass, InputParent, InputName, InputFlags, FeedbackContext)->CreateSkeletalMesh(InglTFImporterOptions, InGlTF, Scenes, InglTFBuffers);
        break;

    case EglTFImportType::Level:
        CreatedObject = FglTFImporterEdLevel::Get(InputFactory, InputClass, InputParent, InputName, InputFlags, FeedbackContext)->CreateLevel(InglTFImporterOptions, InGlTF, Scenes, InglTFBuffers);
        break;

    default:
        break;
    }

    FglTFImporterEd::UpdateAssetImportData(CreatedObject, glTFImporterOptions->FilePathInOS);
    return CreatedObject;
}

UAssetImportData* FglTFImporterEd::GetAssetImportData(UObject* InObject)
{
    UAssetImportData* AssetImportData = nullptr;
    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(InObject))
    {
        AssetImportData = StaticMesh->AssetImportData;
    }
    else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(InObject))
    {
        AssetImportData = SkeletalMesh->AssetImportData;
    }
    else if (UTexture* Texture = Cast<UTexture>(InObject))
    {
        AssetImportData = Texture->AssetImportData;
    }
    return AssetImportData;
}

void FglTFImporterEd::UpdateAssetImportData(UObject* InObject, const FString& InFilePath)
{
    if (!InFilePath.IsEmpty())
    {
        if (UAssetImportData* AssetImportData = GetAssetImportData(InObject))
        {
            AssetImportData->Update(InFilePath);
        }
    }

    if (InObject)
    {
        InObject->PostEditChange();
        InObject->MarkPackageDirty();
    }
}

#undef LOCTEXT_NAMESPACE
