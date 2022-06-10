// Copyright(c) 2016 - 2022 Code 4 Game, Org. All Rights Reserved.

#include "glTF/glTFImporterEd.h"
#include "glTFForUE4EdPrivatePCH.h"

#include "glTF/glTFImporterEdSkeletalMesh.h"
#include "glTF/glTFImporterEdStaticMesh.h"

#include <EditorFramework/AssetImportData.h>

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

TSharedPtr<FglTFImporterEd>
FglTFImporterEd::Get(UFactory* InFactory, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext)
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

UObject* FglTFImporterEd::Create(const TWeakPtr<FglTFImporterOptions>&   InglTFImporterOptions,
                                 const std::shared_ptr<libgltf::SGlTF>&  InGlTF,
                                 const FglTFBuffers&                     InglTFBuffers,
                                 const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
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
        // TODO: print a message
        return nullptr;
    }

    /// make sure the target world
    if (glTFImporterOptions->Details->bImportLevel)
    {
        if (glTFImporterOptions->Details->ImportLevelTemplate.IsValid())
        {
            /// create a world by a template
            UPackage* NewAssetPackage = LoadPackage(nullptr, *glTFImporterOptions->FilePathInEngine, LOAD_None);
            if (!NewAssetPackage)
            {
#if GLTFFORUE_ENGINE_VERSION < 426
                NewAssetPackage = CreatePackage(nullptr, *glTFImporterOptions->FilePathInEngine);
#else
                NewAssetPackage = CreatePackage(*glTFImporterOptions->FilePathInEngine);
#endif
                checkSlow(NewAssetPackage);
                glTFImporterCollection.TargetWorld = Cast<UWorld>(StaticDuplicateObject(
                    glTFImporterOptions->Details->ImportLevelTemplate.TryLoad(), NewAssetPackage, InputName, InputFlags, UWorld::StaticClass()));
            }
            else
            {
                /// don't spawn the actor in the exist level
                glTFImporterOptions->Details->bImportLevel = false;
            }
        }
        else
        {
            glTFImporterCollection.TargetWorld = GEditor ? GEditor->GetEditorWorldContext(false).World() : nullptr;
        }
    }
    else
    {
        glTFImporterCollection.TargetWorld = nullptr;
    }

    // scale the transform
    if (glTFImporterOptions->Details->MeshScaleRatio != 1.0f)
    {
        const FVector    ScaleVector(glTFImporterOptions->Details->MeshScaleRatio);
        const FTransform ScaleTransform(FQuat::Identity, FVector::ZeroVector, ScaleVector);
        for (TPair<int32, FglTFImporterNodeInfo>& NodeInfo : glTFImporterCollection.NodeInfos)
        {
            FglTFImporterNodeInfo& NodeInfoValue = NodeInfo.Value;
            if (glTFImporterOptions->Details->bApplyAbsoluteTransform)
            {
                NodeInfoValue.AbsoluteTransform *= ScaleTransform;
            }
            else
            {
                NodeInfoValue.AbsoluteTransform.ScaleTranslation(ScaleVector);
            }
        }
    }

    UObject* CreatedObject = nullptr;
    for (const std::shared_ptr<libgltf::SScene>& ScenePtr : Scenes)
    {
        UObject* ObjectNode = CreateNodes(InglTFImporterOptions, InGlTF, ScenePtr->nodes, InglTFBuffers, glTFImporterCollection);
        if (!CreatedObject)
            CreatedObject = ObjectNode;
    }

    return CreatedObject;
}

UObject* FglTFImporterEd::CreateNodes(const TWeakPtr<FglTFImporterOptions>&                 InglTFImporterOptions,
                                      const std::shared_ptr<libgltf::SGlTF>&                InGlTF,
                                      const std::vector<std::shared_ptr<libgltf::SGlTFId>>& InNodeIdPtrs,
                                      const FglTFBuffers&                                   InglTFBuffers,
                                      FglTFImporterCollection&                              InOutglTFImporterCollection) const
{
    UObject* CreatedObject = nullptr;
    for (const std::shared_ptr<libgltf::SGlTFId>& NodeIdPtr : InNodeIdPtrs)
    {
        UObject* ObjectNode = CreateNode(InglTFImporterOptions, InGlTF, NodeIdPtr, InglTFBuffers, InOutglTFImporterCollection);
        if (!CreatedObject)
            CreatedObject = ObjectNode;
    }
    return CreatedObject;
}

