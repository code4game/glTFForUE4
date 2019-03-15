// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEdSkeletalMesh.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporterEdMaterial.h"
#include "glTF/glTFImporterEdAnimationSequence.h"

#include "SkeletalMeshTypes.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/Paths.h"

#include "SkelImport.h"
#include "MeshUtilities.h"
#include "AssetRegistryModule.h"

#if ENGINE_MINOR_VERSION < 19
#else
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#endif

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

namespace glTFForUE4Ed
{
    bool CheckAndMerge(const FSkeletalMeshImportData& InImportData, const TArray<FMatrix>& InInverseBindMatrix, const TMap<int32, FString>& InNodeIndexToBoneNames
        , FSkeletalMeshImportData& OutImportData, TArray<FMatrix>& OutInverseBindMatrix, TMap<int32, FString>& OutNodeIndexToBoneNames)
    {
        int32 MaterialsStartIndex = OutImportData.Materials.Num();
        int32 PointsStartIndex = OutImportData.Points.Num();
        int32 WedgesStartIndex = OutImportData.Wedges.Num();
        int32 RefBonesBinaryStartIndex = OutImportData.RefBonesBinary.Num();

        OutImportData.Materials.Append(InImportData.Materials);
        OutImportData.Points.Append(InImportData.Points);

#if ENGINE_MINOR_VERSION < 21
        for (VVertex Vertex : InImportData.Wedges)
#else
        for (SkeletalMeshImportData::FVertex Vertex : InImportData.Wedges)
#endif
        {
            Vertex.VertexIndex += PointsStartIndex;
            Vertex.MatIndex += MaterialsStartIndex;
            OutImportData.Wedges.Add(Vertex);
        }

#if ENGINE_MINOR_VERSION < 21
        for (VTriangle Triangle : InImportData.Faces)
#else
        for (SkeletalMeshImportData::FTriangle Triangle : InImportData.Faces)
#endif
        {
            for (uint8 i = 0; i < GLTF_TRIANGLE_POINTS_NUM; ++i)
            {
                Triangle.WedgeIndex[i] += WedgesStartIndex;
            }
            Triangle.MatIndex += MaterialsStartIndex;
            OutImportData.Faces.Add(Triangle);
        }

#if ENGINE_MINOR_VERSION < 21
        for (VRawBoneInfluence RawBoneInfluence : InImportData.Influences)
#else
        for (SkeletalMeshImportData::FRawBoneInfluence RawBoneInfluence : InImportData.Influences)
#endif
        {
            RawBoneInfluence.VertexIndex += PointsStartIndex;
            OutImportData.Influences.Add(RawBoneInfluence);
        }

        for (int32 PointToRawMapIndex : InImportData.PointToRawMap)
        {
            PointToRawMapIndex += PointsStartIndex;
            OutImportData.PointToRawMap.Add(PointToRawMapIndex);
        }

        if (OutImportData.Materials.Num() > 0)
        {
            OutImportData.MaxMaterialIndex = OutImportData.Materials.Num() - 1;
        }

        OutImportData.NumTexCoords = FMath::Max<uint32>(InImportData.NumTexCoords, OutImportData.NumTexCoords);

        OutImportData.bHasVertexColors |= InImportData.bHasVertexColors;
        OutImportData.bHasNormals |= InImportData.bHasNormals;
        OutImportData.bHasTangents |= InImportData.bHasTangents;

#if ENGINE_MINOR_VERSION < 21
        for (VBone Bone : InImportData.RefBonesBinary)
#else
        for (SkeletalMeshImportData::FBone Bone : InImportData.RefBonesBinary)
#endif
        {
            Bone.ParentIndex += RefBonesBinaryStartIndex;
            OutImportData.RefBonesBinary.Add(Bone);
        }
        OutInverseBindMatrix.Append(InInverseBindMatrix);

        for (const TPair<int32, FString>& NodeIndexToBoneName : InNodeIndexToBoneNames)
        {
            OutNodeIndexToBoneNames.FindOrAdd(NodeIndexToBoneName.Key + RefBonesBinaryStartIndex) = NodeIndexToBoneName.Value;
        }

        return true;
    }

