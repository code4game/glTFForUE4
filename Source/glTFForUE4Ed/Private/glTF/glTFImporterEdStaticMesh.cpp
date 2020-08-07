// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEdStaticMesh.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporterEdMaterial.h"

#include "RenderingThread.h"
#include "RawMesh.h"
#include "StaticMeshResources.h"
#include "AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

namespace glTFForUE4Ed
{
    bool CheckAndMerge(const FRawMesh& InFrom, FRawMesh& OutTo)
    {
        if (InFrom.WedgeIndices.Num() <= 0 || InFrom.WedgeIndices.Num() % GLTF_TRIANGLE_POINTS_NUM) return false;

        OutTo.FaceMaterialIndices.Append(InFrom.FaceMaterialIndices);
        OutTo.FaceSmoothingMasks.Append(InFrom.FaceSmoothingMasks);
        int32 StartIndex = OutTo.VertexPositions.Num();
        for (int32 WedgeIndex : InFrom.WedgeIndices)
        {
            OutTo.WedgeIndices.Add(WedgeIndex + StartIndex);
        }
        OutTo.VertexPositions.Append(InFrom.VertexPositions);
        OutTo.WedgeTangentX.Append(InFrom.WedgeTangentX);
        OutTo.WedgeTangentY.Append(InFrom.WedgeTangentY);
        OutTo.WedgeTangentZ.Append(InFrom.WedgeTangentZ);
        for (int32 i = 0; i < MAX_MESH_TEXTURE_COORDS; ++i)
        {
            OutTo.WedgeTexCoords[i].Append(InFrom.WedgeTexCoords[i]);
        }
        OutTo.WedgeColors.Append(InFrom.WedgeColors);
        return true;
    }
}

TSharedPtr<FglTFImporterEdStaticMesh> FglTFImporterEdStaticMesh::Get(UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEdStaticMesh> glTFImporterEdStaticMesh = MakeShareable(new FglTFImporterEdStaticMesh);
    glTFImporterEdStaticMesh->Set(InClass, InParent, InName, InFlags, InFeedbackContext);
    glTFImporterEdStaticMesh->InputFactory = InFactory;
    return glTFImporterEdStaticMesh;
}

FglTFImporterEdStaticMesh::FglTFImporterEdStaticMesh()
{
    //
}

FglTFImporterEdStaticMesh::~FglTFImporterEdStaticMesh()
{
    //
}

