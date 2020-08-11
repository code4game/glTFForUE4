// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEd.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporterEdStaticMesh.h"
#include "glTF/glTFImporterEdSkeletalMesh.h"
#include "glTF/glTFImporterEdLevel.h"

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

    switch (glTFImporterOptions->ImportType)
    {
    case EglTFImportType::StaticMesh:
        return FglTFImporterEdStaticMesh::Get(InputFactory, InputClass, InputParent, InputName, InputFlags, FeedbackContext)->CreateStaticMesh(InglTFImporterOptions, InGlTF, Scenes, InglTFBuffers);

    case EglTFImportType::SkeletalMesh:
        return FglTFImporterEdSkeletalMesh::Get(InputFactory, InputClass, InputParent, InputName, InputFlags, FeedbackContext)->CreateSkeletalMesh(InglTFImporterOptions, InGlTF, Scenes, InglTFBuffers);

    case EglTFImportType::Level:
        return FglTFImporterEdLevel::Get(InputFactory, InputClass, InputParent, InputName, InputFlags, FeedbackContext)->CreateLevel(InglTFImporterOptions, InGlTF, Scenes, InglTFBuffers);

    default:
        break;
    }
    return nullptr;
}

#undef LOCTEXT_NAMESPACE