    void CopyLODImportData(const FSkeletalMeshImportData& InSkeletalMeshImportData,
        TArray<FVector>& LODPoints,
#if ENGINE_MINOR_VERSION < 21
        TArray<FMeshWedge>& LODWedges,
        TArray<FMeshFace>& LODFaces,
        TArray<FVertInfluence>& LODInfluences,
#else
        TArray<SkeletalMeshImportData::FMeshWedge>& LODWedges,
        TArray<SkeletalMeshImportData::FMeshFace>& LODFaces,
        TArray<SkeletalMeshImportData::FVertInfluence>& LODInfluences,
#endif
        TArray<int32>& LODPointToRawMap)
    {
        // Copy vertex data.
        LODPoints.Empty(InSkeletalMeshImportData.Points.Num());
        LODPoints.AddUninitialized(InSkeletalMeshImportData.Points.Num());
        for (int32 p = 0; p < InSkeletalMeshImportData.Points.Num(); p++)
        {
            LODPoints[p] = InSkeletalMeshImportData.Points[p];
        }

        // Copy wedge information to static LOD level.
        LODWedges.Empty(InSkeletalMeshImportData.Wedges.Num());
        LODWedges.AddUninitialized(InSkeletalMeshImportData.Wedges.Num());
        for (int32 w = 0; w < InSkeletalMeshImportData.Wedges.Num(); w++)
        {
            LODWedges[w].iVertex = InSkeletalMeshImportData.Wedges[w].VertexIndex;
            // Copy all texture coordinates
            FMemory::Memcpy(LODWedges[w].UVs, InSkeletalMeshImportData.Wedges[w].UVs, sizeof(FVector2D) * MAX_TEXCOORDS);
            LODWedges[w].Color = InSkeletalMeshImportData.Wedges[w].Color;

        }

        // Copy triangle/ face data to static LOD level.
        LODFaces.Empty(InSkeletalMeshImportData.Faces.Num());
        LODFaces.AddUninitialized(InSkeletalMeshImportData.Faces.Num());
        for (int32 f = 0; f < InSkeletalMeshImportData.Faces.Num(); f++)
        {
#if ENGINE_MINOR_VERSION < 21
            FMeshFace Face;
#else
            SkeletalMeshImportData::FMeshFace Face;
#endif
            Face.iWedge[0] = InSkeletalMeshImportData.Faces[f].WedgeIndex[0];
            Face.iWedge[1] = InSkeletalMeshImportData.Faces[f].WedgeIndex[1];
            Face.iWedge[2] = InSkeletalMeshImportData.Faces[f].WedgeIndex[2];
            Face.MeshMaterialIndex = InSkeletalMeshImportData.Faces[f].MatIndex;

            Face.TangentX[0] = InSkeletalMeshImportData.Faces[f].TangentX[0];
            Face.TangentX[1] = InSkeletalMeshImportData.Faces[f].TangentX[1];
            Face.TangentX[2] = InSkeletalMeshImportData.Faces[f].TangentX[2];

            Face.TangentY[0] = InSkeletalMeshImportData.Faces[f].TangentY[0];
            Face.TangentY[1] = InSkeletalMeshImportData.Faces[f].TangentY[1];
            Face.TangentY[2] = InSkeletalMeshImportData.Faces[f].TangentY[2];

            Face.TangentZ[0] = InSkeletalMeshImportData.Faces[f].TangentZ[0];
            Face.TangentZ[1] = InSkeletalMeshImportData.Faces[f].TangentZ[1];
            Face.TangentZ[2] = InSkeletalMeshImportData.Faces[f].TangentZ[2];

            LODFaces[f] = Face;
        }

        // Copy weights / influences to static LOD level.
        LODInfluences.Empty(InSkeletalMeshImportData.Influences.Num());
        LODInfluences.AddUninitialized(InSkeletalMeshImportData.Influences.Num());
        for (int32 i = 0; i < InSkeletalMeshImportData.Influences.Num(); i++)
        {
            LODInfluences[i].Weight = InSkeletalMeshImportData.Influences[i].Weight;
            LODInfluences[i].VertIndex = InSkeletalMeshImportData.Influences[i].VertexIndex;
            LODInfluences[i].BoneIndex = InSkeletalMeshImportData.Influences[i].BoneIndex;
        }

        // Copy mapping
        LODPointToRawMap = InSkeletalMeshImportData.PointToRawMap;
    }

    FString FixupBoneName(const FString &InBoneName)
    {
        FString BoneName = InBoneName;

#if ENGINE_MINOR_VERSION < 18
        BoneName.Trim();
        BoneName.TrimTrailing();
#else
        BoneName.TrimStartAndEndInline();
#endif
        BoneName = BoneName.Replace(TEXT(" "), TEXT("-"));

        return BoneName;
    }