UStaticMesh* FglTFImporterEdStaticMesh::CreateStaticMesh(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::vector<std::shared_ptr<libgltf::SScene>>& InScenes, const FglTFBuffers& InBuffers) const
{
    if (!InGlTF || InScenes.empty()) return nullptr;
    if (InputClass != UStaticMesh::StaticClass() || !InputParent || !InputName.IsValid()) return nullptr;

    TArray<int32> NodeParentIndices;
    TArray<FTransform> NodeRelativeTransforms;
    TArray<FTransform> NodeAbsoluteTransforms;
    if (!FglTFImporter::GetNodeParentIndicesAndTransforms(InGlTF, NodeParentIndices, NodeRelativeTransforms, NodeAbsoluteTransforms)) return nullptr;

    TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();
    FString ImportedBaseFilename = FPaths::GetBaseFilename(glTFImporterOptions->FilePathInOS);

    FText TaskName = FText::Format(LOCTEXT("BeginImportAsStaticMeshTask", "Importing the glTF ({0}) as a static mesh ({1})"), FText::FromName(InputName), FText::FromName(InputName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, TaskName, true);

    /// Create new static mesh
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(InputParent, InputClass, InputName, InputFlags);
    checkSlow(StaticMesh);
    if (!StaticMesh) return nullptr;
    FAssetRegistryModule::AssetCreated(StaticMesh);

    StaticMesh->PreEditChange(nullptr);

#if ENGINE_MINOR_VERSION < 23
    TArray<FStaticMeshSourceModel>& StaticMeshSourceModels = StaticMesh->SourceModels;
#else
    TArray<FStaticMeshSourceModel>& StaticMeshSourceModels = StaticMesh->GetSourceModels();
#endif

    StaticMeshSourceModels.Empty();
    new(StaticMeshSourceModels)FStaticMeshSourceModel();

    FStaticMeshSourceModel& SourceModel = StaticMeshSourceModels[0];
    SourceModel.BuildSettings.bUseMikkTSpace = glTFImporterOptions->bUseMikkTSpace;

    StaticMesh->LightingGuid = FGuid::NewGuid();
    StaticMesh->LightMapResolution = 64;
    StaticMesh->LightMapCoordinateIndex = 1;

    FRawMesh NewRawMesh;
    SourceModel.RawMeshBulkData->LoadRawMesh(NewRawMesh);
    NewRawMesh.Empty();

    TArray<FglTFMaterialInfo> glTFMaterialInfos;
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
            if (!GenerateRawMesh(InGlTF, InGlTF->nodes[NodeId], NodeAbsoluteTransforms[NodeId], NodeAbsoluteTransforms, InBuffers, NewRawMesh, glTFMaterialInfos, FeedbackTaskWrapper))
            {
                checkSlow(0);
            }
        }
    }

    if (NewRawMesh.IsValidOrFixable())
    {
        if (glTFImporterOptions->bInvertNormal)
        {
            for (FVector& Normal : NewRawMesh.WedgeTangentZ)
            {
                Normal *= -1.0f;
            }
        }

        for (FVector& Position : NewRawMesh.VertexPositions)
        {
            Position = Position * glTFImporterOptions->MeshScaleRatio;
        }

        SourceModel.BuildSettings.bRecomputeNormals = (glTFImporterOptions->bRecomputeNormals || NewRawMesh.WedgeTangentZ.Num() != NewRawMesh.WedgeIndices.Num());
        SourceModel.BuildSettings.bRecomputeTangents = (glTFImporterOptions->bRecomputeTangents || NewRawMesh.WedgeTangentX.Num() != NewRawMesh.WedgeIndices.Num() || NewRawMesh.WedgeTangentY.Num() != NewRawMesh.WedgeIndices.Num());
        SourceModel.BuildSettings.bRemoveDegenerates = false;
        SourceModel.BuildSettings.bBuildAdjacencyBuffer = false;
        SourceModel.BuildSettings.bUseFullPrecisionUVs = false;
        SourceModel.BuildSettings.bGenerateLightmapUVs = false;
        SourceModel.RawMeshBulkData->SaveRawMesh(NewRawMesh);

        /// Build the static mesh
        TArray<FText> BuildErrors;
        StaticMesh->Build(false, &BuildErrors);
        if (BuildErrors.Num() > 0)
        {
            FeedbackTaskWrapper.Log(ELogVerbosity::Warning, LOCTEXT("StaticMeshBuildHasError", "Failed to build the static mesh!"));
            for (const FText& BuildError : BuildErrors)
            {
                FeedbackTaskWrapper.Log(ELogVerbosity::Warning, BuildError);
            }
        }

#if ENGINE_MINOR_VERSION < 23
        FMeshSectionInfoMap& StaticMeshSectionInfoMap = StaticMesh->SectionInfoMap;
#else
        FMeshSectionInfoMap& StaticMeshSectionInfoMap = StaticMesh->GetSectionInfoMap();
#endif

        TSharedPtr<FglTFImporterEdMaterial> glTFImporterEdMaterial = FglTFImporterEdMaterial::Get(InputFactory, InputClass, InputParent, InputName, InputFlags, FeedbackContext);
        FMeshSectionInfoMap NewMap;
        static UMaterial* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
        TMap<FString, UTexture*> TextureLibrary;
        for (int32 i = 0; i < glTFMaterialInfos.Num(); ++i)
        {
            const FglTFMaterialInfo& glTFMaterialInfo = glTFMaterialInfos[i];
            UMaterialInterface* NewMaterial = nullptr;
            if (glTFImporterOptions->bImportMaterial)
            {
                NewMaterial = glTFImporterEdMaterial->CreateMaterial(InglTFImporterOptions, InGlTF, InBuffers, glTFMaterialInfo, TextureLibrary, FeedbackTaskWrapper);
            }
            if (!NewMaterial)
            {
                NewMaterial = DefaultMaterial;
            }

            FMeshSectionInfo Info = StaticMeshSectionInfoMap.Get(0, i);

#if (ENGINE_MINOR_VERSION < 14)
            int32 Index = StaticMesh->Materials.Add(NewMaterial);
#else
            int32 Index = StaticMesh->StaticMaterials.Add(NewMaterial);
#endif
            Info.MaterialIndex = Index;
            NewMap.Set(0, i, Info);
        }
        StaticMeshSectionInfoMap.Clear();
        StaticMeshSectionInfoMap.CopyFrom(NewMap);

        if (StaticMesh->AssetImportData)
        {
            StaticMesh->AssetImportData->Update(glTFImporterOptions->FilePathInOS);
        }

        StaticMesh->PostEditChange();
        StaticMesh->MarkPackageDirty();
    }
    else
    {
        StaticMesh->ConditionalBeginDestroy();
        StaticMesh = nullptr;
    }
    return StaticMesh;
}