UObject* FglTFImporterEd::CreateNode(const TWeakPtr<FglTFImporterOptions>&    InglTFImporterOptions,
                                     const std::shared_ptr<libgltf::SGlTF>&   InGlTF,
                                     const std::shared_ptr<libgltf::SGlTFId>& InNodeIdPtr,
                                     const FglTFBuffers&                      InglTFBuffers,
                                     FglTFImporterCollection&                 InOutglTFImporterCollection) const
{
    if (!InNodeIdPtr)
        return nullptr;
    const int32 glTFNodeId = *InNodeIdPtr;
    if (glTFNodeId < 0 || glTFNodeId >= static_cast<int32>(InGlTF->nodes.size()))
        return nullptr;
    const std::shared_ptr<libgltf::SNode>& glTFNodePtr = InGlTF->nodes[glTFNodeId];
    if (!glTFNodePtr)
        return nullptr;

    const TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();
    check(glTFImporterOptions->Details);

    const FglTFImporterNodeInfo& NodeInfo = InOutglTFImporterCollection.FindNodeInfo(glTFNodeId);
    const FVector                ScaleVector(glTFImporterOptions->Details->MeshScaleRatio);
    const FTransform             ScaleTransform(FQuat::Identity, FVector::ZeroVector, ScaleVector);
    const FTransform             TransformMesh = glTFImporterOptions->Details->bApplyAbsoluteTransform ? NodeInfo.AbsoluteTransform : ScaleTransform;
    const FTransform TransformActor = glTFImporterOptions->Details->bApplyAbsoluteTransform ? FTransform::Identity : NodeInfo.AbsoluteTransform;

    TArray<UObject*> CreatedObjects;

    std::shared_ptr<libgltf::SMesh> glTFMeshPtr;
    if (glTFNodePtr->mesh)
    {
        const int32_t glTFMeshId = *glTFNodePtr->mesh;
        if (glTFMeshId >= 0 && glTFMeshId < InGlTF->meshes.size())
        {
            glTFMeshPtr = InGlTF->meshes[glTFMeshId];
        }
        else
        {
            UE_LOG(LogglTFForUE4Ed, Error, TEXT("The mesh index is invalid!"));
        }
    }
    if (glTFMeshPtr)
    {
        if (glTFImporterOptions->Details->bImportSkeletalMesh &&
            (glTFNodePtr->skin || (glTFImporterOptions->Details->bImportMorphTarget && !glTFMeshPtr->weights.empty())))
        {
            USkeletalMesh* NewSkeletalMesh = FglTFImporterEdSkeletalMesh::Get(InputFactory, InputParent, InputName, InputFlags, FeedbackContext)
                                                 ->CreateSkeletalMesh(InglTFImporterOptions,
                                                                      InGlTF,
                                                                      glTFNodeId,
                                                                      glTFNodePtr->mesh,
                                                                      glTFNodePtr->skin,
                                                                      InglTFBuffers,
                                                                      TransformMesh,
                                                                      InOutglTFImporterCollection);
            FglTFImporterEd::UpdateAssetImportData(NewSkeletalMesh, InglTFImporterOptions);
            CreatedObjects.Emplace(NewSkeletalMesh);
            if (glTFImporterOptions->Details->bImportLevel)
            {
                SpawnSkeletalMeshActor(InOutglTFImporterCollection.TargetWorld, TransformActor, InputFlags, NewSkeletalMesh);
            }
        }
        else if (glTFImporterOptions->Details->bImportStaticMesh)
        {
            UStaticMesh* NewStaticMesh =
                FglTFImporterEdStaticMesh::Get(InputFactory, InputParent, InputName, InputFlags, FeedbackContext)
                    ->CreateStaticMesh(InglTFImporterOptions, InGlTF, glTFNodePtr->mesh, InglTFBuffers, TransformMesh, InOutglTFImporterCollection);
            FglTFImporterEd::UpdateAssetImportData(NewStaticMesh, InglTFImporterOptions);
            CreatedObjects.Emplace(NewStaticMesh);
            if (glTFImporterOptions->Details->bImportLevel)
            {
                SpawnStaticMeshActor(InOutglTFImporterCollection.TargetWorld, TransformActor, InputFlags, NewStaticMesh);
            }
        }
    }

    if (glTFImporterOptions->Details->bImportLevel && glTFImporterOptions->Details->bImportLightInLevel)
    {
        // TODO: spawn a light in the level
    }

    if (glTFImporterOptions->Details->bImportLevel && glTFImporterOptions->Details->bImportCameraInLevel)
    {
        /// spawn a camera in the level
        std::shared_ptr<libgltf::SCamera> glTFCameraPtr;
        if (glTFNodePtr->camera)
        {
            const int32_t glTFCameraId = *glTFNodePtr->mesh;
            if (glTFCameraId >= 0 && glTFCameraId < InGlTF->cameras.size())
            {
                glTFCameraPtr = InGlTF->cameras[glTFCameraId];
            }
        }
        if (glTFCameraPtr)
        {
            // TODO:
        }
    }

    if (!glTFNodePtr->children.empty())
    {
        UObject* ObjectChlid = CreateNodes(InglTFImporterOptions, InGlTF, glTFNodePtr->children, InglTFBuffers, InOutglTFImporterCollection);
        CreatedObjects.Emplace(ObjectChlid);
    }
    return ((CreatedObjects.Num() > 0) ? CreatedObjects[0] : nullptr);
}

bool FglTFImporterEd::SetAssetImportData(UObject* InObject, const FglTFImporterOptions& InglTFImporterOptions)
{
    if (!InObject)
        return false;
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
    glTFImporterEdData->glTFImporterOptions                  = InglTFImporterOptions;
    glTFImporterEdData->glTFImporterOptions.FilePathInEngine = InObject->GetPathName();
#if GLTFFORUE_ENGINE_VERSION < 414
    glTFImporterEdData->Update(InglTFImporterOptions.FilePathInOS);
#else
    glTFImporterEdData->Update(InglTFImporterOptions.FilePathInOS, InglTFImporterOptions.FileHash.Get());
#endif

    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(InObject))
    {
        StaticMesh->AssetImportData = glTFImporterEdData;
    }
    else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(InObject))
    {
#if GLTFFORUE_ENGINE_VERSION < 427
        SkeletalMesh->AssetImportData = glTFImporterEdData;
#else
        SkeletalMesh->SetAssetImportData(glTFImporterEdData);
#endif
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
#if GLTFFORUE_ENGINE_VERSION < 427
        AssetImportData = SkeletalMesh->AssetImportData;
#else
        AssetImportData = SkeletalMesh->GetAssetImportData();
#endif
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
