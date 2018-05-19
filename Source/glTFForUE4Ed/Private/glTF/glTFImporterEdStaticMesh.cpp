// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEd.h"

#include "glTF/glTFImportOptions.h"

#include "libgltf/libgltf.h"

#include "RenderingThread.h"
#include "RawMesh.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4EdModule"

namespace glTFForUE4Ed
{
    bool CheckAndMerge(const FRawMesh& InFrom, FRawMesh& OutTo)
    {
        if (InFrom.WedgeIndices.Num() <= 0 || InFrom.WedgeIndices.Num() % 3) return false;

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

UStaticMesh* FglTFImporterEd::CreateStaticMesh(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::vector<std::shared_ptr<libgltf::SScene>>& InScenes, const FglTFBuffers& InBuffers) const
{
    if (!InGlTF || InScenes.empty()) return nullptr;
    if (InputClass != UStaticMesh::StaticClass() || !InputParent || !InputName.IsValid()) return nullptr;

    TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();
    FString ImportedBaseFilename = FPaths::GetBaseFilename(glTFImportOptions->FilePathInOS);

    FText TaskName = FText::Format(LOCTEXT("BeginImportMeshTask", "Importing the glTF ({0}) as a static mesh ({1})"), FText::FromName(InputName), FText::FromName(InputName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, TaskName, true);

    /// Create new static mesh
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(InputParent, InputClass, InputName, InputFlags);
    checkSlow(StaticMesh);
    if (!StaticMesh) return nullptr;

    StaticMesh->SourceModels.Empty();
    new(StaticMesh->SourceModels) FStaticMeshSourceModel();

    FStaticMeshSourceModel& SourceModel = StaticMesh->SourceModels[0];
    SourceModel.BuildSettings.bUseMikkTSpace = glTFImportOptions->bUseMikkTSpace;

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
            if (NodeId < 0 || NodeId >= InGlTF->nodes.size())
            {
                checkSlow(0);
                continue;
            }
            if (!GenerateRawMesh(InGlTF, FMatrix::Identity, InGlTF->nodes[NodeId], InBuffers, NewRawMesh, glTFMaterialInfos, FeedbackTaskWrapper))
            {
                checkSlow(0);
            }
        }
    }

    if (NewRawMesh.IsValidOrFixable())
    {
        /// Swap two axises between Y and Z
        FglTFImporter::SwapYZ(NewRawMesh.VertexPositions);
        FglTFImporter::SwapYZ(NewRawMesh.WedgeTangentX);
        FglTFImporter::SwapYZ(NewRawMesh.WedgeTangentY);
        FglTFImporter::SwapYZ(NewRawMesh.WedgeTangentZ);
        if (glTFImportOptions->bInvertNormal)
        {
            for (FVector& Normal : NewRawMesh.WedgeTangentZ)
            {
                Normal *= -1.0f;
            }
        }

        for (FVector& Position : NewRawMesh.VertexPositions)
        {
            Position = Position * glTFImportOptions->MeshScaleRatio;
        }

        SourceModel.BuildSettings.bRecomputeNormals = (glTFImportOptions->bRecomputeNormals || NewRawMesh.WedgeTangentZ.Num() != NewRawMesh.WedgeIndices.Num());
        SourceModel.BuildSettings.bRecomputeTangents = (glTFImportOptions->bRecomputeTangents || NewRawMesh.WedgeTangentX.Num() != NewRawMesh.WedgeIndices.Num() || NewRawMesh.WedgeTangentY.Num() != NewRawMesh.WedgeIndices.Num());
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

        FMeshSectionInfoMap NewMap;
        static UMaterial* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
        TMap<FString, UTexture*> TextureLibrary;
        for (int32 i = 0; i < glTFMaterialInfos.Num(); ++i)
        {
            const FglTFMaterialInfo& glTFMaterialInfo = glTFMaterialInfos[i];
            UMaterialInterface* NewMaterial = nullptr;
            if (glTFImportOptions->bImportMaterial)
            {
                NewMaterial = CreateMaterial(InglTFImportOptions, InGlTF, InBuffers, glTFMaterialInfo, TextureLibrary, FeedbackTaskWrapper);
            }
            if (!NewMaterial)
            {
                NewMaterial = DefaultMaterial;
            }

            FMeshSectionInfo Info = StaticMesh->SectionInfoMap.Get(0, i);

#if (ENGINE_MINOR_VERSION < 14)
            int32 Index = StaticMesh->Materials.Add(NewMaterial);
#else
            int32 Index = StaticMesh->StaticMaterials.Add(NewMaterial);
#endif
            Info.MaterialIndex = Index;
            NewMap.Set(0, i, Info);
        }
        StaticMesh->SectionInfoMap.Clear();
        StaticMesh->SectionInfoMap.CopyFrom(NewMap);

        if (StaticMesh->AssetImportData)
        {
            StaticMesh->AssetImportData->Update(glTFImportOptions->FilePathInOS);
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

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FMatrix& InMatrix, const std::shared_ptr<libgltf::SNode>& InNode, const FglTFBuffers& InBufferFiles, FRawMesh& OutRawMesh, TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    if (!InGlTF || !InNode) return false;

    FMatrix Matrix = InMatrix;
    if (InNode->matrix.size() == 16)
    {
        for (uint32 j = 0; j < 4; ++j)
        {
            for (uint32 i = 0; i < 4; ++i)
            {
                uint32 Index = i + j * 4;
                Matrix.M[j][i] = InNode->matrix[Index];
            }
        }
        Matrix *= InMatrix;
    }
    if (InNode->translation.size() == 3)
    {
        FVector Translation;
        Translation.X = InNode->translation[0];
        Translation.Y = InNode->translation[1];
        Translation.Z = InNode->translation[2];
        Matrix = FTranslationMatrix::Make(Translation) * Matrix;
    }
    if (InNode->rotation.size() == 4)
    {
        FQuat Rotation;
        Rotation.X = InNode->rotation[0];
        Rotation.Y = InNode->rotation[1];
        Rotation.Z = InNode->rotation[2];
        Rotation.W = InNode->rotation[3];
        Matrix = FQuatRotationMatrix::Make(Rotation) * Matrix;
    }
    if (InNode->scale.size() == 3)
    {
        FVector Scale;
        Scale.X = InNode->scale[0];
        Scale.Y = InNode->scale[1];
        Scale.Z = InNode->scale[2];
        Matrix = FScaleMatrix::Make(Scale) * Matrix;
    }

    if (!!(InNode->mesh))
    {
        const int32_t MeshId = *(InNode->mesh);
        if (MeshId < 0 || MeshId >= InGlTF->meshes.size()) return false;
        const auto& Mesh = InGlTF->meshes[MeshId];
        if (!GenerateRawMesh(InGlTF, Matrix, Mesh, InBufferFiles, OutRawMesh, InOutglTFMaterialInfos, InFeedbackTaskWrapper))
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
        if (NodeId < 0 || NodeId >= InGlTF->nodes.size())
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
        if (!GenerateRawMesh(InGlTF, Matrix, NodePtr, InBufferFiles, OutRawMesh, InOutglTFMaterialInfos, InFeedbackTaskWrapper))
        {
            checkSlow(0);
            return false;
        }
    }

    return true;
}

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FMatrix& InMatrix, const std::shared_ptr<libgltf::SMesh>& InMesh, const FglTFBuffers& InBufferFiles, FRawMesh& OutRawMesh, TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    if (!InMesh) return false;

    FString MeshName = InMesh->name.c_str();
    for (int32 i = 0; i < InMesh->primitives.size(); ++i)
    {
        const auto& Primitive = InMesh->primitives[i];
        FRawMesh NewRawMesh;
        int32 MaterialId = INDEX_NONE;
        if (!!Primitive->material)
        {
            MaterialId = (*Primitive->material);
        }
        if (!GenerateRawMesh(InGlTF, InMatrix, Primitive, InBufferFiles, NewRawMesh, InOutglTFMaterialInfos.Num(), InFeedbackTaskWrapper))
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

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FMatrix& InMatrix, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, FRawMesh& OutRawMesh, int32 InMaterialIndex, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
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
    if (!GetMeshData(InGlTF, InMeshPrimitive, InBufferFiles, TriangleIndices, Points, Normals, Tangents, TextureCoords))
    {
        return false;
    }

    if (Points.Num() <= 0) return false;

    OutRawMesh.Empty();

    OutRawMesh.WedgeIndices.Append(TriangleIndices);

    FTransform Transform(InMatrix);
    for (FVector& Point : Points)
    {
        Point = Transform.TransformPosition(Point);
    }
    OutRawMesh.VertexPositions.Append(Points);

    if (Normals.Num() == Points.Num())
    {
        for (FVector& Normal : Normals)
        {
            Normal = Transform.GetRotation().RotateVector(Normal);
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
            WedgeTangentX = Transform.GetRotation().RotateVector(WedgeTangentX);
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

    if (WedgeIndicesCount > 0 && (WedgeIndicesCount % 3) == 0)
    {
        int32 TriangleCount = OutRawMesh.WedgeIndices.Num() / 3;
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