bool FglTFImporterEdStaticMesh::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF
    , const std::shared_ptr<libgltf::SNode>& InNode
    , const FTransform& InNodeAbsoluteTransform
    , const TArray<FTransform>& InNodeAbsoluteTransforms
    , const FglTFBuffers& InBuffers
    , FRawMesh& OutRawMesh
    , TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos
    , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    if (!InGlTF || !InNode) return false;

    if (!!(InNode->mesh))
    {
        const int32_t MeshId = *(InNode->mesh);
        if (MeshId < 0 || MeshId >= static_cast<int32>(InGlTF->meshes.size())) return false;
        const auto& Mesh = InGlTF->meshes[MeshId];
        if (!GenerateRawMesh(InGlTF, Mesh, InNodeAbsoluteTransform, InBuffers, OutRawMesh, InOutglTFMaterialInfos, InFeedbackTaskWrapper))
        {
            checkSlow(0);
            return false;
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
        if (!GenerateRawMesh(InGlTF, NodePtr, InNodeAbsoluteTransforms[NodeId], InNodeAbsoluteTransforms, InBuffers, OutRawMesh, InOutglTFMaterialInfos, InFeedbackTaskWrapper))
        {
            checkSlow(0);
            return false;
        }
    }

    return true;
}

bool FglTFImporterEdStaticMesh::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF
    , const std::shared_ptr<libgltf::SMesh>& InMesh
    , const FTransform& InNodeAbsoluteTransform
    , const FglTFBuffers& InBuffers
    , FRawMesh& OutRawMesh
    , TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos
    , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    if (!InMesh) return false;

    FString MeshName = InMesh->name.c_str();
    for (int32 i = 0; i < static_cast<int32>(InMesh->primitives.size()); ++i)
    {
        const auto& Primitive = InMesh->primitives[i];
        FRawMesh NewRawMesh;
        int32 MaterialId = INDEX_NONE;
        if (!!Primitive->material)
        {
            MaterialId = (*Primitive->material);
        }
        if (!GenerateRawMesh(InGlTF, Primitive, InNodeAbsoluteTransform, InBuffers, NewRawMesh, InOutglTFMaterialInfos.Num(), InFeedbackTaskWrapper))
        {
            checkSlow(0);
            continue;
        }
        if (!glTFForUE4Ed::CheckAndMerge(NewRawMesh, OutRawMesh))
        {
            checkSlow(0);
            continue;
        }
        FString PrimitiveName = FString::Printf(TEXT("%s%d"), *MeshName, i);
        PrimitiveName = FglTFImporter::SanitizeObjectName(PrimitiveName);
        InOutglTFMaterialInfos.Add(FglTFMaterialInfo(MaterialId, PrimitiveName));
    }
    return true;
}