    bool ProcessImportMeshSkeleton(const FSkeletalMeshImportData& InImportData, FReferenceSkeleton& OutReferenceSkeleton, int32& OutSkeletalDepth)
    {
#if ENGINE_MINOR_VERSION < 21
        const TArray<VBone>& RefBonesBinary = InImportData.RefBonesBinary;
#else
        const TArray<SkeletalMeshImportData::FBone>& RefBonesBinary = InImportData.RefBonesBinary;
#endif

        // Setup skeletal hierarchy + names structure.
        OutReferenceSkeleton.Empty();

        // Digest bones to the serializable format.
        for (int32 b = 0; b < RefBonesBinary.Num(); b++)
        {
#if ENGINE_MINOR_VERSION < 21
            const VBone& BinaryBone = RefBonesBinary[b];
#else
            const SkeletalMeshImportData::FBone& BinaryBone = RefBonesBinary[b];
#endif
            const FString BoneName = FixupBoneName(BinaryBone.Name);
#if ENGINE_MINOR_VERSION < 12
            const FMeshBoneInfo BoneInfo(FName(*BoneName, FNAME_Add, true), BinaryBone.Name, BinaryBone.ParentIndex);
#else
            const FMeshBoneInfo BoneInfo(FName(*BoneName), BinaryBone.Name, BinaryBone.ParentIndex);
#endif
            const FTransform BoneTransform(BinaryBone.BonePos.Transform);

            if (OutReferenceSkeleton.FindBoneIndex(BoneInfo.Name) != INDEX_NONE)
            {
                return false;
            }

#if ENGINE_MINOR_VERSION < 14
            OutReferenceSkeleton.Add(BoneInfo, BoneTransform);
#else
            FReferenceSkeletonModifier ReferenceSkeletonModifier(OutReferenceSkeleton, nullptr);
            ReferenceSkeletonModifier.Add(BoneInfo, BoneTransform);
#endif
        }

        // Add hierarchy index to each bone and detect max depth.
        OutSkeletalDepth = 0;

        TArray<int32> SkeletalDepths;
        SkeletalDepths.Empty(RefBonesBinary.Num());
        SkeletalDepths.AddZeroed(RefBonesBinary.Num());
        for (int32 b = 0; b < OutReferenceSkeleton.GetNum(); b++)
        {
            int32 Parent = OutReferenceSkeleton.GetParentIndex(b);
            int32 Depth = 1.0f;

            SkeletalDepths[b] = 1.0f;
            if (Parent != INDEX_NONE)
            {
                Depth += SkeletalDepths[Parent];
            }
            if (OutSkeletalDepth < Depth)
            {
                OutSkeletalDepth = Depth;
            }
            SkeletalDepths[b] = Depth;
        }

        return true;
    }
}

TSharedPtr<FglTFImporterEdSkeletalMesh> FglTFImporterEdSkeletalMesh::Get(UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEdSkeletalMesh> glTFImporterEdSkeletalMesh = MakeShareable(new FglTFImporterEdSkeletalMesh);
    glTFImporterEdSkeletalMesh->Set(InClass, InParent, InName, InFlags, InFeedbackContext);
    glTFImporterEdSkeletalMesh->InputFactory = InFactory;
    return glTFImporterEdSkeletalMesh;
}

FglTFImporterEdSkeletalMesh::FglTFImporterEdSkeletalMesh()
{
    //
}

FglTFImporterEdSkeletalMesh::~FglTFImporterEdSkeletalMesh()
{
    //
}

