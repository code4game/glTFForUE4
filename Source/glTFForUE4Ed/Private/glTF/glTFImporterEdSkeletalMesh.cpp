// Copyright(c) 2016 - 2021 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEdSkeletalMesh.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporterEdMaterial.h"
#include "glTF/glTFImporterEdAnimationSequence.h"

#include <SkeletalMeshTypes.h>
#include <Engine/SkeletalMesh.h>
#include <Misc/Paths.h>

#include <SkelImport.h>
#include <MeshUtilities.h>
#if ENGINE_MINOR_VERSION <= 24
#else
#include <IMeshBuilderModule.h>
#endif
#include <PhysicsAssetUtils.h>
#include <AssetRegistryModule.h>
#include <AssetNotifications.h>

#if ENGINE_MINOR_VERSION <= 18
#else
#include <Rendering/SkeletalMeshModel.h>
#include <Rendering/SkeletalMeshLODModel.h>
#endif

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

namespace glTFForUE4Ed
{
    bool CheckAndMerge(const FSkeletalMeshImportData& InImportData,
        const TMap<int32, FString>& InNodeIndexToBoneNames,
        FSkeletalMeshImportData& OutImportData,
        TMap<int32, FString>& OutNodeIndexToBoneNames)
    {
        int32 MaterialsStartIndex = OutImportData.Materials.Num();
        int32 PointsStartIndex = OutImportData.Points.Num();
        int32 WedgesStartIndex = OutImportData.Wedges.Num();
        int32 RefBonesBinaryStartIndex = OutImportData.RefBonesBinary.Num();

        OutImportData.Materials.Append(InImportData.Materials);
        OutImportData.Points.Append(InImportData.Points);

#if ENGINE_MINOR_VERSION <= 20
        for (VVertex Vertex : InImportData.Wedges)
#else
        for (SkeletalMeshImportData::FVertex Vertex : InImportData.Wedges)
#endif
        {
            Vertex.VertexIndex += PointsStartIndex;
            Vertex.MatIndex += MaterialsStartIndex;
            OutImportData.Wedges.Emplace(Vertex);
        }

#if ENGINE_MINOR_VERSION <= 20
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
            OutImportData.Faces.Emplace(Triangle);
        }

#if ENGINE_MINOR_VERSION <= 20
        for (VRawBoneInfluence RawBoneInfluence : InImportData.Influences)
#else
        for (SkeletalMeshImportData::FRawBoneInfluence RawBoneInfluence : InImportData.Influences)
#endif
        {
            RawBoneInfluence.VertexIndex += PointsStartIndex;
            OutImportData.Influences.Emplace(RawBoneInfluence);
        }

        for (int32 PointToRawMapIndex : InImportData.PointToRawMap)
        {
            PointToRawMapIndex += PointsStartIndex;
            OutImportData.PointToRawMap.Emplace(PointToRawMapIndex);
        }

        if (OutImportData.Materials.Num() > 0)
        {
            OutImportData.MaxMaterialIndex = OutImportData.Materials.Num() - 1;
        }

        OutImportData.NumTexCoords = FMath::Max<uint32>(InImportData.NumTexCoords, OutImportData.NumTexCoords);

        OutImportData.bHasVertexColors |= InImportData.bHasVertexColors;
        OutImportData.bHasNormals |= InImportData.bHasNormals;
        OutImportData.bHasTangents |= InImportData.bHasTangents;

#if ENGINE_MINOR_VERSION <= 20
        for (VBone Bone : InImportData.RefBonesBinary)
#else
        for (SkeletalMeshImportData::FBone Bone : InImportData.RefBonesBinary)
#endif
        {
            Bone.ParentIndex += RefBonesBinaryStartIndex;
            OutImportData.RefBonesBinary.Emplace(Bone);
        }

        for (const TPair<int32, FString>& NodeIndexToBoneName : InNodeIndexToBoneNames)
        {
            OutNodeIndexToBoneNames.FindOrAdd(NodeIndexToBoneName.Key + RefBonesBinaryStartIndex) = NodeIndexToBoneName.Value;
        }

        //TODO:
        OutImportData.MorphTargets.Append(InImportData.MorphTargets);
        OutImportData.MorphTargetModifiedPoints.Append(InImportData.MorphTargetModifiedPoints);
        OutImportData.MorphTargetNames.Append(InImportData.MorphTargetNames);