bool FglTFImporterEdStaticMesh::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF
    , const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive
    , const FTransform& InNodeAbsoluteTransform
    , const FglTFBuffers& InBuffers
    , FRawMesh& OutRawMesh
    , int32 InMaterialIndex
    , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    if (!InMeshPrimitive)
    {
        checkSlow(0);
        return false;
    }

    TArray<uint32> TriangleIndices;
    TArray<FVector> Points;
    TArray<FVector> Normals;
    TArray<FVector4> Tangents;
    TArray<FVector2D> TextureCoords[MAX_MESH_TEXTURE_COORDS];
    if (!FglTFImporter::GetStaticMeshData(InGlTF, InMeshPrimitive, InBuffers, TriangleIndices, Points, Normals, Tangents, TextureCoords))
    {
        return false;
    }

    if (Points.Num() <= 0) return false;

    OutRawMesh.Empty();

    OutRawMesh.WedgeIndices.Append(TriangleIndices);

    for (FVector& Point : Points)
    {
        Point = InNodeAbsoluteTransform.TransformPosition(Point);
    }
    OutRawMesh.VertexPositions.Append(Points);

    if (Normals.Num() == Points.Num())
    {
        for (FVector& Normal : Normals)
        {
            Normal = InNodeAbsoluteTransform.GetRotation().RotateVector(Normal);
        }
    }
    else
    {
        Normals.Init(FVector(1.0f, 0.0f, 0.0f), Points.Num());
    }

    TArray<FVector> WedgeTangentXs;
    TArray<FVector> WedgeTangentYs;
    if (Tangents.Num() == Points.Num())
    {
        for (int32 i = 0; i < Tangents.Num(); ++i)
        {
            const FVector4& Tangent = Tangents[i];

            FVector WedgeTangentX(Tangent.X, Tangent.Y, Tangent.Z);
            WedgeTangentX = InNodeAbsoluteTransform.GetRotation().RotateVector(WedgeTangentX);
            WedgeTangentXs.Add(WedgeTangentX);

            const FVector& Normal = Normals[i];
            WedgeTangentYs.Add(FVector::CrossProduct(Normal, WedgeTangentX * Tangent.W));
        }
    }
    else
    {
        UE_LOG(LogglTFForUE4Ed, Warning, TEXT("Why is the number of tangent not equal with the number of normal?"));
        WedgeTangentXs.Init(FVector(0.0f, 0.0f, 1.0f), Normals.Num());
        WedgeTangentYs.Init(FVector(0.0f, 1.0f, 0.0f), Normals.Num());
    }

    for (int32 i = 0; i < TriangleIndices.Num(); ++i)
    {
        OutRawMesh.WedgeTangentX.Add(WedgeTangentXs[TriangleIndices[i]]);
        OutRawMesh.WedgeTangentY.Add(WedgeTangentYs[TriangleIndices[i]]);
        OutRawMesh.WedgeTangentZ.Add(Normals[TriangleIndices[i]]);
    }

    int32 WedgeIndicesCount = OutRawMesh.WedgeIndices.Num();
    if (TextureCoords[0].Num() <= 0)
    {
        TextureCoords[0].Init(FVector2D::ZeroVector, WedgeIndicesCount);
    }
    for (int32 i = 0; i < MAX_MESH_TEXTURE_COORDS; ++i)
    {
        if (TextureCoords[i].Num() <= 0) continue;
        OutRawMesh.WedgeTexCoords[i].Append(TextureCoords[i]);
    }

    if (WedgeIndicesCount > 0 && (WedgeIndicesCount % GLTF_TRIANGLE_POINTS_NUM) == 0)
    {
        int32 TriangleCount = OutRawMesh.WedgeIndices.Num() / GLTF_TRIANGLE_POINTS_NUM;
        if (OutRawMesh.FaceMaterialIndices.Num() <= 0)
        {
            OutRawMesh.FaceMaterialIndices.Init(InMaterialIndex, TriangleCount);
        }
        if (OutRawMesh.FaceSmoothingMasks.Num() <= 0)
        {
            OutRawMesh.FaceSmoothingMasks.Init(1, TriangleCount);
        }
        for (int32 i = 0; i < MAX_MESH_TEXTURE_COORDS; ++i)
        {
            if (OutRawMesh.WedgeTexCoords[i].Num() <= 0) continue;

            //HACK: calculate the incomplete texcoords data
            if (OutRawMesh.WedgeTexCoords[i].Num() == WedgeIndicesCount)
            {
                //
            }
            else if (OutRawMesh.WedgeTexCoords[i].Num() == OutRawMesh.VertexPositions.Num())
            {
                TArray<FVector2D> WedgeTexCoords = OutRawMesh.WedgeTexCoords[i];
                OutRawMesh.WedgeTexCoords[i].SetNumUninitialized(WedgeIndicesCount);
                for (int32 j = 0; j < OutRawMesh.WedgeIndices.Num(); ++j)
                {
                    OutRawMesh.WedgeTexCoords[i][j] = WedgeTexCoords[OutRawMesh.WedgeIndices[j] % WedgeTexCoords.Num()];
                }
            }
            else
            {
                OutRawMesh.WedgeTexCoords[i].SetNumZeroed(WedgeIndicesCount);
            }
        }
    }
    return OutRawMesh.IsValidOrFixable();
}

#undef LOCTEXT_NAMESPACE
