// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEd.h"

#include "glTF/glTFImporterEdStaticMesh.h"
#include "glTF/glTFImporterEdSkeletalMesh.h"
#include "glTF/glTFImporterEdLevel.h"

#include <EditorFramework/AssetImportData.h>

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

TSharedPtr<FglTFImporterEd> FglTFImporterEd::Get(UFactory* InFactory, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEd> glTFImporterEd = MakeShareable(new FglTFImporterEd);
    glTFImporterEd->Set(InParent, InName, InFlags, InFeedbackContext);
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
    check(glTFImporterOptions->Details);

    FlushRenderingCommands();

    std::vector<std::shared_ptr<libgltf::SScene>> Scenes;
    if (!glTFImporterOptions->Details->bImportAllScene && InGlTF->scene)
    {
        Scenes.push_back(InGlTF->scenes[(int32)(*InGlTF->scene)]);
    }
    else if (InGlTF->scenes.size() > 0)
    {
        Scenes = InGlTF->scenes;
    }

    if (Scenes.empty())
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("No scene!"));
        return nullptr;
    }

    FglTFImporterCollection glTFImporterCollection;
    if (!FglTFImporter::GetNodeInfos(InGlTF, glTFImporterCollection.NodeInfos))
    {
        //TODO: print a message
        return nullptr;
    }

    // scale the transform
    if (glTFImporterOptions->Details->MeshScaleRatio != 1.0f)
    {
        const FVector ScaleVector(glTFImporterOptions->Details->MeshScaleRatio);
        for (TPair<int32, FglTFImporterNodeInfo>& NodeInfo : glTFImporterCollection.NodeInfos)
        {
            FglTFImporterNodeInfo& NodeInfoValue = NodeInfo.Value;
            FTransform& NodeAbsoluteTransform = NodeInfoValue.AbsoluteTransform;
            NodeAbsoluteTransform.MultiplyScale3D(ScaleVector);
            NodeAbsoluteTransform.ScaleTranslation(glTFImporterOptions->Details->MeshScaleRatio);
        }
    }

    UObject* CreatedObject = nullptr;
    for (const std::shared_ptr<libgltf::SScene>& ScenePtr : Scenes)
    {
        UObject* ObjectNode = CreateNodes(InglTFImporterOptions, InGlTF, ScenePtr->nodes, InglTFBuffers, glTFImporterCollection);
        if (!CreatedObject) CreatedObject = ObjectNode;
    }

    return CreatedObject;
}

UObject* FglTFImporterEd::CreateNodes(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::vector<std::shared_ptr<libgltf::SGlTFId>>& InNodeIdPtrs, const FglTFBuffers& InglTFBuffers
    , FglTFImporterCollection& InOutglTFImporterCollection) const
{
    UObject* CreatedObject = nullptr;
    for (const std::shared_ptr<libgltf::SGlTFId>& NodeIdPtr : InNodeIdPtrs)
    {
        UObject* ObjectNode = CreateNode(InglTFImporterOptions, InGlTF, NodeIdPtr, InglTFBuffers, InOutglTFImporterCollection);
        if (!CreatedObject) CreatedObject = ObjectNode;
    }
    return CreatedObject;
}