USkeletalMesh* FglTFImporterEdSkeletalMesh::CreateSkeletalMesh(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions
    , const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::vector<std::shared_ptr<libgltf::SScene>>& InScenes
    , const FglTFBuffers& InBuffers) const
{
    if (!InGlTF || InScenes.empty()) return nullptr;
    if (InputClass != USkeletalMesh::StaticClass() || !InputParent || !InputName.IsValid()) return nullptr;

    TArray<int32> NodeParentIndices;
    TArray<FTransform> NodeRelativeTransforms;
    TArray<FTransform> NodeAbsoluteTransforms;
    if (!FglTFImporter::GetNodeParentIndicesAndTransforms(InGlTF, NodeParentIndices, NodeRelativeTransforms, NodeAbsoluteTransforms)) return nullptr;

    TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();
    FString ImportedBaseFilename = FPaths::GetBaseFilename(glTFImporterOptions->FilePathInOS);

    FText TaskName = FText::Format(LOCTEXT("BeginImportAsSkeletalMeshTask", "Importing the glTF ({0}) as a skeletal mesh ({1})"), FText::FromName(InputName), FText::FromName(InputName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, TaskName, true);

    TArray<USkeletalMesh*> SkeletalMeshs;
    for (const auto& ScenePtr : InScenes)
    {
        for (const auto& NodeIdPtr : ScenePtr->nodes)
        {
            if (!NodeIdPtr)
            {
                checkSlow(0);
                continue;
            }
            const int32 NodeId = *NodeIdPtr;
            if (NodeId < 0 || NodeId >= static_cast<int32>(InGlTF->nodes.size()))
            {
                checkSlow(0);
                continue;
            }

            TArray<USkeletalMesh*> NewSkeletalMeshs;
            NewSkeletalMeshs = CreateSkeletalMesh(InglTFImporterOptions, InGlTF, InGlTF->nodes[NodeId], NodeParentIndices, NodeRelativeTransforms, NodeAbsoluteTransforms, InBuffers);
            if (NewSkeletalMeshs.Num() > 0)
            {
                SkeletalMeshs.Append(NewSkeletalMeshs);
            }
            else
            {
                checkSlow(0);
            }
        }
    }
    return ((SkeletalMeshs.Num() > 0) ? SkeletalMeshs[0] : nullptr);
}

TArray<USkeletalMesh*> FglTFImporterEdSkeletalMesh::CreateSkeletalMesh(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions
    , const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SNode>& InNode
    , const TArray<int32>& InNodeParentIndices, const TArray<FTransform>& InNodeRelativeTransforms, const TArray<FTransform>& InNodeAbsoluteTransforms, const FglTFBuffers& InBuffers) const
{
    TArray<USkeletalMesh*> SkeletalMeshs;
    
    if (!InGlTF || !InNode) return SkeletalMeshs;

    if (!!(InNode->mesh) && !!(InNode->skin))
    {
        const int32_t MeshId = *(InNode->mesh);
        if (MeshId < 0 || MeshId >= static_cast<int32_t>(InGlTF->meshes.size())) return SkeletalMeshs;
        const int32_t SkinId = *(InNode->skin);
        if (SkinId < 0 || SkinId >= static_cast<int32_t>(InGlTF->skins.size())) return SkeletalMeshs;
        const auto& Mesh = InGlTF->meshes[MeshId];
        const auto& Skin = InGlTF->skins[SkinId];
        USkeletalMesh* NewSkeletalMeshs = CreateSkeletalMesh(InglTFImporterOptions, InGlTF, MeshId, Mesh, Skin, InNodeParentIndices, InNodeRelativeTransforms, InNodeAbsoluteTransforms, InBuffers);
        if (NewSkeletalMeshs)
        {
            SkeletalMeshs.Add(NewSkeletalMeshs);
        }
        else
        {
            checkSlow(0);
        }
    }

    for (const auto& NodeIdPtr : InNode->children)
    {
        if (!NodeIdPtr)
        {
            checkSlow(0);
            continue;
        }
        const int32 NodeId = *NodeIdPtr;
        if (NodeId < 0 || NodeId >= static_cast<int32>(InGlTF->nodes.size()))
        {
            checkSlow(0);
            continue;
        }
        const auto& NodePtr = InGlTF->nodes[NodeId];
        if (!NodePtr)
        {
            checkSlow(0);
            continue;
        }
        TArray<USkeletalMesh*> NewSkeletalMeshs;
        NewSkeletalMeshs = CreateSkeletalMesh(InglTFImporterOptions, InGlTF, NodePtr, InNodeParentIndices, InNodeRelativeTransforms, InNodeAbsoluteTransforms, InBuffers);
        if (NewSkeletalMeshs.Num() > 0)
        {
            SkeletalMeshs.Append(NewSkeletalMeshs);
        }
        else
        {
            checkSlow(0);
        }
    }
    return SkeletalMeshs;
}

USkeletalMesh* FglTFImporterEdSkeletalMesh::CreateSkeletalMesh(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions
    , const std::shared_ptr<libgltf::SGlTF>& InGlTF, int32 InMeshId, const std::shared_ptr<libgltf::SMesh>& InMesh, const std::shared_ptr<libgltf::SSkin>& InSkin
    , const TArray<int32>& InNodeParentIndices, const TArray<FTransform>& InNodeRelativeTransforms, const TArray<FTransform>& InNodeAbsoluteTransforms, const FglTFBuffers& InBuffers) const
{
    FText TaskName = FText::Format(LOCTEXT("BeginImportAsSkeletalMeshTask", "Importing the glTF ({0}) as a skeletal mesh ({1})"), FText::FromName(InputName), FText::FromName(InputName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, TaskName, true);

    FSkeletalMeshImportData SkeletalMeshImportData;
    TArray<FMatrix> RefBasesInvMatrix;
    TMap<int32, FString> NodeIndexToBoneNames;
    TArray<FglTFMaterialInfo> MaterialInfos;
    if (!GenerateSkeletalMeshImportData(InGlTF, InMesh, InSkin, InNodeParentIndices, InNodeRelativeTransforms, InNodeAbsoluteTransforms, InBuffers, SkeletalMeshImportData, RefBasesInvMatrix, NodeIndexToBoneNames, MaterialInfos, FeedbackTaskWrapper))
    {
        checkSlow(0);
        return nullptr;
    }

    TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();

    /// Create new static mesh
    FString MeshName(InMesh->name.c_str());
    if (MeshName.IsEmpty())
    {
        MeshName = FString::Printf(TEXT("Id%d"), InMeshId);
    }
    FName SkeletalMeshName(*FString::Printf(TEXT("%s_%s"), *InputName.ToString(), *MeshName));
    USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>(InputParent, InputClass, SkeletalMeshName, InputFlags);
    checkSlow(SkeletalMesh);
    if (!SkeletalMesh) return nullptr;
    FAssetRegistryModule::AssetCreated(SkeletalMesh);

    SkeletalMesh->PreEditChange(nullptr);

    SkeletalMesh->RefBasesInvMatrix = RefBasesInvMatrix;

    FScaleMatrix ScaleMatrix(glTFImporterOptions->MeshScaleRatio);
#if ENGINE_MINOR_VERSION < 21
    for (VBone& Bone : SkeletalMeshImportData.RefBonesBinary)
#else
    for (SkeletalMeshImportData::FBone& Bone : SkeletalMeshImportData.RefBonesBinary)
#endif
    {
        if (Bone.ParentIndex != INDEX_NONE) continue;
        FTransform& RootTransform = Bone.BonePos.Transform;
        RootTransform.SetFromMatrix(RootTransform.ToMatrixWithScale() * ScaleMatrix);
    }

    TArray<FVector> LODPoints;
#if ENGINE_MINOR_VERSION < 21
    TArray<FMeshWedge> LODWedges;
    TArray<FMeshFace> LODFaces;
    TArray<FVertInfluence> LODInfluences;
#else
    TArray<SkeletalMeshImportData::FMeshWedge> LODWedges;
    TArray<SkeletalMeshImportData::FMeshFace> LODFaces;
    TArray<SkeletalMeshImportData::FVertInfluence> LODInfluences;
#endif
    TArray<int32> LODPointToRawMap;
    glTFForUE4Ed::CopyLODImportData(SkeletalMeshImportData, LODPoints, LODWedges, LODFaces, LODInfluences, LODPointToRawMap);

    const bool bShouldComputeNormals = glTFImporterOptions->bRecomputeNormals || !SkeletalMeshImportData.bHasNormals;
    const bool bShouldComputeTangents = glTFImporterOptions->bRecomputeTangents || !SkeletalMeshImportData.bHasTangents;

    IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");

#if ENGINE_MINOR_VERSION < 19
    FSkeletalMeshResource* ImportedResource = SkeletalMesh->GetImportedResource();
#else
    FSkeletalMeshModel* ImportedResource = SkeletalMesh->GetImportedModel();
#endif
    check(ImportedResource->LODModels.Num() == 0);
    ImportedResource->LODModels.Empty();
#if ENGINE_MINOR_VERSION < 19
    new(ImportedResource->LODModels)FStaticLODModel();
#else
    new(ImportedResource->LODModels)FSkeletalMeshLODModel();
#endif

#if ENGINE_MINOR_VERSION < 20
    TArray<FSkeletalMeshLODInfo>& SkeletalMeshLODInfo = SkeletalMesh->LODInfo;
#else
    TArray<FSkeletalMeshLODInfo>& SkeletalMeshLODInfo = SkeletalMesh->GetLODInfoArray();
#endif
    SkeletalMeshLODInfo.Empty();
    SkeletalMeshLODInfo.AddZeroed();
    SkeletalMeshLODInfo[0].LODHysteresis = 0.02f;
    FSkeletalMeshOptimizationSettings Settings;
    // set default reduction settings values
    SkeletalMeshLODInfo[0].ReductionSettings = Settings;

#if ENGINE_MINOR_VERSION < 19
    FStaticLODModel& LODModel = ImportedResource->LODModels[0];
#else
    FSkeletalMeshLODModel& LODModel = ImportedResource->LODModels[0];
#endif
    LODModel.NumTexCoords = FMath::Max<uint32>(1, SkeletalMeshImportData.NumTexCoords);

    int32 SkeletalDepth = 0;
    if (!glTFForUE4Ed::ProcessImportMeshSkeleton(SkeletalMeshImportData, SkeletalMesh->RefSkeleton, SkeletalDepth))
    {
        SkeletalMesh->ClearFlags(RF_Standalone);
        SkeletalMesh->Rename(nullptr, GetTransientPackage());
        SkeletalMesh->MarkPendingKill();
        return nullptr;
    }

    TArray<FText> WarningMessages;
    TArray<FName> WarningNames;
#if ENGINE_MINOR_VERSION < 11
    if (!MeshUtilities.BuildSkeletalMesh(LODModel, SkeletalMesh->RefSkeleton, LODInfluences, LODWedges, LODFaces, LODPoints, LODPointToRawMap, false/*ImportOptions->bKeepOverlappingVertices*/, bShouldComputeNormals, bShouldComputeTangents, &WarningMessages, &WarningNames))
#else
    IMeshUtilities::MeshBuildOptions BuildOptions;
    BuildOptions.bComputeNormals = bShouldComputeNormals;
    BuildOptions.bComputeTangents = bShouldComputeTangents;
    BuildOptions.bUseMikkTSpace = glTFImporterOptions->bUseMikkTSpace;
    if (!MeshUtilities.BuildSkeletalMesh(LODModel, SkeletalMesh->RefSkeleton, LODInfluences, LODWedges, LODFaces, LODPoints, LODPointToRawMap, BuildOptions, &WarningMessages, &WarningNames))
#endif
    {
        SkeletalMesh->ClearFlags(RF_Standalone);
        SkeletalMesh->Rename(nullptr, GetTransientPackage());
        SkeletalMesh->MarkPendingKill();
        return nullptr;
    }

    /// generate the skeleton object
    FString SkeletonObjectName = FString::Printf(TEXT("%s_Skeleton"), *SkeletalMeshName.ToString());
    USkeleton* Skeleton = NewObject<USkeleton>(InputParent, USkeleton::StaticClass(), *SkeletonObjectName, InputFlags);
    if (Skeleton)
    {
        FAssetRegistryModule::AssetCreated(Skeleton);

        Skeleton->MergeAllBonesToBoneTree(SkeletalMesh);

        if (SkeletalMesh->Skeleton != Skeleton)
        {
            SkeletalMesh->Skeleton = Skeleton;
        }

        Skeleton->MarkPackageDirty();
    }

    /// generate the skeleton animation
    if (glTFImporterOptions->bImportAnimationForSkeletalMesh)
    {
        TSharedPtr<FglTFImporterEdAnimationSequence> glTFImporterEdAnimationSequence = FglTFImporterEdAnimationSequence::Get(InputFactory, InputClass, InputParent, SkeletalMeshName, InputFlags, FeedbackContext);
        glTFImporterEdAnimationSequence->CreateAnimationSequence(
            InglTFImporterOptions, InGlTF
            , InNodeRelativeTransforms, InNodeAbsoluteTransforms, InBuffers, NodeIndexToBoneNames
            , SkeletalMesh, Skeleton
            , FeedbackTaskWrapper);
    }

    /// import the material
    if (glTFImporterOptions->bImportMaterial)
    {
        TMap<FString, UTexture*> TextureLibrary;
        TSharedPtr<FglTFImporterEdMaterial> glTFImporterEdMaterial = FglTFImporterEdMaterial::Get(InputFactory, InputClass, InputParent, SkeletalMeshName, InputFlags, FeedbackContext);
        for (const FglTFMaterialInfo& MaterialInfo : MaterialInfos)
        {
            UMaterial* NewMaterial = glTFImporterEdMaterial->CreateMaterial(InglTFImporterOptions, InGlTF, InBuffers, MaterialInfo, TextureLibrary, FeedbackTaskWrapper);
            if (!NewMaterial)
            {
                checkSlow(0);
                continue;
            }
            SkeletalMesh->Materials.Add(FSkeletalMaterial(NewMaterial));
        }
    }
    else
    {
#if ENGINE_MINOR_VERSION < 21
        for (const VMaterial& Material : SkeletalMeshImportData.Materials)
#else
        for (const SkeletalMeshImportData::FMaterial& Material : SkeletalMeshImportData.Materials)
#endif
        {
            SkeletalMesh->Materials.Add(Material.Material.Get());
        }
    }

    SkeletalMesh->PostEditChange();
    SkeletalMesh->MarkPackageDirty();
    return SkeletalMesh;
}

bool FglTFImporterEdSkeletalMesh::GenerateSkeletalMeshImportData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMesh>& InMesh, const std::shared_ptr<libgltf::SSkin>& InSkin
    , const TArray<int32>& InNodeParentIndices, const TArray<FTransform>& InNodeRelativeTransforms, const TArray<FTransform>& InNodeAbsoluteTransforms, const FglTFBuffers& InBuffers
    , FSkeletalMeshImportData& OutSkeletalMeshImportData, TArray<FMatrix>& OutInverseBindMatrices, TMap<int32, FString>& OutNodeIndexToBoneNames, TArray<FglTFMaterialInfo>& InOutMaterialInfo
    , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    FString MeshName = InMesh->name.c_str();
    for (int32 i = 0; i < static_cast<int32>(InMesh->primitives.size()); ++i)
    {
        const auto& Primitive = InMesh->primitives[i];
        FSkeletalMeshImportData NewSkeletalMeshImportData;
        TArray<FMatrix> NewInverseBindMatrices;
        TMap<int32, FString> NewNodeIndexToBoneNames;
        int32 MaterialId = INDEX_NONE;
        if (!!Primitive->material)
        {
            MaterialId = (*Primitive->material);
        }
        if (!GenerateSkeletalMeshImportData(InGlTF, Primitive, InSkin
            , InNodeParentIndices, InNodeRelativeTransforms, InNodeAbsoluteTransforms, InBuffers
            , NewSkeletalMeshImportData, NewInverseBindMatrices, NewNodeIndexToBoneNames
            , InFeedbackTaskWrapper))
        {
            checkSlow(0);
            continue;
        }
        if (glTFForUE4Ed::CheckAndMerge(NewSkeletalMeshImportData, NewInverseBindMatrices, NewNodeIndexToBoneNames
            , OutSkeletalMeshImportData, OutInverseBindMatrices, OutNodeIndexToBoneNames))
        {
            checkSlow(0);
        }

        FString PrimitiveName = FString::Printf(TEXT("%s%d"), *MeshName, i);
        PrimitiveName = FglTFImporter::SanitizeObjectName(PrimitiveName);
        InOutMaterialInfo.Add(FglTFMaterialInfo(MaterialId, PrimitiveName));
    }
    return true;
}

bool FglTFImporterEdSkeletalMesh::GenerateSkeletalMeshImportData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const std::shared_ptr<libgltf::SSkin>& InSkin
    , const TArray<int32>& InNodeParentIds, const TArray<FTransform>& InNodeRelativeTransforms, const TArray<FTransform>& InNodeAbsoluteTransforms, const FglTFBuffers& InBuffers
    , FSkeletalMeshImportData& OutSkeletalMeshImportData, TArray<FMatrix>& OutInverseBindMatrices, TMap<int32, FString>& OutNodeIndexToBoneNames
    , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    if (!InMeshPrimitive || !InSkin)
    {
        checkSlow(0);
        return false;
    }

    TArray<uint32> TriangleIndices;
    TArray<FVector> Points;
    TArray<FVector> Normals;
    TArray<FVector4> Tangents;
    TArray<FVector2D> TextureCoords[MAX_TEXCOORDS];
    TArray<FMatrix> InverseBindMatrices;
    TArray<FVector4> JointIndeies[GLTF_JOINT_LAYERS_NUM_MAX];
    TArray<FVector4> JointWeights[GLTF_JOINT_LAYERS_NUM_MAX];
    if (!FglTFImporter::GetSkeletalMeshData(InGlTF, InMeshPrimitive, InSkin, InBuffers, TriangleIndices, Points, Normals, Tangents, TextureCoords, InverseBindMatrices, JointIndeies, JointWeights))
    {
        return false;
    }

    if (Points.Num() <= 0 || InverseBindMatrices.Num() < static_cast<int32>(InSkin->joints.size())) return false;

    /// collect the joint id
    TArray<int32> JointIds;
    for (const std::shared_ptr<libgltf::SGlTFId>& JointIdPtr : InSkin->joints)
    {
        if (!JointIdPtr) return false;
        JointIds.Add(*JointIdPtr);
    }

    OutInverseBindMatrices = InverseBindMatrices;

    TArray<FTransform> BoneAbsoluteTransforms;

    for (int32 i = 0; i < JointIds.Num(); ++i)
    {
        const int32 JointId = JointIds[i];

        checkSlow(JointId >= 0 && JointId < static_cast<int32>(InGlTF->nodes.size()));
        if (JointId < 0 || JointId >= static_cast<int32>(InGlTF->nodes.size())) return false;

#if ENGINE_MINOR_VERSION < 21
        VBone Bone;
#else
        SkeletalMeshImportData::FBone Bone;
#endif
        Bone.BonePos.Transform.SetIdentity();

        const std::shared_ptr<libgltf::SNode>& NodePtr = InGlTF->nodes[JointId];
        if (!NodePtr || NodePtr->name.empty())
        {
            Bone.Name = FString::Printf(TEXT("Bone_%s_%d"), InSkin->name.c_str(), JointId);
        }
        else
        {
            Bone.Name = FString::Printf(TEXT("Bone_%s_%d_%s"), InSkin->name.c_str(), JointId, NodePtr->name.c_str());
        }
        Bone.Flags = 0;
        Bone.NumChildren = static_cast<int32>(InGlTF->nodes[JointId]->children.size());

        /// it is a root if the id is not contained in the joints
        Bone.ParentIndex = InNodeParentIds[JointId];
        if (!JointIds.Contains(Bone.ParentIndex))
        {
            Bone.ParentIndex = INDEX_NONE;
        }

        if (Bone.ParentIndex == INDEX_NONE)
        {
            Bone.BonePos.Transform = InNodeAbsoluteTransforms[JointId];
        }
        else
        {
            Bone.BonePos.Transform = InNodeRelativeTransforms[JointId];
        }

        //TODO:
        Bone.BonePos.Length = 1.0f;
        Bone.BonePos.XSize = 100.0f;
        Bone.BonePos.YSize = 100.0f;
        Bone.BonePos.ZSize = 100.0f;

        OutSkeletalMeshImportData.RefBonesBinary.Add(Bone);

        OutNodeIndexToBoneNames.Add(JointId, Bone.Name);
    }

    /// revise parent index to bone index
    for (int32 i = 0; i < OutSkeletalMeshImportData.RefBonesBinary.Num(); ++i)
    {
#if ENGINE_MINOR_VERSION < 21
        VBone& Bone = OutSkeletalMeshImportData.RefBonesBinary[i];
#else
        SkeletalMeshImportData::FBone& Bone = OutSkeletalMeshImportData.RefBonesBinary[i];
#endif
        if (Bone.ParentIndex == INDEX_NONE) continue;
        int32 ParentBoneIndex = INDEX_NONE;
        if (!JointIds.Find(Bone.ParentIndex, ParentBoneIndex)) return false;
        Bone.ParentIndex = ParentBoneIndex;
    }

    const TArray<FVector4>& JointIndeies0 = JointIndeies[0];
    const TArray<FVector4>& JointWeights0 = JointWeights[0];
    if (JointIndeies0.Num() == Points.Num() && JointWeights0.Num() == Points.Num())
    {
        for (int32 i = 0; i < Points.Num(); ++i)
        {
            const FVector4& JointIndex = JointIndeies0[i];
            const FVector4& JointWeight = JointWeights0[i];

            TArray<int32> JointIndeiesTemp;
            TArray<float> JointWeightsTemp;
            JointIndeiesTemp.Add(static_cast<int32>(JointIndex.X));
            JointWeightsTemp.Add(JointWeight.X);
            JointIndeiesTemp.Add(static_cast<int32>(JointIndex.Y));
            JointWeightsTemp.Add(JointWeight.Y);
            JointIndeiesTemp.Add(static_cast<int32>(JointIndex.Z));
            JointWeightsTemp.Add(JointWeight.Z);
            JointIndeiesTemp.Add(static_cast<int32>(JointIndex.W));
            JointWeightsTemp.Add(JointWeight.W);

#if ENGINE_MINOR_VERSION < 21
            VRawBoneInfluence RawBoneInfluence;
#else
            SkeletalMeshImportData::FRawBoneInfluence RawBoneInfluence;
#endif
            RawBoneInfluence.VertexIndex = i;

            for (int32 j = 0; j < JointIndeiesTemp.Num(); ++j)
            {
                if (JointWeightsTemp[j] == 0.0f) continue;

                RawBoneInfluence.BoneIndex = JointIndeiesTemp[j];
                RawBoneInfluence.Weight = JointWeightsTemp[j];
                OutSkeletalMeshImportData.Influences.Add(RawBoneInfluence);
            }
        }
    }

    OutSkeletalMeshImportData.PointToRawMap.SetNumZeroed(Points.Num());
    for (int32 i = 0; i < Points.Num(); ++i)
    {
        OutSkeletalMeshImportData.PointToRawMap[i] = i;
    }
    OutSkeletalMeshImportData.Points.Append(Points);

    for (int32 i = 0; i < TriangleIndices.Num(); i += GLTF_TRIANGLE_POINTS_NUM)
    {
#if ENGINE_MINOR_VERSION < 21
        VTriangle TriangleFace;
#else
        SkeletalMeshImportData::FTriangle TriangleFace;
#endif
        for (int32 j = 0; j < GLTF_TRIANGLE_POINTS_NUM; ++j)
        {
            TriangleFace.WedgeIndex[j] = OutSkeletalMeshImportData.Wedges.Num();

            const int32 PointIndex = TriangleIndices[i + j];
            if (Normals.Num() == Points.Num())
            {
                const FVector& Normal = Normals[PointIndex];
                TriangleFace.TangentZ[j] = Normal;

                if (Tangents.Num() == Points.Num())
                {
                    const FVector4& Tangent = Tangents[PointIndex];

                    FVector WedgeTangentX(Tangent.X, Tangent.Y, Tangent.Z);

                    TriangleFace.TangentX[j] = WedgeTangentX;
                    TriangleFace.TangentY[j] = FVector::CrossProduct(Normal, WedgeTangentX * Tangent.W);
                }
            }

#if ENGINE_MINOR_VERSION < 21
            VVertex Wedge;
#else
            SkeletalMeshImportData::FVertex Wedge;
#endif
            Wedge.VertexIndex = PointIndex;

            for (int32 k = 0; k < MAX_TEXCOORDS; ++k)
            {
                Wedge.UVs[k] = FVector2D::ZeroVector;

                const TArray<FVector2D>& TextureCoord = TextureCoords[k];
                if (PointIndex >= TextureCoord.Num()) continue;
                Wedge.UVs[k] = TextureCoord[PointIndex];
            }
            OutSkeletalMeshImportData.Wedges.Add(Wedge);
        }
        TriangleFace.MatIndex = 0;
        TriangleFace.AuxMatIndex = 0;
        TriangleFace.SmoothingGroups = 0;
        OutSkeletalMeshImportData.Faces.Add(TriangleFace);
    }

    //TODO: material/texture
#if ENGINE_MINOR_VERSION < 21
    VMaterial Material;
#else
    SkeletalMeshImportData::FMaterial Material;
#endif
    Material.Material = UMaterial::GetDefaultMaterial(MD_Surface);
    Material.MaterialImportName = TEXT("Default");
    OutSkeletalMeshImportData.Materials.Add(Material);

    OutSkeletalMeshImportData.NumTexCoords = 0;
    for (int32 i = 0; i < MAX_TEXCOORDS; ++i)
    {
        const TArray<FVector2D>& TextureCoord = TextureCoords[i];
        if (Points.Num() != TextureCoord.Num() && TriangleIndices.Num() != TextureCoord.Num()) continue;
        ++OutSkeletalMeshImportData.NumTexCoords;
    }
    OutSkeletalMeshImportData.MaxMaterialIndex = 0;
    OutSkeletalMeshImportData.bHasVertexColors = false;
    OutSkeletalMeshImportData.bHasNormals = (Normals.Num() == Points.Num());
    OutSkeletalMeshImportData.bHasTangents = (Tangents.Num() == Points.Num());

    return true;
}

#undef LOCTEXT_NAMESPACE
