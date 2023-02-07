// Copyright(c) 2016 - 2023 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEdSkeletalMesh.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporterEdMaterial.h"
#include "glTF/glTFImporterEdAnimationSequence.h"

#include <SkeletalMeshTypes.h>
#include <Engine/SkeletalMesh.h>
#include <Animation/MorphTarget.h>
#include <Misc/Paths.h>

#if GLTFFORUE_ENGINE_VERSION < 427
#    include <SkelImport.h>
#else
#    include <ImportUtils/SkelImport.h>
#endif
#include <MeshUtilities.h>
#if GLTFFORUE_ENGINE_VERSION < 424
#else
#    include <IMeshBuilderModule.h>
#    include <LODUtilities.h>
#endif
#include <PhysicsAssetUtils.h>
#if GLTFFORUE_ENGINE_VERSION < 501
#    include <AssetRegistryModule.h>
#else
#    include <AssetRegistry/AssetRegistryModule.h>
#endif
#include <AssetNotifications.h>

#if GLTFFORUE_ENGINE_VERSION < 419
#else
#    include <Rendering/SkeletalMeshModel.h>
#    include <Rendering/SkeletalMeshLODModel.h>
#endif

#if GLTFFORUE_ENGINE_VERSION < 427
#else
#    include <Engine/SkeletalMesh.h>
#endif

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

namespace glTFForUE4Ed
{
    FString FixupBoneName(const FString& InBoneName)
    {
        FString BoneName = InBoneName;

#if GLTFFORUE_ENGINE_VERSION < 418
        BoneName.Trim();
        BoneName.TrimTrailing();
#else
        BoneName.TrimStartAndEndInline();
#endif
        BoneName = BoneName.Replace(TEXT(" "), TEXT("-"));

        return BoneName;
    }

    bool CheckAndMerge(const FSkeletalMeshImportData& InImportData,
                       const TMap<int32, FString>&    InNodeIndexToBoneNames,
                       FSkeletalMeshImportData&       OutImportData,
                       TMap<int32, FString>&          OutNodeIndexToBoneNames)
    {
        int32 MaterialsStartIndex      = OutImportData.Materials.Num();
        int32 PointsStartIndex         = OutImportData.Points.Num();
        int32 WedgesStartIndex         = OutImportData.Wedges.Num();
        int32 RefBonesBinaryStartIndex = OutImportData.RefBonesBinary.Num();

        OutImportData.Materials.Append(InImportData.Materials);
        OutImportData.Points.Append(InImportData.Points);

#if GLTFFORUE_ENGINE_VERSION < 421
        for (VVertex Vertex : InImportData.Wedges)
#else
        for (SkeletalMeshImportData::FVertex Vertex : InImportData.Wedges)
#endif
        {
            Vertex.VertexIndex += PointsStartIndex;
            Vertex.MatIndex += MaterialsStartIndex;
            OutImportData.Wedges.Emplace(Vertex);
        }

#if GLTFFORUE_ENGINE_VERSION < 421
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

#if GLTFFORUE_ENGINE_VERSION < 421
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

#if GLTFFORUE_ENGINE_VERSION < 421
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

        return true;
    }

    void TransformSkeletalMeshImportData(FSkeletalMeshImportData& InOutSkeletalMeshImportData, const FTransform& InNodeTransform)
    {
#if GLTFFORUE_ENGINE_VERSION < 500
        for (FVector& Point : InOutSkeletalMeshImportData.Points)
#else
        for (FVector3f& Point : InOutSkeletalMeshImportData.Points)
#endif
        {
#if GLTFFORUE_ENGINE_VERSION < 500
            Point = InNodeTransform.TransformPosition(Point);
#else
            Point = FVector3f(InNodeTransform.TransformPosition(FVector(Point)));
#endif
        }
        for (SkeletalMeshImportData::FTriangle& Triangle : InOutSkeletalMeshImportData.Faces)
        {
            for (int32 i = 0; i < 3; ++i)
            {
#if GLTFFORUE_ENGINE_VERSION < 500
                FVector& TangentX = Triangle.TangentX[i];
                TangentX          = InNodeTransform.TransformVectorNoScale(TangentX);
                FVector& TangentY = Triangle.TangentY[i];
                TangentY          = InNodeTransform.TransformVectorNoScale(TangentY);
                FVector& TangentZ = Triangle.TangentZ[i];
                TangentZ          = InNodeTransform.TransformVectorNoScale(TangentZ);
#else
                FVector3f& TangentX = Triangle.TangentX[i];
                TangentX            = FVector3f(InNodeTransform.TransformVectorNoScale(FVector(TangentX)));
                FVector3f& TangentY = Triangle.TangentY[i];
                TangentY            = FVector3f(InNodeTransform.TransformVectorNoScale(FVector(TangentY)));
                FVector3f& TangentZ = Triangle.TangentZ[i];
                TangentZ            = FVector3f(InNodeTransform.TransformVectorNoScale(FVector(TangentZ)));
#endif
            }
        }
    }

    void CopyLODImportData(const FSkeletalMeshImportData& InSkeletalMeshImportData,
                           TArray<FVector>&               LODPoints,
#if GLTFFORUE_ENGINE_VERSION < 421
                           TArray<FMeshWedge>&     LODWedges,
                           TArray<FMeshFace>&      LODFaces,
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
#if GLTFFORUE_ENGINE_VERSION < 500
            LODPoints[p] = InSkeletalMeshImportData.Points[p];
#else
            LODPoints[p] = FVector(InSkeletalMeshImportData.Points[p]);
#endif
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
#if GLTFFORUE_ENGINE_VERSION < 421
            FMeshFace Face;
#else
            SkeletalMeshImportData::FMeshFace Face;
#endif
            Face.iWedge[0]         = InSkeletalMeshImportData.Faces[f].WedgeIndex[0];
            Face.iWedge[1]         = InSkeletalMeshImportData.Faces[f].WedgeIndex[1];
            Face.iWedge[2]         = InSkeletalMeshImportData.Faces[f].WedgeIndex[2];
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
            LODInfluences[i].Weight    = InSkeletalMeshImportData.Influences[i].Weight;
            LODInfluences[i].VertIndex = InSkeletalMeshImportData.Influences[i].VertexIndex;
            LODInfluences[i].BoneIndex = InSkeletalMeshImportData.Influences[i].BoneIndex;
        }

