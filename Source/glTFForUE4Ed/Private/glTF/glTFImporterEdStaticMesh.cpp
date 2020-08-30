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

TSharedPtr<FglTFImporterEdStaticMesh> FglTFImporterEdStaticMesh::Get(UFactory* InFactory, UObject* InParent, FName InName, EObjectFlags InFlags, FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEdStaticMesh> glTFImporterEdStaticMesh = MakeShareable(new FglTFImporterEdStaticMesh);
    glTFImporterEdStaticMesh->Set(InParent, InName, InFlags, InFeedbackContext);
    glTFImporterEdStaticMesh->InputFactory = InFactory;
    return glTFImporterEdStaticMesh;
}

FglTFImporterEdStaticMesh::FglTFImporterEdStaticMesh()
    : Super()
{
    //
}

FglTFImporterEdStaticMesh::~FglTFImporterEdStaticMesh()
{
    //
}

UStaticMesh* FglTFImporterEdStaticMesh::CreateStaticMesh(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions
    , const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SGlTFId>& InMeshId, const FglTFBuffers& InBuffers
    , const FTransform& InNodeAbsoluteTransform, FglTFImporterCollection& InOutglTFImporterCollection) const
{
    if (!InGlTF || !InMeshId)
    {
        checkfSlow(0, TEXT("The glTF data is invalid!"));
        return nullptr;
    }
    const int32_t MeshId = *InMeshId;

    /// try to find the static mesh from the collection by the mesh id
    if (InOutglTFImporterCollection.StaticMeshes.Contains(MeshId))
    {
        return InOutglTFImporterCollection.StaticMeshes[MeshId];
    }
    if (MeshId < 0 || MeshId >= static_cast<int32>(InGlTF->meshes.size()))
    {
        checkfSlow(0, TEXT("The glTF mesh's id is invalid!"));
        return nullptr;
    }
    const std::shared_ptr<libgltf::SMesh>& MeshPtr = InGlTF->meshes[MeshId];
    if (!MeshPtr)
    {
        checkfSlow(0, TEXT("The glTF mesh is invalid!"));
        return nullptr;
    }
    if (!InputParent || !InputName.IsValid())
    {
        checkfSlow(0, TEXT("The input class is invalid"));
        return nullptr;
    }

    const TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();
    check(glTFImporterOptions->Details);

    const FString MeshName = GLTF_GLTFSTRING_TO_TCHAR(MeshPtr->name.c_str());
    const FString StaticMeshName = MeshName.IsEmpty()
        ? FString::Printf(TEXT("SM_%s_%d"), *InputName.ToString(), MeshId)
        : FString::Printf(TEXT("SM_%s_%d_%s"), *InputName.ToString(), MeshId, *MeshName);

    const FText TaskName = FText::Format(LOCTEXT("BeginImportAsStaticMeshTask", "Importing the glTF mesh ({0}) as a static mesh ({1})"), FText::AsNumber(MeshId), FText::FromString(StaticMeshName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, TaskName, true);

    FRawMesh NewRawMesh;
    TArray<int32> glTFMaterialIds;
    if (!GenerateRawMesh(InGlTF, MeshPtr, InBuffers
        , InNodeAbsoluteTransform, NewRawMesh, glTFMaterialIds
        , FeedbackTaskWrapper, InOutglTFImporterCollection))
    {
        checkSlow(0);
        return nullptr;
    }

    bool bCreated = false;
    UStaticMesh* NewStaticMesh = FindObject<UStaticMesh>(InputParent, *StaticMeshName);
    if (!NewStaticMesh)
    {
        /// Create a new static mesh
        NewStaticMesh = NewObject<UStaticMesh>(InputParent, UStaticMesh::StaticClass(), *StaticMeshName, InputFlags);
        checkSlow(NewStaticMesh);
        if (NewStaticMesh) FAssetRegistryModule::AssetCreated(NewStaticMesh);
        bCreated = true;
    }
    if (!NewStaticMesh) return nullptr;

    NewStaticMesh->PreEditChange(nullptr);

#if ENGINE_MINOR_VERSION <= 22
    TArray<FStaticMeshSourceModel>& StaticMeshSourceModels = NewStaticMesh->SourceModels;
#else
    TArray<FStaticMeshSourceModel>& StaticMeshSourceModels = NewStaticMesh->GetSourceModels();
#endif

    StaticMeshSourceModels.Empty();
    new(StaticMeshSourceModels)FStaticMeshSourceModel();
    FStaticMeshSourceModel& SourceModel = StaticMeshSourceModels[0];
    SourceModel.BuildSettings.bUseMikkTSpace = glTFImporterOptions->Details->bUseMikkTSpace;

    NewStaticMesh->LightingGuid = FGuid::NewGuid();
    NewStaticMesh->LightMapResolution = 64;
    NewStaticMesh->LightMapCoordinateIndex = 1;

    if (!NewRawMesh.IsValidOrFixable())
    {
        // destroy new object
        if (bCreated)
        {
            NewStaticMesh->ConditionalBeginDestroy();
            NewStaticMesh = nullptr;
        }
        return NewStaticMesh;
    }

    if (glTFImporterOptions->Details->bInvertNormal)
    {
        for (FVector& Normal : NewRawMesh.WedgeTangentZ)
        {
            Normal *= -1.0f;
        }
    }

    SourceModel.BuildSettings.bRecomputeNormals = (glTFImporterOptions->Details->bRecomputeNormals || NewRawMesh.WedgeTangentZ.Num() != NewRawMesh.WedgeIndices.Num());
    SourceModel.BuildSettings.bRecomputeTangents = (glTFImporterOptions->Details->bRecomputeTangents || NewRawMesh.WedgeTangentX.Num() != NewRawMesh.WedgeIndices.Num() || NewRawMesh.WedgeTangentY.Num() != NewRawMesh.WedgeIndices.Num());
    SourceModel.BuildSettings.bRemoveDegenerates = glTFImporterOptions->Details->bRemoveDegenerates;
    SourceModel.BuildSettings.bBuildAdjacencyBuffer = glTFImporterOptions->Details->bBuildAdjacencyBuffer;
    SourceModel.BuildSettings.bUseFullPrecisionUVs = glTFImporterOptions->Details->bUseFullPrecisionUVs;
    SourceModel.BuildSettings.bGenerateLightmapUVs = glTFImporterOptions->Details->bGenerateLightmapUVs;
    SourceModel.RawMeshBulkData->SaveRawMesh(NewRawMesh);

    /// Build the static mesh
    TArray<FText> BuildErrors;
    NewStaticMesh->Build(false, &BuildErrors);
    if (BuildErrors.Num() > 0)
    {
        FeedbackTaskWrapper.Log(ELogVerbosity::Warning, LOCTEXT("StaticMeshBuildHasError", "Failed to build the static mesh!"));
        for (const FText& BuildError : BuildErrors)
        {
            FeedbackTaskWrapper.Log(ELogVerbosity::Warning, BuildError);
        }
    }

    // clear old materials
#if (ENGINE_MINOR_VERSION <= 13)
    NewStaticMesh->Materials.Empty();
#else
    NewStaticMesh->StaticMaterials.Empty();
#endif

#if ENGINE_MINOR_VERSION <= 22
    FMeshSectionInfoMap& StaticMeshSectionInfoMap = NewStaticMesh->SectionInfoMap;
#else
    FMeshSectionInfoMap& StaticMeshSectionInfoMap = NewStaticMesh->GetSectionInfoMap();
#endif

    TSharedPtr<FglTFImporterEdMaterial> glTFImporterEdMaterial = FglTFImporterEdMaterial::Get(InputFactory, InputParent, InputName, InputFlags, FeedbackContext);
    FMeshSectionInfoMap NewMap;
    static UMaterial* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
    int32 MaterialIndex = 0;
    for (int32 i = 0; i < glTFMaterialIds.Num(); ++i)
    {
        const int32& glTFMaterialId = glTFMaterialIds[i];
        UMaterialInterface* NewMaterial = nullptr;
        if (glTFImporterOptions->Details->bImportMaterial)
        {
            NewMaterial = glTFImporterEdMaterial->CreateMaterial(InglTFImporterOptions
                , InGlTF, glTFMaterialId, InBuffers, FeedbackTaskWrapper
                , InOutglTFImporterCollection);
        }
        if (!NewMaterial)
        {
            NewMaterial = DefaultMaterial;
        }

        FMeshSectionInfo Info = StaticMeshSectionInfoMap.Get(0, i);

#if (ENGINE_MINOR_VERSION <= 13)
        MaterialIndex = NewStaticMesh->Materials.Emplace(NewMaterial);
#else
        MaterialIndex = NewStaticMesh->StaticMaterials.Emplace(NewMaterial);
#endif
        Info.MaterialIndex = MaterialIndex;
        NewMap.Set(0, i, Info);
    }
    StaticMeshSectionInfoMap.Clear();
    StaticMeshSectionInfoMap.CopyFrom(NewMap);

    /// update the collection
    InOutglTFImporterCollection.StaticMeshes.Add(MeshId, NewStaticMesh);
    return NewStaticMesh;
}

bool FglTFImporterEdStaticMesh::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMesh>& InMesh, const FglTFBuffers& InBuffers
    , const FTransform& InNodeAbsoluteTransform, FRawMesh& OutRawMesh, TArray<int32>& InOutglTFMaterialIds
    , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper, FglTFImporterCollection& InOutglTFImporterCollection) const
{
    if (!InMesh) return false;

    const FString MeshName = GLTF_GLTFSTRING_TO_TCHAR(InMesh->name.c_str());
    for (int32 i = 0; i < static_cast<int32>(InMesh->primitives.size()); ++i)
    {
        const auto& Primitive = InMesh->primitives[i];
        FRawMesh NewRawMesh;
        int32 MaterialId = INDEX_NONE;
        if (!!Primitive->material)
        {
            MaterialId = (*Primitive->material);
        }
        if (!GenerateRawMesh(InGlTF, Primitive, InBuffers, InNodeAbsoluteTransform, NewRawMesh, InOutglTFMaterialIds.Num(), InFeedbackTaskWrapper, InOutglTFImporterCollection))
        {
            checkSlow(0);
            continue;
        }
        if (!glTFForUE4Ed::CheckAndMerge(NewRawMesh, OutRawMesh))
        {
            checkSlow(0);
            continue;
        }

        InOutglTFMaterialIds.Add(MaterialId);
    }
    return true;
}

bool FglTFImporterEdStaticMesh::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBuffers
    , const FTransform& InNodeAbsoluteTransform, FRawMesh& OutRawMesh, int32 InMaterialIndex
    , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper, FglTFImporterCollection& InOutglTFImporterCollection) const
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

    const bool bNodeAbsoluteTransformIsIdentity = InNodeAbsoluteTransform.Equals(FTransform::Identity);
    if (!bNodeAbsoluteTransformIsIdentity)
    {
        for (FVector& Point : Points)
        {
            Point = InNodeAbsoluteTransform.TransformPosition(Point);
        }
    }
    OutRawMesh.VertexPositions.Append(Points);

    if (Normals.Num() == Points.Num())
    {
        if (!bNodeAbsoluteTransformIsIdentity)
        {
            for (FVector& Normal : Normals)
            {
                Normal = InNodeAbsoluteTransform.GetRotation().RotateVector(Normal);
            }
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
            if (!bNodeAbsoluteTransformIsIdentity)
            {
                WedgeTangentX = InNodeAbsoluteTransform.GetRotation().RotateVector(WedgeTangentX);
            }
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