        return true;
    }

    void CopyLODImportData(const FSkeletalMeshImportData& InSkeletalMeshImportData,
        TArray<FVector>& LODPoints,
#if ENGINE_MINOR_VERSION <= 20
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
#if ENGINE_MINOR_VERSION <= 20
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

#if ENGINE_MINOR_VERSION <= 17
        BoneName.Trim();
        BoneName.TrimTrailing();
#else
        BoneName.TrimStartAndEndInline();
#endif
        BoneName = BoneName.Replace(TEXT(" "), TEXT("-"));

        return BoneName;
    }

    bool ProcessImportMeshSkeleton(const FSkeletalMeshImportData& InImportData, const USkeleton* InSkeleton, FReferenceSkeleton& OutReferenceSkeleton, int32& OutSkeletalDepth, glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper)
    {
#if ENGINE_MINOR_VERSION <= 20
        const TArray<VBone>& RefBonesBinary = InImportData.RefBonesBinary;
#else
        const TArray<SkeletalMeshImportData::FBone>& RefBonesBinary = InImportData.RefBonesBinary;
#endif

        // Setup skeletal hierarchy + names structure.
        OutReferenceSkeleton.Empty();

#if ENGINE_MINOR_VERSION <= 13
#else
        FReferenceSkeletonModifier ReferenceSkeletonModifier(OutReferenceSkeleton, InSkeleton);
#endif

        // Digest bones to the serializable format.
        for (int32 b = 0; b < RefBonesBinary.Num(); b++)
        {
#if ENGINE_MINOR_VERSION <= 20
            const VBone& BinaryBone = RefBonesBinary[b];
#else
            const SkeletalMeshImportData::FBone& BinaryBone = RefBonesBinary[b];
#endif
            const FString BoneNameString = FixupBoneName(BinaryBone.Name);
#if ENGINE_MINOR_VERSION <= 11
            const FName BoneName(*BoneNameString, FNAME_Add, true);
#else
            const FName BoneName(*BoneNameString);
#endif
            if (OutReferenceSkeleton.FindBoneIndex(BoneName) != INDEX_NONE)
            {
                InFeedbackTaskWrapper.Log(ELogVerbosity::Error, FText::Format(LOCTEXT("ProcessSkeletonTheNameOfBoneIsRepeated", "The name of bone - '{0}' is repeated!"), FText::FromName(BoneName)));
                return false;
            }

            const FMeshBoneInfo BoneInfo(BoneName, BinaryBone.Name, BinaryBone.ParentIndex);
#if ENGINE_MINOR_VERSION <= 13
            OutReferenceSkeleton.Add(BoneInfo, BinaryBone.BonePos.Transform);
#else
            ReferenceSkeletonModifier.Add(BoneInfo, BinaryBone.BonePos.Transform);
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

TSharedPtr<FglTFImporterEdSkeletalMesh> FglTFImporterEdSkeletalMesh::Get(UFactory* InFactory, UObject* InParent, FName InName, EObjectFlags InFlags, FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEdSkeletalMesh> glTFImporterEdSkeletalMesh = MakeShareable(new FglTFImporterEdSkeletalMesh);
    glTFImporterEdSkeletalMesh->Set(InParent, InName, InFlags, InFeedbackContext);
    glTFImporterEdSkeletalMesh->InputFactory = InFactory;
    return glTFImporterEdSkeletalMesh;
}

FglTFImporterEdSkeletalMesh::FglTFImporterEdSkeletalMesh()
    : Super()
{
    //
}

FglTFImporterEdSkeletalMesh::~FglTFImporterEdSkeletalMesh()
{
    //
}

USkeletalMesh* FglTFImporterEdSkeletalMesh::CreateSkeletalMesh(
    const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions,
    const std::shared_ptr<libgltf::SGlTF>& InGlTF,
    const int32 InNodeId,
    const std::shared_ptr<libgltf::SGlTFId>& InMeshId,
    const std::shared_ptr<libgltf::SGlTFId>& InSkinId,
    const class FglTFBuffers& InBuffers,
    const FTransform& InNodeTransform,
    FglTFImporterCollection& InOutglTFImporterCollection) const
{
    if (!InglTFImporterOptions.IsValid()) return nullptr;
    if (!InGlTF || !InMeshId) return nullptr;

    const int32 glTFMeshId = *InMeshId;
    if (glTFMeshId < 0 || glTFMeshId >= static_cast<int32>(InGlTF->meshes.size())) return nullptr;
    if (InOutglTFImporterCollection.SkeletalMeshes.Contains(glTFMeshId))
    {
        return InOutglTFImporterCollection.SkeletalMeshes[glTFMeshId];
    }
    const std::shared_ptr<libgltf::SMesh>& glTFMeshPtr = InGlTF->meshes[glTFMeshId];

    std::shared_ptr<libgltf::SSkin> glTFSkinPtr = nullptr;
    if (InSkinId)
    {
        const int32 glTFSkinId = *InSkinId;
        if (glTFSkinId >= 0 && glTFSkinId < static_cast<int32>(InGlTF->skins.size()))
        {
            glTFSkinPtr = InGlTF->skins[glTFSkinId];
        }
    }

    if (!InputParent || !InputName.IsValid()) return nullptr;

    const TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();
    check(glTFImporterOptions->Details);

    const FString MeshName =
        FglTFImporter::SanitizeObjectName(GLTF_GLTFSTRING_TO_TCHAR(glTFMeshPtr->name.c_str()));
    const FString SkeletalMeshName = MeshName.IsEmpty()
        ? FString::Printf(TEXT("SK_%s_%d"), *InputName.ToString(), glTFMeshId)
        : FString::Printf(TEXT("SK_%s_%d_%s"), *InputName.ToString(), glTFMeshId, *MeshName);

    const FText TaskName = FText::Format(LOCTEXT("BeginImportAsSkeletalMeshTask", "Importing the glTF mesh ({0}) as a skeletal mesh ({1})"), FText::AsNumber(glTFMeshId), FText::FromString(SkeletalMeshName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, TaskName, true);

    FSkeletalMeshImportData SkeletalMeshImportData;
    TArray<FMatrix> RefBasesInvMatrix;
    TMap<int32, FString> NodeIndexToBoneNames;
    TArray<int32> MaterialIds;
    if (!GenerateSkeletalMeshImportData(InGlTF, InNodeId, glTFMeshPtr, glTFSkinPtr, InBuffers,
        MeshName, InNodeTransform, SkeletalMeshImportData, RefBasesInvMatrix, NodeIndexToBoneNames,
        MaterialIds, FeedbackTaskWrapper, InOutglTFImporterCollection))
    {
        FeedbackTaskWrapper.Log(ELogVerbosity::Error, LOCTEXT("CreateSkeletalMeshFailedToGeneateTheFSkeletalMeshImportData", "Failed to generate the `FSkeletalMeshImportData`!"));
        return nullptr;
    }

    FReferenceSkeleton RefSkeleton;
    int32 SkeletalDepth = 0;
    if (!glTFForUE4Ed::ProcessImportMeshSkeleton(SkeletalMeshImportData, nullptr,
        RefSkeleton, SkeletalDepth, FeedbackTaskWrapper))
    {
        FeedbackTaskWrapper.Log(ELogVerbosity::Error, LOCTEXT("CreateSkeletalMeshFailedToProcessTheFSkeletalMeshImportDataAndFRefSkeleton", "Failed to process the `FSkeletalMeshImportData` and `FRefSkeleton`!"));
        return nullptr;
    }

    IMeshUtilities& MeshUtilities =
        FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");

    const bool bShouldComputeNormals =
        glTFImporterOptions->Details->bRecomputeNormals || !SkeletalMeshImportData.bHasNormals;
    const bool bShouldComputeTangents =
        glTFImporterOptions->Details->bRecomputeTangents || !SkeletalMeshImportData.bHasTangents;

    TArray<FVector> LODPoints;
#if ENGINE_MINOR_VERSION <= 20
    TArray<FMeshWedge> LODWedges;
    TArray<FMeshFace> LODFaces;
    TArray<FVertInfluence> LODInfluences;
#else
    TArray<SkeletalMeshImportData::FMeshWedge> LODWedges;
    TArray<SkeletalMeshImportData::FMeshFace> LODFaces;
    TArray<SkeletalMeshImportData::FVertInfluence> LODInfluences;
#endif
    TArray<int32> LODPointToRawMap;
    //TODO: check the version that can't call `CopyLODImportData`
#if ENGINE_MINOR_VERSION <= 22
    glTFForUE4Ed::CopyLODImportData(SkeletalMeshImportData,
        LODPoints, LODWedges, LODFaces, LODInfluences, LODPointToRawMap);
#else
    SkeletalMeshImportData.CopyLODImportData(
        LODPoints, LODWedges, LODFaces, LODInfluences, LODPointToRawMap);
#endif

#if ENGINE_MINOR_VERSION <= 18
    FStaticLODModel LODModel;
#else
    FSkeletalMeshLODModel LODModel;
#endif
    LODModel.NumTexCoords = FMath::Max<uint32>(1, SkeletalMeshImportData.NumTexCoords);

    TArray<FText> WarningMessages;
    TArray<FName> WarningNames;
#if ENGINE_MINOR_VERSION <= 10
    if (!MeshUtilities.BuildSkeletalMesh(LODModel, RefSkeleton,
        LODInfluences, LODWedges, LODFaces, LODPoints, LODPointToRawMap,
        false/*ImportOptions->bKeepOverlappingVertices*/, bShouldComputeNormals,
        bShouldComputeTangents, &WarningMessages, &WarningNames))
#elif ENGINE_MINOR_VERSION <= 24
    IMeshUtilities::MeshBuildOptions BuildOptions;
    BuildOptions.bRemoveDegenerateTriangles = glTFImporterOptions->Details->bRemoveDegenerates;
    BuildOptions.bComputeNormals = bShouldComputeNormals;
    BuildOptions.bComputeTangents = bShouldComputeTangents;
    BuildOptions.bUseMikkTSpace = glTFImporterOptions->Details->bUseMikkTSpace;
    if (!MeshUtilities.BuildSkeletalMesh(LODModel, RefSkeleton, LODInfluences,
        LODWedges, LODFaces, LODPoints, LODPointToRawMap, BuildOptions,
        &WarningMessages, &WarningNames))
#else
    if (false)
#endif
    {
        for (const FText& WarningMessage : WarningMessages)
        {
            FeedbackTaskWrapper.Log(ELogVerbosity::Warning, WarningMessage);
        }
        FeedbackTaskWrapper.Log(ELogVerbosity::Error, LOCTEXT("CreateSkeletalMeshFailedToBuildTheSkeletalMesh", "Failed to build the skeletal mesh!"));
        return nullptr;
    }

    const FString NewPackagePath =
        FPackageName::GetLongPackagePath(InputParent->GetPathName()) / SkeletalMeshName;
    UObject* NewAssetPackage = InputParent;

    /// load or create new skeletal mesh
    bool bCreated = false;
    USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(NewAssetPackage, *SkeletalMeshName);
    if (!SkeletalMesh)
    {
        NewAssetPackage = LoadPackage(nullptr, *NewPackagePath, LOAD_None);
        if (!NewAssetPackage)
        {
#if (ENGINE_MINOR_VERSION <= 25)
            NewAssetPackage = CreatePackage(nullptr, *NewPackagePath);
#else
            NewAssetPackage = CreatePackage(*NewPackagePath);
#endif
            checkSlow(NewAssetPackage);
        }
        if (!NewAssetPackage)
        {
            //TODO: output error
            return nullptr;
        }
        SkeletalMesh = LoadObject<USkeletalMesh>(NewAssetPackage, *SkeletalMeshName);
    }
    if (!SkeletalMesh)
    {
        /// create new skeletal mesh
        SkeletalMesh = NewObject<USkeletalMesh>(NewAssetPackage, USkeletalMesh::StaticClass(), *SkeletalMeshName, InputFlags);
        checkSlow(SkeletalMesh);
        if (SkeletalMesh) FAssetRegistryModule::AssetCreated(SkeletalMesh);
        bCreated = true;
    }
    if (!SkeletalMesh) return nullptr;

    SkeletalMesh->PreEditChange(nullptr);

#if ENGINE_MINOR_VERSION <= 18
    FSkeletalMeshResource* ImportedResource = SkeletalMesh->GetImportedResource();
#else
    FSkeletalMeshModel* ImportedResource = SkeletalMesh->GetImportedModel();
#endif
    check(ImportedResource);

    /// clean old data
    {
        SkeletalMesh->RefBasesInvMatrix.Empty();

#if ENGINE_MINOR_VERSION <= 19
        TArray<FSkeletalMeshLODInfo>& SkeletalMeshLODInfos = SkeletalMesh->LODInfo;
#else
        TArray<FSkeletalMeshLODInfo>& SkeletalMeshLODInfos = SkeletalMesh->GetLODInfoArray();
#endif
        SkeletalMeshLODInfos.Empty();
        SkeletalMeshLODInfos.Add(FSkeletalMeshLODInfo());
        SkeletalMeshLODInfos[0].LODHysteresis = 0.02f;

        ImportedResource->LODModels.Empty();
#if ENGINE_MINOR_VERSION <= 18
        new(ImportedResource->LODModels)FStaticLODModel();
#elif ENGINE_MINOR_VERSION <= 20
        new(ImportedResource->LODModels)FSkeletalMeshLODModel();
#else
        ImportedResource->LODModels.Add(new FSkeletalMeshLODModel);
#endif

        SkeletalMesh->Materials.Empty();
    }

    SkeletalMesh->RefSkeleton = RefSkeleton;
    SkeletalMesh->CalculateInvRefMatrices();

#if ENGINE_MINOR_VERSION <= 24
    ImportedResource->LODModels[0] = LODModel;
#else
    FSkeletalMeshBuildSettings BuildOptions;
    BuildOptions.bRemoveDegenerates = glTFImporterOptions->Details->bRemoveDegenerates;
    BuildOptions.bRecomputeNormals = bShouldComputeNormals;
    BuildOptions.bRecomputeTangents = bShouldComputeTangents;
    BuildOptions.bUseMikkTSpace = glTFImporterOptions->Details->bUseMikkTSpace;
    SkeletalMesh->SaveLODImportedData(0, SkeletalMeshImportData);
    check(SkeletalMesh->GetLODInfo(0) != nullptr);
    SkeletalMesh->GetLODInfo(0)->BuildSettings = BuildOptions;
    IMeshBuilderModule& MeshBuilderModule = IMeshBuilderModule::GetForRunningPlatform();
    if (!MeshBuilderModule.BuildSkeletalMesh(SkeletalMesh, 0, false))
    {
        if (bCreated)
        {
            SkeletalMesh->MarkPendingKill();
        }
        FeedbackTaskWrapper.Log(ELogVerbosity::Error, LOCTEXT("CreateSkeletalMeshFailedToBuildTheSkeletalMesh", "Failed to build the skeletal mesh!"));
        return nullptr;
    }
#endif

    /// import the material
    if (glTFImporterOptions->Details->bImportMaterial)
    {
        TSharedPtr<FglTFImporterEdMaterial> glTFImporterEdMaterial = FglTFImporterEdMaterial::Get(InputFactory, InputParent, InputName, InputFlags, FeedbackContext);
        for (const int32& MaterialId : MaterialIds)
        {
            UMaterialInterface* NewMaterial = glTFImporterEdMaterial->CreateMaterial(InglTFImporterOptions
                , InGlTF, MaterialId, InBuffers, FeedbackTaskWrapper
                , InOutglTFImporterCollection);
            if (!NewMaterial)
            {
                checkSlow(0);
                continue;
            }
            SkeletalMesh->Materials.Emplace(FSkeletalMaterial(NewMaterial));
        }
    }
    else
    {
#if ENGINE_MINOR_VERSION <= 20
        for (const VMaterial& Material : SkeletalMeshImportData.Materials)
#else
        for (const SkeletalMeshImportData::FMaterial& Material : SkeletalMeshImportData.Materials)
#endif
        {
            SkeletalMesh->Materials.Emplace(Material.Material.Get());
        }
    }

    SkeletalMesh->PostEditChange();
    SkeletalMesh->MarkPackageDirty();

    /// generate the skeleton object
    FString SkeletonObjectName = FString::Printf(TEXT("%s_Skeleton"), *SkeletalMeshName);
    USkeleton* Skeleton = SkeletalMesh->Skeleton;
    if (!Skeleton)
    {
        Skeleton = LoadObject<USkeleton>(NewAssetPackage, *SkeletonObjectName);
    }
    if (!Skeleton)
    {
        Skeleton = NewObject<USkeleton>(NewAssetPackage, USkeleton::StaticClass(), *SkeletonObjectName, InputFlags);
        if (Skeleton) FAssetRegistryModule::AssetCreated(Skeleton);
    }
    checkSlow(Skeleton);
    if (Skeleton)
    {
        if (SkeletalMesh->Skeleton != Skeleton)
        {
            SkeletalMesh->Skeleton = Skeleton;
        }

        Skeleton->MergeAllBonesToBoneTree(SkeletalMesh);

        FAssetNotifications::SkeletonNeedsToBeSaved(Skeleton);

        Skeleton->MarkPackageDirty();
    }

    /// import the animation
    if (glTFImporterOptions->Details->bImportAnimation)
    {
        /// generate the skeleton animation
        FglTFImporterEdAnimationSequence::Get(InputFactory, NewAssetPackage, *SkeletalMeshName, InputFlags, FeedbackContext)
            ->CreateAnimationSequence(InglTFImporterOptions, InGlTF
                , InBuffers, NodeIndexToBoneNames
                , SkeletalMesh, Skeleton
                , FeedbackTaskWrapper
                , InOutglTFImporterCollection);
    }

    /// generate the physics object
    /// it will cause a crash when rebuild the physics asset of the skeletal mesh
    //TODO: supports to reimport the physics asset
    if (bCreated && glTFImporterOptions->Details->bCreatePhysicsAsset)
    {
        FString PhysicsObjectName = FString::Printf(TEXT("%s_Physics"), *SkeletalMeshName);
        UPhysicsAsset* PhysicsAsset = SkeletalMesh->PhysicsAsset;
        SkeletalMesh->PhysicsAsset = nullptr;
        if (!PhysicsAsset)
        {
            PhysicsAsset = LoadObject<UPhysicsAsset>(NewAssetPackage, *PhysicsObjectName);
        }
        if (!PhysicsAsset)
        {
            PhysicsAsset = NewObject<UPhysicsAsset>(NewAssetPackage, UPhysicsAsset::StaticClass(), *PhysicsObjectName, InputFlags);
            if (PhysicsAsset) FAssetRegistryModule::AssetCreated(PhysicsAsset);
        }
        if (PhysicsAsset)
        {
            PhysicsAsset->InvalidateAllPhysicsMeshes();

            FPhysAssetCreateParams NewBodyData;
            NewBodyData.MinBoneSize = SkeletalMeshImportData.RefBonesBinary.Num();
            FText CreationErrorMessage;
            if (!FPhysicsAssetUtils::CreateFromSkeletalMesh(PhysicsAsset, SkeletalMesh, NewBodyData, CreationErrorMessage))
            {
                checkSlow(0);
                FeedbackTaskWrapper.Log(ELogVerbosity::Error, CreationErrorMessage);

                TArray<UObject*> ObjectsToDelete;
                ObjectsToDelete.Add(PhysicsAsset);
                ObjectTools::DeleteObjects(ObjectsToDelete, false);
            }
        }
    }

    InOutglTFImporterCollection.SkeletalMeshes.Add(glTFMeshId, SkeletalMesh);
    return SkeletalMesh;
}

bool FglTFImporterEdSkeletalMesh::GenerateSkeletalMeshImportData(
    const std::shared_ptr<libgltf::SGlTF>& InGlTF,
    const int32 InNodeId,
    const std::shared_ptr<libgltf::SMesh>& InMesh,
    const std::shared_ptr<libgltf::SSkin>& InSkin,
    const FglTFBuffers& InBuffers,
    const FString& InMeshName,
    const FTransform& InNodeTransform,
    FSkeletalMeshImportData& OutSkeletalMeshImportData,
    TArray<FMatrix>& OutInverseBindMatrices,
    TMap<int32, FString>& OutNodeIndexToBoneNames,
    TArray<int32>& InOutMaterialIds,
    const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper,
    FglTFImporterCollection& InOutglTFImporterCollection) const
{
    for (int32 i = 0; i < static_cast<int32>(InMesh->primitives.size()); ++i)
    {
        const auto& Primitive = InMesh->primitives[i];
        FSkeletalMeshImportData NewSkeletalMeshImportData;
        TMap<int32, FString> NewNodeIndexToBoneNames;
        int32 MaterialId = INDEX_NONE;
        if (!!Primitive->material)
        {
            MaterialId = (*Primitive->material);
        }
        if (!GenerateSkeletalMeshImportData(
            InGlTF, InMesh, Primitive, InBuffers,
            NewSkeletalMeshImportData, NewNodeIndexToBoneNames,
            InFeedbackTaskWrapper, InOutglTFImporterCollection))
        {
            checkSlow(0);
            continue;
        }
        if (glTFForUE4Ed::CheckAndMerge(NewSkeletalMeshImportData, NewNodeIndexToBoneNames
            , OutSkeletalMeshImportData, OutNodeIndexToBoneNames))
        {
            checkSlow(0);
        }

        InOutMaterialIds.Emplace(MaterialId);
    }

    // generate the skeletal data
    if (InSkin == nullptr)
    {
        OutSkeletalMeshImportData.RefBonesBinary.SetNum(1);
#if ENGINE_MINOR_VERSION <= 20
        VBone& EmptyBone = OutSkeletalMeshImportData.RefBonesBinary[0];
#else
        SkeletalMeshImportData::FBone& EmptyBone = OutSkeletalMeshImportData.RefBonesBinary[0];
#endif
        EmptyBone.Name = FString::Printf(TEXT("Bone_%s_Root"), *InMeshName);
        EmptyBone.Flags = 0;
        EmptyBone.NumChildren = 0;
        EmptyBone.ParentIndex = INDEX_NONE;
        EmptyBone.BonePos.Transform.SetIdentity();
        EmptyBone.BonePos.Length = 1.0f;
        EmptyBone.BonePos.XSize = 100.0f;
        EmptyBone.BonePos.YSize = 100.0f;
        EmptyBone.BonePos.ZSize = 100.0f;
        OutNodeIndexToBoneNames.Add(InNodeId, EmptyBone.Name);

        OutSkeletalMeshImportData.Influences.SetNum(OutSkeletalMeshImportData.Points.Num());
        for (int32 i = 0, ic = OutSkeletalMeshImportData.Influences.Num(); i < ic; ++i)
        {
#if ENGINE_MINOR_VERSION <= 20
            VRawBoneInfluence RawBoneInfluence;
            VRawBoneInfluence& EmptyInfluence = OutSkeletalMeshImportData.Influences[i];
#else
            SkeletalMeshImportData::FRawBoneInfluence& EmptyInfluence = OutSkeletalMeshImportData.Influences[i];
#endif
            EmptyInfluence.Weight = 1.0f;
            EmptyInfluence.VertexIndex = i;
            EmptyInfluence.BoneIndex = 0;
        }

        OutInverseBindMatrices.SetNum(1);
        OutInverseBindMatrices[0] = FMatrix::Identity;

        /// transform the point and tangent
        if (!InNodeTransform.Equals(FTransform::Identity))
        {
            for (FVector& Point : OutSkeletalMeshImportData.Points)
            {
                Point = InNodeTransform.TransformPosition(Point);
            }
            for (SkeletalMeshImportData::FTriangle& Triangle : OutSkeletalMeshImportData.Faces)
            {
                for (int32 i = 0; i < 3; ++i)
                {
                    FVector& TangentX = Triangle.TangentX[i];
                    TangentX = InNodeTransform.TransformVectorNoScale(TangentX);
                    FVector& TangentY = Triangle.TangentY[i];
                    TangentY = InNodeTransform.TransformVectorNoScale(TangentY);
                    FVector& TangentZ = Triangle.TangentZ[i];
                    TangentZ = InNodeTransform.TransformVectorNoScale(TangentZ);
                }
            }
        }
        return true;
    }
    return GenerateSkeletalMeshImportData(InGlTF, InSkin, InBuffers, InNodeTransform,
        OutSkeletalMeshImportData, OutInverseBindMatrices, OutNodeIndexToBoneNames,
        InFeedbackTaskWrapper, InOutglTFImporterCollection);
}

bool FglTFImporterEdSkeletalMesh::GenerateSkeletalMeshImportData(
    const std::shared_ptr<libgltf::SGlTF>& InGlTF,
    const std::shared_ptr<libgltf::SMesh>& InMesh,
    const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive,
    const FglTFBuffers& InBuffers,
    FSkeletalMeshImportData& OutSkeletalMeshImportData,
    TMap<int32, FString>& OutNodeIndexToBoneNames,
    const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper,
    FglTFImporterCollection& InOutglTFImporterCollection) const
{
    if (!InMeshPrimitive)
    {
        checkSlow(0);
        return false;
    }

    TArray<uint32> TriangleIndices;
    TArray<FVector> Points;
    TArray<TArray<FVector>> MorphTargetsPoints;
    TArray<FVector> Normals;
    TArray<TArray<FVector>> MorphTargetsNormals;
    TArray<FVector4> Tangents;
    TArray<TArray<FVector4>> MorphTargetsTangents;
    TArray<FVector2D> TextureCoords[MAX_TEXCOORDS];
    TArray<FVector4> JointsIndeies[GLTF_JOINT_LAYERS_NUM_MAX];
    TArray<FVector4> JointsWeights[GLTF_JOINT_LAYERS_NUM_MAX];
    if (!FglTFImporter::GetSkeletalMeshData(InGlTF, InMeshPrimitive, InBuffers,
        TriangleIndices,
        Points, MorphTargetsPoints,
        Normals, MorphTargetsNormals,
        Tangents, MorphTargetsTangents,
        TextureCoords,
        JointsIndeies, JointsWeights))
    {
        return false;
    }

    if (Points.Num() <= 0) return false;

    const TArray<FVector4>& JointIndeies0 = JointsIndeies[0];
    const TArray<FVector4>& JointWeights0 = JointsWeights[0];
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

#if ENGINE_MINOR_VERSION <= 20
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
    OutSkeletalMeshImportData.Points = Points;

    for (int32 i = 0; i < TriangleIndices.Num(); i += GLTF_TRIANGLE_POINTS_NUM)
    {
#if ENGINE_MINOR_VERSION <= 20
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

#if ENGINE_MINOR_VERSION <= 20
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
#if ENGINE_MINOR_VERSION <= 20
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

bool FglTFImporterEdSkeletalMesh::GenerateSkeletalMeshImportData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SSkin>& InSkin, const class FglTFBuffers& InBuffers
    , const FTransform& InNodeTransform, FSkeletalMeshImportData& OutSkeletalMeshImportData, TArray<FMatrix>& OutInverseBindMatrices, TMap<int32, FString>& OutNodeIndexToBoneNames
    , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper, FglTFImporterCollection& InOutglTFImporterCollection) const
{
    if (!InSkin)
    {
        checkSlow(0);
        return false;
    }

    if (!FglTFImporter::GetInverseBindMatrices(InGlTF, InSkin, InBuffers, OutInverseBindMatrices))
    {
        checkSlow(0);
        //TODO:
        return false;
    }

    if (OutInverseBindMatrices.Num() != static_cast<int32>(InSkin->joints.size()))
    {
        checkSlow(0);
        //TODO:
        return false;
    }

    const FString SkinName = GLTF_GLTFSTRING_TO_TCHAR(InSkin->name.c_str());

    /// collect the joint id
    TArray<int32> JointIds;
    for (int32 i = 0, ic = static_cast<int32>(InSkin->joints.size()); i < ic; ++i)
    {
        const std::shared_ptr<libgltf::SGlTFId>& JointIdPtr = InSkin->joints[i];
        if (!JointIdPtr) return false;

        const int32 JointId = *JointIdPtr;

        checkSlow(JointId >= 0 && JointId < static_cast<int32>(InGlTF->nodes.size()));
        if (JointId < 0 || JointId >= static_cast<int32>(InGlTF->nodes.size()))
        {
            checkSlow(0);
            //TODO:
            return false;
        }

        JointIds.Emplace(JointId);

#if ENGINE_MINOR_VERSION <= 20
        VBone Bone;
#else
        SkeletalMeshImportData::FBone Bone;
#endif
        Bone.BonePos.Transform.SetIdentity();

        const std::shared_ptr<libgltf::SNode>& NodePtr = InGlTF->nodes[JointId];
        if (!NodePtr || NodePtr->name.empty())
        {
            Bone.Name = FString::Printf(TEXT("Bone_%s_%d"), *SkinName, JointId);
        }
        else
        {
            FString NodeName = GLTF_GLTFSTRING_TO_TCHAR(NodePtr->name.c_str());
            Bone.Name = FString::Printf(TEXT("Bone_%s_%d_%s"), *SkinName, JointId, *NodeName);
        }
        Bone.Flags = 0;
        Bone.NumChildren = static_cast<int32>(InGlTF->nodes[JointId]->children.size());

        const FglTFImporterNodeInfo& NodeInfo = InOutglTFImporterCollection.FindNodeInfo(JointId);

        /// it is a root if the id is not contained in the joints
        Bone.ParentIndex = NodeInfo.ParentIndex;
        if (!JointIds.Contains(Bone.ParentIndex))
        {
            Bone.ParentIndex = INDEX_NONE;
            Bone.BonePos.Transform.SetFromMatrix(OutInverseBindMatrices[i].Inverse());
            Bone.BonePos.Transform *= InNodeTransform;
        }
        else
        {
            Bone.BonePos.Transform = NodeInfo.RelativeTransform;
        }

        //TODO:
        Bone.BonePos.Length = 1.0f;
        Bone.BonePos.XSize = 100.0f;
        Bone.BonePos.YSize = 100.0f;
        Bone.BonePos.ZSize = 100.0f;

        OutSkeletalMeshImportData.RefBonesBinary.Emplace(Bone);

        OutNodeIndexToBoneNames.Add(JointId, Bone.Name);
    }

    /// convert the parent index from the inde of the joint list to the index of the bone list
#if ENGINE_MINOR_VERSION <= 20
    for (VBone& Bone : OutSkeletalMeshImportData.RefBonesBinary)
#else
    for (SkeletalMeshImportData::FBone& Bone : OutSkeletalMeshImportData.RefBonesBinary)
#endif
    {
        if (Bone.ParentIndex == INDEX_NONE) continue;
        int32 ParentBoneIndex = INDEX_NONE;
        if (!JointIds.Find(Bone.ParentIndex, ParentBoneIndex)) return false;
        Bone.ParentIndex = ParentBoneIndex;
    }

    /// transform the point and tangent
    if (!InNodeTransform.Equals(FTransform::Identity))
    {
        for (FVector& Point : OutSkeletalMeshImportData.Points)
        {
            Point = InNodeTransform.TransformPosition(Point);
        }
        for (SkeletalMeshImportData::FTriangle& Triangle : OutSkeletalMeshImportData.Faces)
        {
            for (int32 i = 0; i < 3; ++i)
            {
                FVector& TangentX = Triangle.TangentX[i];
                TangentX = InNodeTransform.TransformVectorNoScale(TangentX);
                FVector& TangentY = Triangle.TangentY[i];
                TangentY = InNodeTransform.TransformVectorNoScale(TangentY);
                FVector& TangentZ = Triangle.TangentZ[i];
                TangentZ = InNodeTransform.TransformVectorNoScale(TangentZ);
            }
        }
    }

    return true;
}

#undef LOCTEXT_NAMESPACE