        // Copy mapping
        LODPointToRawMap = InSkeletalMeshImportData.PointToRawMap;
    }

    bool ProcessImportMeshSkeleton(const FSkeletalMeshImportData&    InImportData,
                                   const USkeleton*                  InSkeleton,
                                   FReferenceSkeleton&               OutReferenceSkeleton,
                                   int32&                            OutSkeletalDepth,
                                   glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper)
    {
#if GLTFFORUE_ENGINE_VERSION < 421
        const TArray<VBone>& RefBonesBinary = InImportData.RefBonesBinary;
#else
        const TArray<SkeletalMeshImportData::FBone>& RefBonesBinary = InImportData.RefBonesBinary;
#endif

        // Setup skeletal hierarchy + names structure.
        OutReferenceSkeleton.Empty();

#if GLTFFORUE_ENGINE_VERSION < 414
#else
        FReferenceSkeletonModifier ReferenceSkeletonModifier(OutReferenceSkeleton, InSkeleton);
#endif

        TArray<int32> RootBoneIndices;
        for (int32 b = 0; b < RefBonesBinary.Num(); b++)
        {
#if GLTFFORUE_ENGINE_VERSION < 421
            const VBone& BinaryBone = RefBonesBinary[b];
#else
            const SkeletalMeshImportData::FBone& BinaryBone = RefBonesBinary[b];
#endif
            if (BinaryBone.ParentIndex == INDEX_NONE)
                RootBoneIndices.Add(b);
        }
        int32 NewRootIndex = INDEX_NONE;
        if (RootBoneIndices.Num() > 1)
        {
            /// create new root bone for multi root bones
            /// and use the transform of the first root bone
            const FName         BoneName(TEXT("RootNew"));
            const FString       BoneString = BoneName.ToString();
            const FMeshBoneInfo BoneInfo(BoneName, BoneString, -1);
#if GLTFFORUE_ENGINE_VERSION < 414
            NewRootIndex = OutReferenceSkeleton.GetNum();
            OutReferenceSkeleton.Add(BoneInfo, RefBonesBinary[RootBoneIndices[0]].BonePos.Transform);
#elif GLTFFORUE_ENGINE_VERSION < 500
            NewRootIndex = ReferenceSkeletonModifier.GetReferenceSkeleton().GetNum();
            ReferenceSkeletonModifier.Add(BoneInfo, RefBonesBinary[RootBoneIndices[0]].BonePos.Transform);
#else
            NewRootIndex = ReferenceSkeletonModifier.GetReferenceSkeleton().GetNum();
            ReferenceSkeletonModifier.Add(BoneInfo, FTransform(RefBonesBinary[RootBoneIndices[0]].BonePos.Transform));
#endif
        }

        // Digest bones to the serializable format.
        for (int32 b = 0; b < RefBonesBinary.Num(); b++)
        {
#if GLTFFORUE_ENGINE_VERSION < 421
            const VBone& BinaryBone = RefBonesBinary[b];
#else
            const SkeletalMeshImportData::FBone& BinaryBone = RefBonesBinary[b];
#endif
            const FString BoneNameString = FixupBoneName(BinaryBone.Name);
#if GLTFFORUE_ENGINE_VERSION < 412
            const FName BoneName(*BoneNameString, FNAME_Add, true);
#else
            const FName BoneName(*BoneNameString);
#endif
            if (OutReferenceSkeleton.FindBoneIndex(BoneName) != INDEX_NONE)
            {
                InFeedbackTaskWrapper.Log(ELogVerbosity::Error,
                                          FText::Format(LOCTEXT("ProcessSkeletonTheNameOfBoneIsRepeated", "The name of bone - '{0}' is repeated!"),
                                                        FText::FromName(BoneName)));
                return false;
            }

            int32 BoneParentIndex = BinaryBone.ParentIndex;
            if (BoneParentIndex == -1)
                BoneParentIndex = NewRootIndex;
            else if (NewRootIndex != -1)
                BoneParentIndex += 1;
            const FMeshBoneInfo BoneInfo(BoneName, BinaryBone.Name, BoneParentIndex);

#if GLTFFORUE_ENGINE_VERSION < 414
            OutReferenceSkeleton.Add(BoneInfo, BinaryBone.BonePos.Transform);
#elif GLTFFORUE_ENGINE_VERSION < 500
            ReferenceSkeletonModifier.Add(BoneInfo, BinaryBone.BonePos.Transform);
#else
            ReferenceSkeletonModifier.Add(BoneInfo, FTransform(BinaryBone.BonePos.Transform));
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
            int32 Depth  = 1.0f;

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

    bool BuildSkeletalMeshImportData(const TArray<uint32>&    InTriangleIndices,
                                     const TArray<FVector>&   InPoints,
                                     const TArray<FVector>&   InNormals,
                                     const TArray<FVector4>&  InTangents,
                                     const TArray<FVector2D>  InTextureCoords[MAX_TEXCOORDS],
                                     const TArray<FVector4>   InJointsIndeies[GLTF_JOINT_LAYERS_NUM_MAX],
                                     const TArray<FVector4>   InJointsWeights[GLTF_JOINT_LAYERS_NUM_MAX],
                                     FSkeletalMeshImportData& InOutSkeletalMeshImportData)
    {
        if (InPoints.Num() <= 0)
            return false;

        const TArray<FVector4>& JointIndeies0 = InJointsIndeies[0];
        const TArray<FVector4>& JointWeights0 = InJointsWeights[0];
        if (JointIndeies0.Num() == InPoints.Num() && JointWeights0.Num() == InPoints.Num())
        {
            for (int32 i = 0; i < InPoints.Num(); ++i)
            {
                const FVector4& JointIndex  = JointIndeies0[i];
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

#if GLTFFORUE_ENGINE_VERSION < 421
                VRawBoneInfluence RawBoneInfluence;
#else
                SkeletalMeshImportData::FRawBoneInfluence RawBoneInfluence;
#endif
                RawBoneInfluence.VertexIndex = i;

                for (int32 j = 0; j < JointIndeiesTemp.Num(); ++j)
                {
                    if (JointWeightsTemp[j] == 0.0f)
                        continue;

                    RawBoneInfluence.BoneIndex = JointIndeiesTemp[j];
                    RawBoneInfluence.Weight    = JointWeightsTemp[j];
                    InOutSkeletalMeshImportData.Influences.Add(RawBoneInfluence);
                }
            }
        }

        InOutSkeletalMeshImportData.PointToRawMap.SetNumZeroed(InPoints.Num());
        InOutSkeletalMeshImportData.Points.SetNumZeroed(InPoints.Num());
        for (int32 i = 0; i < InPoints.Num(); ++i)
        {
            InOutSkeletalMeshImportData.PointToRawMap[i] = i;
#if GLTFFORUE_ENGINE_VERSION < 500
#else
            InOutSkeletalMeshImportData.Points[i] = FVector3f(InPoints[i]);
#endif
        }
#if GLTFFORUE_ENGINE_VERSION < 500
        InOutSkeletalMeshImportData.Points = InPoints;
#endif

        for (int32 i = 0; i < InTriangleIndices.Num(); i += GLTF_TRIANGLE_POINTS_NUM)
        {
#if GLTFFORUE_ENGINE_VERSION < 421
            VTriangle TriangleFace;
#else
            SkeletalMeshImportData::FTriangle TriangleFace;
#endif
            for (int32 j = 0; j < GLTF_TRIANGLE_POINTS_NUM; ++j)
            {
                TriangleFace.WedgeIndex[j] = InOutSkeletalMeshImportData.Wedges.Num();

                const int32 PointIndex = InTriangleIndices[i + j];
                if (InNormals.Num() == InPoints.Num())
                {
                    const FVector& Normal = InNormals[PointIndex];
#if GLTFFORUE_ENGINE_VERSION < 500
                    TriangleFace.TangentZ[j] = Normal;
#else
                    TriangleFace.TangentZ[j] = FVector3f(Normal);
#endif

                    if (InTangents.Num() == InPoints.Num())
                    {
                        const FVector4& Tangent = InTangents[PointIndex];

                        FVector WedgeTangentX(Tangent.X, Tangent.Y, Tangent.Z);

#if GLTFFORUE_ENGINE_VERSION < 500
                        TriangleFace.TangentX[j] = WedgeTangentX;
                        TriangleFace.TangentY[j] = FVector::CrossProduct(Normal, WedgeTangentX * Tangent.W);
#else
                        TriangleFace.TangentX[j] = FVector3f(WedgeTangentX);
                        TriangleFace.TangentY[j] = FVector3f(FVector::CrossProduct(Normal, WedgeTangentX * Tangent.W));
#endif
                    }
                }

#if GLTFFORUE_ENGINE_VERSION < 421
                VVertex Wedge;
#else
                SkeletalMeshImportData::FVertex Wedge;
#endif
                Wedge.VertexIndex = PointIndex;

                for (int32 k = 0; k < MAX_TEXCOORDS; ++k)
                {
#if GLTFFORUE_ENGINE_VERSION < 500
                    Wedge.UVs[k] = FVector2D::ZeroVector;
#else
                    Wedge.UVs[k] = FVector2f::ZeroVector;
#endif

                    const TArray<FVector2D>& TextureCoord = InTextureCoords[k];
                    if (PointIndex >= TextureCoord.Num())
                        continue;

#if GLTFFORUE_ENGINE_VERSION < 500
                    Wedge.UVs[k] = TextureCoord[PointIndex];
#else
                    Wedge.UVs[k] = FVector2f(TextureCoord[PointIndex]);
#endif
                }
                InOutSkeletalMeshImportData.Wedges.Add(Wedge);
            }
            TriangleFace.MatIndex        = 0;
            TriangleFace.AuxMatIndex     = 0;
            TriangleFace.SmoothingGroups = 0;
            InOutSkeletalMeshImportData.Faces.Add(TriangleFace);
        }

        // TODO: material/texture
#if GLTFFORUE_ENGINE_VERSION < 421
        VMaterial Material;
#else
        SkeletalMeshImportData::FMaterial Material;
#endif
        Material.Material           = UMaterial::GetDefaultMaterial(MD_Surface);
        Material.MaterialImportName = TEXT("Default");
        InOutSkeletalMeshImportData.Materials.Add(Material);

        InOutSkeletalMeshImportData.NumTexCoords = 0;
        for (int32 i = 0; i < MAX_TEXCOORDS; ++i)
        {
            const TArray<FVector2D>& TextureCoord = InTextureCoords[i];
            if (InPoints.Num() != TextureCoord.Num() && InTriangleIndices.Num() != TextureCoord.Num())
                continue;
            ++InOutSkeletalMeshImportData.NumTexCoords;
        }
        InOutSkeletalMeshImportData.MaxMaterialIndex = 0;
        InOutSkeletalMeshImportData.bHasVertexColors = false;
        InOutSkeletalMeshImportData.bHasNormals      = (InNormals.Num() == InPoints.Num());
        InOutSkeletalMeshImportData.bHasTangents     = (InTangents.Num() == InPoints.Num());

        return true;
    }
} // namespace glTFForUE4Ed

TSharedPtr<FglTFImporterEdSkeletalMesh> FglTFImporterEdSkeletalMesh::Get(UFactory*         InFactory, //
                                                                         UObject*          InParent,
                                                                         FName             InName,
                                                                         EObjectFlags      InFlags,
                                                                         FFeedbackContext* InFeedbackContext)
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

USkeletalMesh* FglTFImporterEdSkeletalMesh::CreateSkeletalMesh(const TWeakPtr<FglTFImporterOptions>&    InglTFImporterOptions,
                                                               const std::shared_ptr<libgltf::SGlTF>&   InGlTF,
                                                               const int32                              InNodeId,
                                                               const std::shared_ptr<libgltf::SGlTFId>& InMeshId,
                                                               const std::shared_ptr<libgltf::SGlTFId>& InSkinId,
                                                               const class FglTFBuffers&                InBuffers,
                                                               const FTransform&                        InNodeTransform,
                                                               FglTFImporterCollection&                 InOutglTFImporterCollection) const
{
    if (!InglTFImporterOptions.IsValid())
        return nullptr;
    if (!InGlTF || !InMeshId)
        return nullptr;

    const int32 glTFMeshId = *InMeshId;
    if (glTFMeshId < 0 || glTFMeshId >= static_cast<int32>(InGlTF->meshes.size()))
        return nullptr;
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

    if (!InputParent || !InputName.IsValid())
        return nullptr;

    const TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();
    check(glTFImporterOptions->Details);

    const FString MeshName         = FglTFImporter::SanitizeObjectName(GLTF_GLTFSTRING_TO_TCHAR(glTFMeshPtr->name.c_str()));
    const FString SkeletalMeshName = MeshName.IsEmpty() ? FString::Printf(TEXT("SK_%s_%d"), *InputName.ToString(), glTFMeshId)
                                                        : FString::Printf(TEXT("SK_%s_%d_%s"), *InputName.ToString(), glTFMeshId, *MeshName);

    const FText TaskName = FText::Format(LOCTEXT("BeginImportAsSkeletalMeshTask", "Importing the glTF mesh ({0}) as a skeletal mesh ({1})"),
                                         FText::AsNumber(glTFMeshId),
                                         FText::FromString(SkeletalMeshName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, TaskName, true);

    FSkeletalMeshImportData SkeletalMeshImportData;
    TArray<FMatrix>         RefBasesInvMatrix;
    TMap<int32, FString>    NodeIndexToBoneNames;
    TArray<int32>           MaterialIds;
    if (!GenerateSkeletalMeshImportData(glTFImporterOptions,
                                        InGlTF,
                                        InNodeId,
                                        glTFMeshPtr,
                                        glTFSkinPtr,
                                        InBuffers,
                                        MeshName,
                                        InNodeTransform,
                                        SkeletalMeshImportData,
                                        RefBasesInvMatrix,
                                        NodeIndexToBoneNames,
                                        MaterialIds,
                                        FeedbackTaskWrapper,
                                        InOutglTFImporterCollection))
    {
        FeedbackTaskWrapper.Log(
            ELogVerbosity::Error,
            LOCTEXT("CreateSkeletalMeshFailedToGeneateTheFSkeletalMeshImportData", "Failed to generate the `FSkeletalMeshImportData`!"));
        return nullptr;
    }

    FReferenceSkeleton RefSkeleton;
    int32              SkeletalDepth = 0;
    if (!glTFForUE4Ed::ProcessImportMeshSkeleton(SkeletalMeshImportData, nullptr, RefSkeleton, SkeletalDepth, FeedbackTaskWrapper))
    {
        FeedbackTaskWrapper.Log(ELogVerbosity::Error,
                                LOCTEXT("CreateSkeletalMeshFailedToProcessTheFSkeletalMeshImportDataAndFRefSkeleton",
                                        "Failed to process the `FSkeletalMeshImportData` and `FRefSkeleton`!"));
        return nullptr;
    }

    IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");

    const bool bShouldComputeNormals  = glTFImporterOptions->Details->bRecomputeNormals || !SkeletalMeshImportData.bHasNormals;
    const bool bShouldComputeTangents = glTFImporterOptions->Details->bRecomputeTangents || !SkeletalMeshImportData.bHasTangents;

#if GLTFFORUE_ENGINE_VERSION < 500
    TArray<FVector> LODPoints;
#else
    TArray<FVector3f> LODPoints;
#endif
#if GLTFFORUE_ENGINE_VERSION < 421
    TArray<FMeshWedge>     LODWedges;
    TArray<FMeshFace>      LODFaces;
    TArray<FVertInfluence> LODInfluences;
#else
    TArray<SkeletalMeshImportData::FMeshWedge> LODWedges;
    TArray<SkeletalMeshImportData::FMeshFace> LODFaces;
    TArray<SkeletalMeshImportData::FVertInfluence> LODInfluences;
#endif
    TArray<int32> LODPointToRawMap;
    // TODO: check the version that can't call `CopyLODImportData`
#if GLTFFORUE_ENGINE_VERSION < 423
    glTFForUE4Ed::CopyLODImportData(SkeletalMeshImportData, LODPoints, LODWedges, LODFaces, LODInfluences, LODPointToRawMap);
#else
    SkeletalMeshImportData.CopyLODImportData(LODPoints, LODWedges, LODFaces, LODInfluences, LODPointToRawMap);
#endif

#if GLTFFORUE_ENGINE_VERSION < 419
    FStaticLODModel LODModel;
#else
    FSkeletalMeshLODModel LODModel;
#endif
    LODModel.NumTexCoords = FMath::Max<uint32>(1, SkeletalMeshImportData.NumTexCoords);

    TArray<FText> WarningMessages;
    TArray<FName> WarningNames;
#if GLTFFORUE_ENGINE_VERSION < 411
    if (!MeshUtilities.BuildSkeletalMesh(LODModel,
                                         RefSkeleton,
                                         LODInfluences,
                                         LODWedges,
                                         LODFaces,
                                         LODPoints,
                                         LODPointToRawMap,
                                         false /*ImportOptions->bKeepOverlappingVertices*/,
                                         bShouldComputeNormals,
                                         bShouldComputeTangents,
                                         &WarningMessages,
                                         &WarningNames))
#elif GLTFFORUE_ENGINE_VERSION < 424
    IMeshUtilities::MeshBuildOptions BuildOptions;
    BuildOptions.bRemoveDegenerateTriangles = glTFImporterOptions->Details->bRemoveDegenerates;
    BuildOptions.bComputeNormals = bShouldComputeNormals;
    BuildOptions.bComputeTangents = bShouldComputeTangents;
    BuildOptions.bUseMikkTSpace = glTFImporterOptions->Details->bUseMikkTSpace;
    if (!MeshUtilities.BuildSkeletalMesh(
            LODModel, RefSkeleton, LODInfluences, LODWedges, LODFaces, LODPoints, LODPointToRawMap, BuildOptions, &WarningMessages, &WarningNames))
#elif GLTFFORUE_ENGINE_VERSION < 425
    LODModel.RawSkeletalMeshBulkData.SaveRawMesh(SkeletalMeshImportData);
    if (false)
#else
    if (false)
#endif
    {
        for (const FText& WarningMessage : WarningMessages)
        {
            FeedbackTaskWrapper.Log(ELogVerbosity::Warning, WarningMessage);
        }
        FeedbackTaskWrapper.Log(ELogVerbosity::Error,
                                LOCTEXT("CreateSkeletalMeshFailedToBuildTheSkeletalMesh", "Failed to build the skeletal mesh!"));
        return nullptr;
    }

    const FString NewPackagePath  = FPackageName::GetLongPackagePath(InputParent->GetPathName()) / SkeletalMeshName;
    UObject*      NewAssetPackage = InputParent;

    /// load or create new skeletal mesh
    bool           bCreated     = false;
    USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(NewAssetPackage, *SkeletalMeshName);
    if (!SkeletalMesh)
    {
        NewAssetPackage = LoadPackage(nullptr, *NewPackagePath, LOAD_None);
        if (!NewAssetPackage)
        {
#if GLTFFORUE_ENGINE_VERSION < 426
            NewAssetPackage = CreatePackage(nullptr, *NewPackagePath);
#else
            NewAssetPackage = CreatePackage(*NewPackagePath);
#endif
            checkSlow(NewAssetPackage);
        }
        if (!NewAssetPackage)
        {
            // TODO: output error
            return nullptr;
        }
        SkeletalMesh = LoadObject<USkeletalMesh>(NewAssetPackage, *SkeletalMeshName);
    }
    if (!SkeletalMesh)
    {
        /// create new skeletal mesh
        SkeletalMesh = NewObject<USkeletalMesh>(NewAssetPackage, USkeletalMesh::StaticClass(), *SkeletalMeshName, InputFlags);
        checkSlow(SkeletalMesh);
        if (SkeletalMesh)
            FAssetRegistryModule::AssetCreated(SkeletalMesh);
        bCreated = true;
    }
    else
    {
        /// clean old data
        SkeletalMesh->PreEditChange(nullptr);
        SkeletalMesh->InvalidateDeriveDataCacheGUID();
        SkeletalMesh->UnregisterAllMorphTarget();

#if GLTFFORUE_ENGINE_VERSION < 427
        SkeletalMesh->RefBasesInvMatrix.Empty();
        SkeletalMesh->Materials.Empty();
#else
        SkeletalMesh->GetRefBasesInvMatrix().Empty();
        SkeletalMesh->GetMaterials().Empty();
#endif
    }
    if (!SkeletalMesh)
        return nullptr;

#if GLTFFORUE_ENGINE_VERSION < 419
    FSkeletalMeshResource* ImportedResource = SkeletalMesh->GetImportedResource();
#else
    FSkeletalMeshModel* ImportedResource = SkeletalMesh->GetImportedModel();
#endif
    check(ImportedResource);

    {
        /// regenerate lod info
#if GLTFFORUE_ENGINE_VERSION < 420
        TArray<FSkeletalMeshLODInfo>& SkeletalMeshLODInfos = SkeletalMesh->LODInfo;
#else
        TArray<FSkeletalMeshLODInfo>& SkeletalMeshLODInfos = SkeletalMesh->GetLODInfoArray();
#endif
        SkeletalMeshLODInfos.Empty();
        SkeletalMeshLODInfos.Add(FSkeletalMeshLODInfo());

        ImportedResource->LODModels.Empty();
#if GLTFFORUE_ENGINE_VERSION < 419
        new (ImportedResource->LODModels) FStaticLODModel();
#elif GLTFFORUE_ENGINE_VERSION < 421
        new (ImportedResource->LODModels) FSkeletalMeshLODModel();
#else
        ImportedResource->LODModels.Add(new FSkeletalMeshLODModel);
#endif
    }

#if GLTFFORUE_ENGINE_VERSION < 427
    SkeletalMesh->RefSkeleton = RefSkeleton;
#else
    SkeletalMesh->SetRefSkeleton(RefSkeleton);
#endif
    SkeletalMesh->CalculateInvRefMatrices();

#if GLTFFORUE_ENGINE_VERSION < 424
    ImportedResource->LODModels[0] = LODModel;
#else
#    if GLTFFORUE_ENGINE_VERSION < 425
    ImportedResource->LODModels[0] = LODModel;
#    else
    SkeletalMesh->SaveLODImportedData(0, SkeletalMeshImportData);
#    endif
    /// use new mesh builder
    FSkeletalMeshBuildSettings BuildOptions;
    BuildOptions.bRemoveDegenerates = glTFImporterOptions->Details->bRemoveDegenerates;
    BuildOptions.bRecomputeNormals = bShouldComputeNormals;
    BuildOptions.bRecomputeTangents = bShouldComputeTangents;
    BuildOptions.bUseMikkTSpace = glTFImporterOptions->Details->bUseMikkTSpace;
    check(SkeletalMesh->GetLODInfo(0) != nullptr);
    SkeletalMesh->GetLODInfo(0)->BuildSettings = BuildOptions;
#    if GLTFFORUE_ENGINE_VERSION < 425
    IMeshBuilderModule& MeshBuilderModule = IMeshBuilderModule::Get();
#    else
    IMeshBuilderModule&                MeshBuilderModule = IMeshBuilderModule::GetForRunningPlatform();
#    endif
#    if GLTFFORUE_ENGINE_VERSION < 427
    if (!MeshBuilderModule.BuildSkeletalMesh(SkeletalMesh, 0, false))
#    else
    const FSkeletalMeshBuildParameters SkeletalMeshBuildParameters(SkeletalMesh, GetTargetPlatformManagerRef().GetRunningTargetPlatform(), 0, false);
    if (!MeshBuilderModule.BuildSkeletalMesh(SkeletalMeshBuildParameters))
#    endif
    {
        if (bCreated)
        {
#    if GLTFFORUE_ENGINE_VERSION < 500
            SkeletalMesh->MarkPendingKill();
#    else
            SkeletalMesh->MarkAsGarbage();
#    endif
        }
        FeedbackTaskWrapper.Log(ELogVerbosity::Error,
                                LOCTEXT("CreateSkeletalMeshFailedToBuildTheSkeletalMesh", "Failed to build the skeletal mesh!"));
        return nullptr;
    }
#endif

    /// import the material
    if (glTFImporterOptions->Details->bImportMaterial)
    {
        TSharedPtr<FglTFImporterEdMaterial> glTFImporterEdMaterial =
            FglTFImporterEdMaterial::Get(InputFactory, InputParent, InputName, InputFlags, FeedbackContext);
        for (const int32& MaterialId : MaterialIds)
        {
            UMaterialInterface* NewMaterial = glTFImporterEdMaterial->CreateMaterial(
                InglTFImporterOptions, InGlTF, MaterialId, InBuffers, FeedbackTaskWrapper, InOutglTFImporterCollection);
            if (!NewMaterial)
            {
                checkSlow(0);
                NewMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
            }

#if GLTFFORUE_ENGINE_VERSION < 427
            SkeletalMesh->Materials.Emplace(FSkeletalMaterial(NewMaterial));
#else
            SkeletalMesh->GetMaterials().Emplace(FSkeletalMaterial(NewMaterial));
#endif
        }
    }
    else
    {
#if GLTFFORUE_ENGINE_VERSION < 421
        for (const VMaterial& Material : SkeletalMeshImportData.Materials)
#else
        for (const SkeletalMeshImportData::FMaterial& Material : SkeletalMeshImportData.Materials)
#endif
        {
#if GLTFFORUE_ENGINE_VERSION < 427
            SkeletalMesh->Materials.Emplace(Material.Material.Get());
#else
            SkeletalMesh->GetMaterials().Emplace(Material.Material.Get());
#endif
        }
    }

    SkeletalMesh->PostEditChange();
    SkeletalMesh->MarkPackageDirty();

    /// generate the skeleton object
    FString SkeletonObjectName = FString::Printf(TEXT("%s_Skeleton"), *SkeletalMeshName);
#if GLTFFORUE_ENGINE_VERSION < 427
    USkeleton* Skeleton = SkeletalMesh->Skeleton;
#else
    USkeleton* Skeleton = SkeletalMesh->GetSkeleton();
#endif
    if (!Skeleton)
    {
        Skeleton = LoadObject<USkeleton>(NewAssetPackage, *SkeletonObjectName);
    }
    if (!Skeleton)
    {
        Skeleton = NewObject<USkeleton>(NewAssetPackage, USkeleton::StaticClass(), *SkeletonObjectName, InputFlags);
        if (Skeleton)
            FAssetRegistryModule::AssetCreated(Skeleton);
    }
    checkSlow(Skeleton);
    if (Skeleton)
    {
#if GLTFFORUE_ENGINE_VERSION < 427
        SkeletalMesh->Skeleton = Skeleton;
#else
        SkeletalMesh->SetSkeleton(Skeleton);
#endif

        Skeleton->MergeAllBonesToBoneTree(SkeletalMesh);

        FAssetNotifications::SkeletonNeedsToBeSaved(Skeleton);

        Skeleton->MarkPackageDirty();
    }

    /// import the animation
    if (glTFImporterOptions->Details->bImportAnimation)
    {
        /// generate the skeleton animation
        FglTFImporterEdAnimationSequence::Get(InputFactory, NewAssetPackage, *SkeletalMeshName, InputFlags, FeedbackContext)
            ->CreateAnimationSequence(InglTFImporterOptions,
                                      InGlTF,
                                      InBuffers,
                                      NodeIndexToBoneNames,
                                      SkeletalMeshImportData.MorphTargetNames,
                                      SkeletalMesh,
                                      Skeleton,
                                      FeedbackTaskWrapper,
                                      InOutglTFImporterCollection);
    }

    /// generate the physics object
    /// it will cause a crash when rebuild the physics asset of the skeletal mesh
    // TODO: supports to reimport the physics asset
    if (bCreated && glTFImporterOptions->Details->bCreatePhysicsAsset)
    {
        FString PhysicsObjectName = FString::Printf(TEXT("%s_Physics"), *SkeletalMeshName);
#if GLTFFORUE_ENGINE_VERSION < 427
        UPhysicsAsset* PhysicsAsset = SkeletalMesh->PhysicsAsset;
        SkeletalMesh->PhysicsAsset  = nullptr;
#else
        UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset();
        SkeletalMesh->SetPhysicsAsset(nullptr);
#endif
        if (!PhysicsAsset)
            PhysicsAsset = LoadObject<UPhysicsAsset>(NewAssetPackage, *PhysicsObjectName);
        UPhysicsAsset* NewPhysicsAsset = nullptr;
        if (!PhysicsAsset)
        {
            PhysicsAsset = NewObject<UPhysicsAsset>(NewAssetPackage, UPhysicsAsset::StaticClass(), *PhysicsObjectName, InputFlags);
            if (PhysicsAsset)
                FAssetRegistryModule::AssetCreated(PhysicsAsset);
            NewPhysicsAsset = PhysicsAsset;
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

                /// remove the useless object
                if (NewPhysicsAsset == PhysicsAsset)
#if GLTFFORUE_ENGINE_VERSION < 500
                    NewPhysicsAsset->MarkPendingKill();
#else
                    NewPhysicsAsset->MarkAsGarbage();
#endif
            }
        }
    }

    InOutglTFImporterCollection.SkeletalMeshes.Add(glTFMeshId, SkeletalMesh);
    return SkeletalMesh;
}

bool FglTFImporterEdSkeletalMesh::GenerateSkeletalMeshImportData(const TSharedPtr<FglTFImporterOptions>& InglTFImporterOptions,
                                                                 const std::shared_ptr<libgltf::SGlTF>&  InGlTF,
                                                                 const int32                             InNodeId,
                                                                 const std::shared_ptr<libgltf::SMesh>&  InMesh,
                                                                 const std::shared_ptr<libgltf::SSkin>&  InSkin,
                                                                 const FglTFBuffers&                     InBuffers,
                                                                 const FString&                          InMeshName,
                                                                 const FTransform&                       InNodeTransform,
                                                                 FSkeletalMeshImportData&                OutSkeletalMeshImportData,
                                                                 TArray<FMatrix>&                        OutInverseBindMatrices,
                                                                 TMap<int32, FString>&                   OutNodeIndexToBoneNames,
                                                                 TArray<int32>&                          InOutMaterialIds,
                                                                 const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper,
                                                                 FglTFImporterCollection&                InOutglTFImporterCollection) const
{
    for (int32 i = 0; i < static_cast<int32>(InMesh->primitives.size()); ++i)
    {
        const auto&             Primitive = InMesh->primitives[i];
        FSkeletalMeshImportData NewSkeletalMeshImportData;
        TMap<int32, FString>    NewNodeIndexToBoneNames;
        int32                   MaterialId = INDEX_NONE;
        if (!!Primitive->material)
        {
            MaterialId = (*Primitive->material);
        }
        TArray<FSkeletalMeshImportData> NewMorphTargetImportDatas;
        if (!GenerateSkeletalMeshImportData(InGlTF,
                                            InMesh,
                                            Primitive,
                                            InBuffers,
                                            NewSkeletalMeshImportData,
                                            NewMorphTargetImportDatas,
                                            NewNodeIndexToBoneNames,
                                            InFeedbackTaskWrapper,
                                            InOutglTFImporterCollection))
        {
            checkSlow(0);
            continue;
        }
        if (!glTFForUE4Ed::CheckAndMerge(NewSkeletalMeshImportData, NewNodeIndexToBoneNames, OutSkeletalMeshImportData, OutNodeIndexToBoneNames))
        {
            checkSlow(0);
        }

        if (InglTFImporterOptions->Details->bImportMorphTarget && NewMorphTargetImportDatas.Num() > 0)
        {
            if (OutSkeletalMeshImportData.MorphTargets.Num() <= 0)
            {
                OutSkeletalMeshImportData.MorphTargets = NewMorphTargetImportDatas;
            }
            else
            {
                while (OutSkeletalMeshImportData.MorphTargets.Num() < NewMorphTargetImportDatas.Num())
                {
                    OutSkeletalMeshImportData.MorphTargets.Add(
                        OutSkeletalMeshImportData.MorphTargets[OutSkeletalMeshImportData.MorphTargets.Num() - 1]);
                }
                TMap<int32, FString> NodeIndexToBoneNames;
                for (int32 j = 0; j < NewMorphTargetImportDatas.Num(); ++j)
                {
                    if (!glTFForUE4Ed::CheckAndMerge(
                            NewMorphTargetImportDatas[j], NewNodeIndexToBoneNames, OutSkeletalMeshImportData.MorphTargets[j], NodeIndexToBoneNames))
                    {
                        checkSlow(0);
                    }
                }
            }
        }

        InOutMaterialIds.Emplace(MaterialId);
    }

    /// ready for the morph target
    if (OutSkeletalMeshImportData.MorphTargets.Num() > 0)
    {
        OutSkeletalMeshImportData.MorphTargetModifiedPoints.SetNum(OutSkeletalMeshImportData.MorphTargets.Num());
        OutSkeletalMeshImportData.MorphTargetNames.SetNum(OutSkeletalMeshImportData.MorphTargets.Num());
        {
            TSet<uint32>& MorphTargetModifiedPoint = OutSkeletalMeshImportData.MorphTargetModifiedPoints[0];
            MorphTargetModifiedPoint.Empty();

            for (int32 i = 0, jc = OutSkeletalMeshImportData.Points.Num(); i < jc; ++i)
            {
                MorphTargetModifiedPoint.Add(static_cast<uint32>(i));
            }
        }
        for (int32 i = 0, ic = OutSkeletalMeshImportData.MorphTargetModifiedPoints.Num(); i < ic; ++i)
        {
            TSet<uint32>& MorphTargetModifiedPoint = OutSkeletalMeshImportData.MorphTargetModifiedPoints[i];
            MorphTargetModifiedPoint               = OutSkeletalMeshImportData.MorphTargetModifiedPoints[0];
        }
        for (int32 i = 0; i < OutSkeletalMeshImportData.MorphTargetNames.Num(); ++i)
        {
            FString& MorphTargetName = OutSkeletalMeshImportData.MorphTargetNames[i];
            MorphTargetName          = FString::Printf(TEXT("MorphTarget%d"), i);
        }
    }

    // generate the skeletal data if no skin
    if (InSkin == nullptr)
    {
        OutSkeletalMeshImportData.RefBonesBinary.SetNum(1);
#if GLTFFORUE_ENGINE_VERSION < 421
        VBone& EmptyBone = OutSkeletalMeshImportData.RefBonesBinary[0];
#else
        SkeletalMeshImportData::FBone& EmptyBone = OutSkeletalMeshImportData.RefBonesBinary[0];
#endif
        EmptyBone.Name        = FString::Printf(TEXT("Bone_%s_Root"), *InMeshName);
        EmptyBone.Flags       = 0;
        EmptyBone.NumChildren = 0;
        EmptyBone.ParentIndex = INDEX_NONE;
        EmptyBone.BonePos.Transform.SetIdentity();
        EmptyBone.BonePos.Length = 1.0f;
        EmptyBone.BonePos.XSize  = 100.0f;
        EmptyBone.BonePos.YSize  = 100.0f;
        EmptyBone.BonePos.ZSize  = 100.0f;
        OutNodeIndexToBoneNames.Add(InNodeId, EmptyBone.Name);

        OutSkeletalMeshImportData.Influences.SetNum(OutSkeletalMeshImportData.Points.Num());
        for (int32 i = 0, ic = OutSkeletalMeshImportData.Influences.Num(); i < ic; ++i)
        {
#if GLTFFORUE_ENGINE_VERSION < 421
            VRawBoneInfluence  RawBoneInfluence;
            VRawBoneInfluence& EmptyInfluence = OutSkeletalMeshImportData.Influences[i];
#else
            SkeletalMeshImportData::FRawBoneInfluence& EmptyInfluence = OutSkeletalMeshImportData.Influences[i];
#endif
            EmptyInfluence.Weight      = 1.0f;
            EmptyInfluence.VertexIndex = i;
            EmptyInfluence.BoneIndex   = 0;
        }

        OutInverseBindMatrices.SetNum(1);
        OutInverseBindMatrices[0] = FMatrix::Identity;

        /// transform the point and tangent
        if (!InNodeTransform.Equals(FTransform::Identity))
        {
            glTFForUE4Ed::TransformSkeletalMeshImportData(OutSkeletalMeshImportData, InNodeTransform);

            for (FSkeletalMeshImportData& MorphTargetImportData : OutSkeletalMeshImportData.MorphTargets)
            {
                glTFForUE4Ed::TransformSkeletalMeshImportData(MorphTargetImportData, InNodeTransform);
            }
        }
        return true;
    }
    return GenerateSkeletalMeshImportData(InGlTF,
                                          InSkin,
                                          InBuffers,
                                          InNodeTransform,
                                          OutSkeletalMeshImportData,
                                          OutInverseBindMatrices,
                                          OutNodeIndexToBoneNames,
                                          InFeedbackTaskWrapper,
                                          InOutglTFImporterCollection);
}

bool FglTFImporterEdSkeletalMesh::GenerateSkeletalMeshImportData(const std::shared_ptr<libgltf::SGlTF>&          InGlTF,
                                                                 const std::shared_ptr<libgltf::SMesh>&          InMesh,
                                                                 const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive,
                                                                 const FglTFBuffers&                             InBuffers,
                                                                 FSkeletalMeshImportData&                        OutSkeletalMeshImportData,
                                                                 TArray<FSkeletalMeshImportData>&                OutMorphTargetImportDatas,
                                                                 TMap<int32, FString>&                           OutNodeIndexToBoneNames,
                                                                 const glTFForUE4::FFeedbackTaskWrapper&         InFeedbackTaskWrapper,
                                                                 FglTFImporterCollection&                        InOutglTFImporterCollection) const
{
    if (!InMeshPrimitive)
    {
        checkSlow(0);
        return false;
    }

    TArray<uint32>           TriangleIndices;
    TArray<FVector>          Points;
    TArray<TArray<FVector>>  MorphTargetsPoints;
    TArray<FVector>          Normals;
    TArray<TArray<FVector>>  MorphTargetsNormals;
    TArray<FVector4>         Tangents;
    TArray<TArray<FVector4>> MorphTargetsTangents;
    TArray<FVector2D>        TextureCoords[MAX_TEXCOORDS];
    TArray<FVector4>         JointsIndeies[GLTF_JOINT_LAYERS_NUM_MAX];
    TArray<FVector4>         JointsWeights[GLTF_JOINT_LAYERS_NUM_MAX];
    if (!FglTFImporter::GetSkeletalMeshData(InGlTF,
                                            InMeshPrimitive,
                                            InBuffers,
                                            TriangleIndices,
                                            Points,
                                            MorphTargetsPoints,
                                            Normals,
                                            MorphTargetsNormals,
                                            Tangents,
                                            MorphTargetsTangents,
                                            TextureCoords,
                                            JointsIndeies,
                                            JointsWeights))
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to get the skeletal mesh data!"));
        return false;
    }

    if (!glTFForUE4Ed::BuildSkeletalMeshImportData(
            TriangleIndices, Points, Normals, Tangents, TextureCoords, JointsIndeies, JointsWeights, OutSkeletalMeshImportData))
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to build the skeletal mesh data!"));
        return false;
    }

    /// ready for morph target
    FglTFImporter::MergeMorphTarget<>(MorphTargetsPoints, Points, 1.0f);
    FglTFImporter::MergeMorphTarget<>(MorphTargetsNormals, Normals, 1.0f);
    FglTFImporter::MergeMorphTarget<>(MorphTargetsTangents, Tangents, 1.0f);

    /// build for morph target
    const int32 MeshWeightsCount = static_cast<int32>(InMesh->weights.size());
    if (MeshWeightsCount == MorphTargetsPoints.Num() && MeshWeightsCount == MorphTargetsNormals.Num() &&
        MeshWeightsCount == MorphTargetsTangents.Num())
    {
        OutMorphTargetImportDatas.SetNum(MeshWeightsCount);
        for (int32 i = 0, ic = OutMorphTargetImportDatas.Num(); i < ic; ++i)
        {
            if (glTFForUE4Ed::BuildSkeletalMeshImportData(TriangleIndices,
                                                          MorphTargetsPoints[i],
                                                          MorphTargetsNormals[i],
                                                          MorphTargetsTangents[i],
                                                          TextureCoords,
                                                          JointsIndeies,
                                                          JointsWeights,
                                                          OutMorphTargetImportDatas[i]))
            {
                continue;
            }
            OutMorphTargetImportDatas[i] = OutSkeletalMeshImportData;
#if GLTFFORUE_ENGINE_VERSION < 500
            OutMorphTargetImportDatas[i].Points = MorphTargetsPoints[i];
#else
            OutMorphTargetImportDatas[i].Points.SetNum(MorphTargetsPoints[i].Num());
            for (int32 j = 0, jc = OutMorphTargetImportDatas[i].Points.Num(); j < jc; ++j)
                OutMorphTargetImportDatas[i].Points[j] = FVector3f(MorphTargetsPoints[i][j]);
#endif
            OutMorphTargetImportDatas[i].PointToRawMap.Empty();
        }
    }

    return true;
}