UObject* FglTFImporterEd::CreateNode(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SGlTFId>& InNodeIdPtr, const FglTFBuffers& InglTFBuffers
    , FglTFImporterCollection& InOutglTFImporterCollection) const
{
    if (!InNodeIdPtr) return nullptr;
    const int32 NodeId = *InNodeIdPtr;
    if (NodeId < 0 || NodeId >= static_cast<int32>(InGlTF->nodes.size())) return nullptr;
    const std::shared_ptr<libgltf::SNode>& NodePtr = InGlTF->nodes[NodeId];
    if (!NodePtr) return nullptr;

    const TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();
    check(glTFImporterOptions->Details);

    const FTransform NodeTransform = glTFImporterOptions->Details->bUseAbsolateTransform
        ? InOutglTFImporterCollection.FindNodeInfo(NodeId).AbsoluteTransform
        : FTransform::Identity;

    TArray<UObject*> CreatedObjects;
    if (!!(NodePtr->mesh))
    {
        if (!glTFImporterOptions->Details->bImportSkeletalMesh || !NodePtr->skin)
        {
            UStaticMesh* NewStaticMesh = FglTFImporterEdStaticMesh::Get(InputFactory, InputParent, InputName, InputFlags, FeedbackContext)
                ->CreateStaticMesh(InglTFImporterOptions, InGlTF, NodePtr->mesh
                    , NodeTransform, InglTFBuffers, InOutglTFImporterCollection);
            FglTFImporterEd::UpdateAssetImportData(NewStaticMesh, InglTFImporterOptions);
            CreatedObjects.Emplace(NewStaticMesh);
        }
        else
        {
            USkeletalMesh* SkeletalMesh = FglTFImporterEdSkeletalMesh::Get(InputFactory, InputParent, InputName, InputFlags, FeedbackContext)
                ->CreateSkeletalMesh(InglTFImporterOptions, InGlTF, NodePtr->mesh, NodePtr->skin
                    , NodeTransform, InglTFBuffers, InOutglTFImporterCollection);
            FglTFImporterEd::UpdateAssetImportData(SkeletalMesh, InglTFImporterOptions);
            CreatedObjects.Emplace(SkeletalMesh);
        }
    }
    if (!!(NodePtr->camera))
    {
        //TODO:
    }

    if (!NodePtr->children.empty())
    {
        UObject* ObjectChlid = CreateNodes(InglTFImporterOptions, InGlTF, NodePtr->children, InglTFBuffers, InOutglTFImporterCollection);
        CreatedObjects.Emplace(ObjectChlid);
    }
    return ((CreatedObjects.Num() > 0) ? CreatedObjects[0] : nullptr);
}

bool FglTFImporterEd::SetAssetImportData(UObject* InObject, const FglTFImporterOptions& InglTFImporterOptions)
{
    if (!InObject) return false;
    // just supports `UStaticMesh` and `USkeletalMesh`
    if (!InObject->IsA<UStaticMesh>() && !InObject->IsA<USkeletalMesh>())
    {
        return false;
    }

    UglTFImporterEdData* glTFImporterEdData = Cast<UglTFImporterEdData>(GetAssetImportData(InObject));
    if (!glTFImporterEdData)
    {
        glTFImporterEdData = NewObject<UglTFImporterEdData>(InObject);
    }
    glTFImporterEdData->glTFImporterOptions = InglTFImporterOptions;
    glTFImporterEdData->glTFImporterOptions.FilePathInEngine = InObject->GetPathName();
    glTFImporterEdData->Update(InglTFImporterOptions.FilePathInOS);

    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(InObject))
    {
        StaticMesh->AssetImportData = glTFImporterEdData;
    }
    else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(InObject))
    {
        SkeletalMesh->AssetImportData = glTFImporterEdData;
    }
    if (glTFImporterEdData)
    {
        glTFImporterEdData->MarkPackageDirty();
    }
    return true;
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

void FglTFImporterEd::UpdateAssetImportData(UObject* InObject, const FString& InFilePathInOS)
{
    if (!InFilePathInOS.IsEmpty())
    {
        if (UAssetImportData* AssetImportData = GetAssetImportData(InObject))
        {
            AssetImportData->Update(InFilePathInOS);
        }
    }

    if (InObject)
    {
        InObject->PostEditChange();
        InObject->MarkPackageDirty();
    }
}

void FglTFImporterEd::UpdateAssetImportData(UObject* InObject, const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions)
{
    if (!InglTFImporterOptions.IsValid())
    {
        return;
    }

    const TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();

    if (!glTFImporterOptions->FilePathInOS.IsEmpty())
    {
        SetAssetImportData(InObject, *glTFImporterOptions);
    }

    if (InObject)
    {
        InObject->PostEditChange();
        InObject->MarkPackageDirty();
    }
}

UglTFImporterEdData::UglTFImporterEdData(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    //
}

#undef LOCTEXT_NAMESPACE