bool FglTFImporterEdSkeletalMesh::GenerateSkeletalMeshImportData(const std::shared_ptr<libgltf::SGlTF>&  InGlTF,
                                                                 const std::shared_ptr<libgltf::SSkin>&  InSkin,
                                                                 const class FglTFBuffers&               InBuffers,
                                                                 const FTransform&                       InNodeTransform,
                                                                 FSkeletalMeshImportData&                OutSkeletalMeshImportData,
                                                                 TArray<FMatrix>&                        OutInverseBindMatrices,
                                                                 TMap<int32, FString>&                   OutNodeIndexToBoneNames,
                                                                 const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper,
                                                                 FglTFImporterCollection&                InOutglTFImporterCollection) const
{
    if (!InSkin)
    {
        checkSlow(0);
        return false;
    }

    if (!FglTFImporter::GetInverseBindMatrices(InGlTF, InSkin, InBuffers, OutInverseBindMatrices))
    {
        checkSlow(0);
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to get the inverse bind matrix!"));
        return false;
    }

    if (OutInverseBindMatrices.Num() != static_cast<int32>(InSkin->joints.size()))
    {
        checkSlow(0);
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to the number of inverse bind matrix is incorrect!"));
        return false;
    }

    const FString SkinName = GLTF_GLTFSTRING_TO_TCHAR(InSkin->name.c_str());

    /// Collect the joint id
    TArray<int32> JointIds;
    for (int32 i = 0, ic = static_cast<int32>(InSkin->joints.size()); i < ic; ++i)
    {
        const std::shared_ptr<libgltf::SGlTFId>& JointIdPtr = InSkin->joints[i];
        if (!JointIdPtr)
            return false;

        const int32 JointId = *JointIdPtr;

        checkSlow(JointId >= 0 && JointId < static_cast<int32>(InGlTF->nodes.size()));
        if (JointId < 0 || JointId >= static_cast<int32>(InGlTF->nodes.size()))
        {
            checkSlow(0);
            UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to the %d's joint id is incorrect!"), i);
            return false;
        }

        JointIds.Emplace(JointId);
    }

    const FVector NodeScale3D = InNodeTransform.GetScale3D();
    for (int32 i = 0, ic = JointIds.Num(); i < ic; ++i)
    {
        const int32 JointId = JointIds[i];

#if GLTFFORUE_ENGINE_VERSION < 421
        VBone Bone;
#else
        SkeletalMeshImportData::FBone Bone;
#endif

        const std::shared_ptr<libgltf::SNode>& NodePtr = InGlTF->nodes[JointId];
        if (!NodePtr || NodePtr->name.empty())
        {
            Bone.Name = FString::Printf(TEXT("B_%s_%d"), *SkinName, JointId);
        }
        else
        {
            FString NodeName = GLTF_GLTFSTRING_TO_TCHAR(NodePtr->name.c_str());
            Bone.Name        = FString::Printf(TEXT("B_%s_%d_%s"), *SkinName, JointId, *NodeName);
        }
        Bone.Flags       = 0;
        Bone.NumChildren = static_cast<int32>(InGlTF->nodes[JointId]->children.size());

        const FglTFImporterNodeInfo& NodeInfo = InOutglTFImporterCollection.FindNodeInfo(JointId);

        const FTransform BonePosTransform = FTransform(OutInverseBindMatrices[i].Inverse());
#if GLTFFORUE_ENGINE_VERSION < 500
        Bone.BonePos.Transform = BonePosTransform;
#else
        Bone.BonePos.Transform = ::FTransform3f(BonePosTransform);
#endif

        /// it is a root if the id is not contained in the joints
        if (!JointIds.Contains(NodeInfo.ParentIndex))
        {
            Bone.ParentIndex = INDEX_NONE;

#if GLTFFORUE_ENGINE_VERSION < 500
            Bone.BonePos.Transform *= InNodeTransform;
#else
            Bone.BonePos.Transform *= ::FTransform3f(InNodeTransform);
#endif
        }
        else
        {
            Bone.ParentIndex = NodeInfo.ParentIndex;

            int32            InverseBindMatricesParentIndex = JointIds.Find(NodeInfo.ParentIndex);
            const FTransform ParentBonePosTransform         = FTransform(OutInverseBindMatrices[InverseBindMatricesParentIndex]);
#if GLTFFORUE_ENGINE_VERSION < 500
            Bone.BonePos.Transform *= ParentBonePosTransform;
#else
            Bone.BonePos.Transform *= ::FTransform3f(ParentBonePosTransform);
#endif
        }

        // For testing / debug
        Bone.BonePos.Length = 1.0f;
        Bone.BonePos.XSize  = NodeScale3D.X;
        Bone.BonePos.YSize  = NodeScale3D.Y;
        Bone.BonePos.ZSize  = NodeScale3D.Z;

        OutSkeletalMeshImportData.RefBonesBinary.Emplace(Bone);

        OutNodeIndexToBoneNames.Add(JointId, Bone.Name);
    }

    /// convert the parent index from the inde of the joint list to the index of the bone list
    for (auto& Bone : OutSkeletalMeshImportData.RefBonesBinary)
    {
        if (Bone.ParentIndex == INDEX_NONE)
            continue;
        int32 ParentBoneIndex = INDEX_NONE;
        if (!JointIds.Find(Bone.ParentIndex, ParentBoneIndex))
            return false;
        Bone.ParentIndex = ParentBoneIndex;
    }

    /// transform the point and tangent
    if (!InNodeTransform.Equals(FTransform::Identity))
    {
        glTFForUE4Ed::TransformSkeletalMeshImportData(OutSkeletalMeshImportData, InNodeTransform);

        for (FSkeletalMeshImportData& MorphTargetImportData : OutSkeletalMeshImportData.MorphTargets)
        {
            glTFForUE4Ed::TransformSkeletalMeshImportData(MorphTargetImportData, InNodeTransform);
        }
    }
    return true;
}

#undef LOCTEXT_NAMESPACE
