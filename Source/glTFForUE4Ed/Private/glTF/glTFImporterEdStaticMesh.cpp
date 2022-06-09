// Copyright(c) 2016 - 2022 Code 4 Game, Org. All Rights Reserved.

#include "glTF/glTFImporterEdStaticMesh.h"
#include "glTFForUE4EdPrivatePCH.h"

#include "glTF/glTFImporterEdMaterial.h"
#include "glTF/glTFImporterOptions.h"

#include "AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Misc/Paths.h"
#include "RawMesh.h"
#include "RenderingThread.h"
#include "StaticMeshResources.h"

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

namespace glTFForUE4Ed
{
    bool CheckAndMerge(const FRawMesh& InFrom, FRawMesh& OutTo)
    {
        if (InFrom.WedgeIndices.Num() <= 0 || InFrom.WedgeIndices.Num() % GLTF_TRIANGLE_POINTS_NUM)
            return false;

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
} // namespace glTFForUE4Ed

TSharedPtr<FglTFImporterEdStaticMesh> FglTFImporterEdStaticMesh::Get(UFactory*         InFactory, //
                                                                     UObject*          InParent,
                                                                     FName             InName,
                                                                     EObjectFlags      InFlags,
                                                                     FFeedbackContext* InFeedbackContext)
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

UStaticMesh* FglTFImporterEdStaticMesh::CreateStaticMesh(const TWeakPtr<FglTFImporterOptions>&    InglTFImporterOptions,
                                                         const std::shared_ptr<libgltf::SGlTF>&   InGlTF,
                                                         const std::shared_ptr<libgltf::SGlTFId>& InMeshId,
                                                         const FglTFBuffers&                      InBuffers,
                                                         const FTransform&                        InNodeAbsoluteTransform,
                                                         FglTFImporterCollection&                 InOutglTFImporterCollection) const
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

    const FString MeshName       = FglTFImporter::SanitizeObjectName(GLTF_GLTFSTRING_TO_TCHAR(MeshPtr->name.c_str()));
    const FString StaticMeshName = MeshName.IsEmpty() ? FString::Printf(TEXT("SM_%s_%d"), *InputName.ToString(), MeshId)
                                                      : FString::Printf(TEXT("SM_%s_%d_%s"), *InputName.ToString(), MeshId, *MeshName);

    const FText TaskName = FText::Format(LOCTEXT("BeginImportAsStaticMeshTask", "Importing the glTF mesh ({0}) as a static mesh ({1})"),
                                         FText::AsNumber(MeshId),
                                         FText::FromString(StaticMeshName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, TaskName, true);

    FRawMesh      NewRawMesh;
    TArray<int32> glTFMaterialIds;
    if (!GenerateRawMesh(glTFImporterOptions,
                         InGlTF,
                         MeshPtr,
                         InBuffers,
                         InNodeAbsoluteTransform,
                         NewRawMesh,
                         glTFMaterialIds,
                         FeedbackTaskWrapper,
                         InOutglTFImporterCollection))
    {
        checkSlow(0);
        return nullptr;
    }

    const FString NewPackagePath  = FPackageName::GetLongPackagePath(InputParent->GetPathName()) / StaticMeshName;
    UObject*      NewAssetPackage = InputParent;

    /// load or create new static mesh
    bool         bCreated      = false;
    UStaticMesh* NewStaticMesh = LoadObject<UStaticMesh>(NewAssetPackage, *StaticMeshName);
    if (!NewStaticMesh)
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
        NewStaticMesh = LoadObject<UStaticMesh>(NewAssetPackage, *StaticMeshName);
    }
    if (!NewStaticMesh)
    {
        /// create new static mesh
        NewStaticMesh = NewObject<UStaticMesh>(NewAssetPackage, UStaticMesh::StaticClass(), *StaticMeshName, InputFlags);
        checkSlow(NewStaticMesh);
        if (NewStaticMesh)
            FAssetRegistryModule::AssetCreated(NewStaticMesh);
        bCreated = true;
    }
    if (!NewStaticMesh)
        return nullptr;

    NewStaticMesh->PreEditChange(nullptr);

#if GLTFFORUE_ENGINE_VERSION < 500
#   if GLTFFORUE_ENGINE_VERSION < 423
    TArray<FStaticMeshSourceModel>& StaticMeshSourceModels = NewStaticMesh->SourceModels;
#   else
    TArray<FStaticMeshSourceModel>& StaticMeshSourceModels = NewStaticMesh->GetSourceModels();
#   endif
    StaticMeshSourceModels.Empty();
    new (StaticMeshSourceModels) FStaticMeshSourceModel();
    FStaticMeshSourceModel& SourceModel = StaticMeshSourceModels[0];
#else
    NewStaticMesh->SetNumSourceModels(1);
    FStaticMeshSourceModel& SourceModel = NewStaticMesh->GetSourceModel(0);
#endif

    SourceModel.BuildSettings.bUseMikkTSpace = glTFImporterOptions->Details->bUseMikkTSpace;

#if GLTFFORUE_ENGINE_VERSION < 427
    NewStaticMesh->LightingGuid            = FGuid::NewGuid();
    NewStaticMesh->LightMapResolution      = 64;
    NewStaticMesh->LightMapCoordinateIndex = 1;
#else
    NewStaticMesh->SetLightingGuid(FGuid::NewGuid());
    NewStaticMesh->SetLightMapResolution(64);
    NewStaticMesh->SetLightMapCoordinateIndex(1);
#endif

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
        for (auto& Normal : NewRawMesh.WedgeTangentZ)
        {
            Normal *= -1.0f;
        }
    }

    SourceModel.BuildSettings.bRecomputeNormals =
        (glTFImporterOptions->Details->bRecomputeNormals || NewRawMesh.WedgeTangentZ.Num() != NewRawMesh.WedgeIndices.Num());
    SourceModel.BuildSettings.bRecomputeTangents =
        (glTFImporterOptions->Details->bRecomputeTangents || NewRawMesh.WedgeTangentX.Num() != NewRawMesh.WedgeIndices.Num() ||
         NewRawMesh.WedgeTangentY.Num() != NewRawMesh.WedgeIndices.Num());
    SourceModel.BuildSettings.bRemoveDegenerates    = glTFImporterOptions->Details->bRemoveDegenerates;
#if GLTFFORUE_ENGINE_VERSION < 500
    SourceModel.BuildSettings.bBuildAdjacencyBuffer = glTFImporterOptions->Details->bBuildAdjacencyBuffer;
#endif
    SourceModel.BuildSettings.bUseFullPrecisionUVs  = glTFImporterOptions->Details->bUseFullPrecisionUVs;
    SourceModel.BuildSettings.bGenerateLightmapUVs  = glTFImporterOptions->Details->bGenerateLightmapUVs;
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
#if GLTFFORUE_ENGINE_VERSION < 414
    NewStaticMesh->Materials.Empty();
#elif GLTFFORUE_ENGINE_VERSION < 427
    NewStaticMesh->StaticMaterials.Empty();
#else
    NewStaticMesh->GetStaticMaterials().Empty();
#endif

#if GLTFFORUE_ENGINE_VERSION < 423
    FMeshSectionInfoMap& StaticMeshSectionInfoMap = NewStaticMesh->SectionInfoMap;
#else
    FMeshSectionInfoMap& StaticMeshSectionInfoMap = NewStaticMesh->GetSectionInfoMap();
#endif

    TSharedPtr<FglTFImporterEdMaterial> glTFImporterEdMaterial =
        FglTFImporterEdMaterial::Get(InputFactory, InputParent, InputName, InputFlags, FeedbackContext);
    FMeshSectionInfoMap NewMap;
    static UMaterial*   DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
    for (int32 i = 0; i < glTFMaterialIds.Num(); ++i)
    {
        const int32&        glTFMaterialId = glTFMaterialIds[i];
        UMaterialInterface* NewMaterial    = nullptr;
        if (glTFImporterOptions->Details->bImportMaterial)
        {
            NewMaterial = glTFImporterEdMaterial->CreateMaterial(
                InglTFImporterOptions, InGlTF, glTFMaterialId, InBuffers, FeedbackTaskWrapper, InOutglTFImporterCollection);
        }
        if (!NewMaterial)
        {
            NewMaterial = DefaultMaterial;
        }

        FMeshSectionInfo Info = StaticMeshSectionInfoMap.Get(0, i);

#if GLTFFORUE_ENGINE_VERSION < 414
        Info.MaterialIndex = NewStaticMesh->Materials.Emplace(NewMaterial);
#elif GLTFFORUE_ENGINE_VERSION < 427
        Info.MaterialIndex = NewStaticMesh->StaticMaterials.Emplace(NewMaterial);
#else
        Info.MaterialIndex = NewStaticMesh->GetStaticMaterials().Emplace(NewMaterial);
#endif
        NewMap.Set(0, i, Info);
    }
    StaticMeshSectionInfoMap.Clear();
    StaticMeshSectionInfoMap.CopyFrom(NewMap);

    /// update the collection
    InOutglTFImporterCollection.StaticMeshes.Add(MeshId, NewStaticMesh);
    return NewStaticMesh;
}

bool FglTFImporterEdStaticMesh::GenerateRawMesh(const TSharedPtr<FglTFImporterOptions>  InglTFImporterOptions,
                                                const std::shared_ptr<libgltf::SGlTF>&  InGlTF,
                                                const std::shared_ptr<libgltf::SMesh>&  InMesh,
                                                const FglTFBuffers&                     InBuffers,
                                                const FTransform&                       InNodeAbsoluteTransform,
                                                FRawMesh&                               OutRawMesh,
                                                TArray<int32>&                          InOutglTFMaterialIds,
                                                const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper,
                                                FglTFImporterCollection&                InOutglTFImporterCollection) const
{
    if (!InMesh)
        return false;

    const FString MeshName = GLTF_GLTFSTRING_TO_TCHAR(InMesh->name.c_str());
    for (int32 i = 0; i < static_cast<int32>(InMesh->primitives.size()); ++i)
    {
        const auto& Primitive = InMesh->primitives[i];
        FRawMesh    NewRawMesh;
        int32       MaterialId = INDEX_NONE;
        if (!!Primitive->material)
        {
            MaterialId = (*Primitive->material);
        }
        if (!GenerateRawMesh(InglTFImporterOptions,
                             InGlTF,
                             InMesh,
                             Primitive,
                             InBuffers,
                             InNodeAbsoluteTransform,
                             NewRawMesh,
                             InOutglTFMaterialIds.Num(),
                             InFeedbackTaskWrapper,
                             InOutglTFImporterCollection))
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

bool FglTFImporterEdStaticMesh::GenerateRawMesh(const TSharedPtr<FglTFImporterOptions>          InglTFImporterOptions,
                                                const std::shared_ptr<libgltf::SGlTF>&          InGlTF,
                                                const std::shared_ptr<libgltf::SMesh>&          InMesh,
                                                const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive,
                                                const FglTFBuffers&                             InBuffers,
                                                const FTransform&                               InNodeAbsoluteTransform,
                                                FRawMesh&                                       OutRawMesh,
                                                int32                                           InMaterialIndex,
                                                const glTFForUE4::FFeedbackTaskWrapper&         InFeedbackTaskWrapper,
                                                FglTFImporterCollection&                        InOutglTFImporterCollection) const
{
    checkSlow(InMesh != nullptr);
    checkSlow(InMeshPrimitive != nullptr);
    checkSlow(InglTFImporterOptions.IsValid());
    if (!InMesh || !InMeshPrimitive || !InglTFImporterOptions.IsValid())
    {
        return false;
    }

    TArray<uint32>           TriangleIndices;
    TArray<FVector>          Points;
    TArray<TArray<FVector>>  MorphTargetsPoints;
    TArray<FVector>          Normals;
    TArray<TArray<FVector>>  MorphTargetsNormals;
    TArray<FVector4>         Tangents;
    TArray<TArray<FVector4>> MorphTargetsTangents;
    TArray<FVector2D>        TextureCoords[MAX_MESH_TEXTURE_COORDS];
    if (!FglTFImporter::GetStaticMeshData(InGlTF,
                                          InMeshPrimitive,
                                          InBuffers,
                                          TriangleIndices,
                                          Points,
                                          MorphTargetsPoints,
                                          Normals,
                                          MorphTargetsNormals,
                                          Tangents,
                                          MorphTargetsTangents,
                                          TextureCoords))
    {
        return false;
    }

    if (Points.Num() <= 0)
        return false;

    OutRawMesh.Empty();

    OutRawMesh.WedgeIndices.Append(TriangleIndices);

    if (InglTFImporterOptions->Details->bImportMorphTarget)
    {
        /// merge with the morph target
        FglTFImporter::MergeMorphTarget<FVector>(Points, MorphTargetsPoints, InMesh->weights);
        FglTFImporter::MergeMorphTarget<FVector>(Normals, MorphTargetsNormals, InMesh->weights);
        FglTFImporter::MergeMorphTarget<FVector4>(Tangents, MorphTargetsTangents, InMesh->weights);
    }

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

#if GLTFFORUE_ENGINE_VERSION < 500
    TArray<FVector> WedgeTangentXs;
    TArray<FVector> WedgeTangentYs;
#else
    TArray<FVector3f> WedgeTangentXs;
    TArray<FVector3f> WedgeTangentYs;
#endif
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
#if GLTFFORUE_ENGINE_VERSION < 500
            WedgeTangentXs.Add(WedgeTangentX);
#else
            WedgeTangentXs.Add(FVector3f(WedgeTangentX));
#endif

            const FVector& Normal = Normals[i];
#if GLTFFORUE_ENGINE_VERSION < 500
            WedgeTangentYs.Add(FVector::CrossProduct(Normal, WedgeTangentX * Tangent.W));
#else
            WedgeTangentYs.Add(FVector3f(FVector::CrossProduct(Normal, FVector(WedgeTangentX) * Tangent.W)));
#endif
        }
    }
    else
    {
        UE_LOG(LogglTFForUE4Ed,
               Warning,
               TEXT("Why is the number of tangent not equal with the number of "
                    "normal?"));
#if GLTFFORUE_ENGINE_VERSION < 500
        WedgeTangentXs.Init(FVector(0.0f, 0.0f, 1.0f), Normals.Num());
        WedgeTangentYs.Init(FVector(0.0f, 1.0f, 0.0f), Normals.Num());
#else
        WedgeTangentXs.Init(FVector3f(0.0f, 0.0f, 1.0f), Normals.Num());
        WedgeTangentYs.Init(FVector3f(0.0f, 1.0f, 0.0f), Normals.Num());
#endif
    }

    for (int32 i = 0; i < TriangleIndices.Num(); ++i)
    {
        OutRawMesh.WedgeTangentX.Add(WedgeTangentXs[TriangleIndices[i]]);
        OutRawMesh.WedgeTangentY.Add(WedgeTangentYs[TriangleIndices[i]]);
#if GLTFFORUE_ENGINE_VERSION < 500
        OutRawMesh.WedgeTangentZ.Add(Normals[TriangleIndices[i]]);
#else
        OutRawMesh.WedgeTangentZ.Add(FVector3f(Normals[TriangleIndices[i]]));
#endif
    }

    int32 WedgeIndicesCount = OutRawMesh.WedgeIndices.Num();
    if (TextureCoords[0].Num() <= 0)
    {
        TextureCoords[0].Init(FVector2D::ZeroVector, WedgeIndicesCount);
    }
    for (int32 i = 0; i < MAX_MESH_TEXTURE_COORDS; ++i)
    {
        if (TextureCoords[i].Num() <= 0)
            continue;
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
            if (OutRawMesh.WedgeTexCoords[i].Num() <= 0)
                continue;

            // HACK: calculate the incomplete texcoords data
            if (OutRawMesh.WedgeTexCoords[i].Num() == WedgeIndicesCount)
            {
                //
            }
            else if (OutRawMesh.WedgeTexCoords[i].Num() == OutRawMesh.VertexPositions.Num())
            {
#if GLTFFORUE_ENGINE_VERSION < 500
                const TArray<FVector2D> WedgeTexCoords = OutRawMesh.WedgeTexCoords[i];
#else
                const TArray<FVector2f> WedgeTexCoords = OutRawMesh.WedgeTexCoords[i];
#endif
                OutRawMesh.WedgeTexCoords[i].SetNumUninitialized(WedgeIndicesCount);
                for (int32 j = 0, jc = OutRawMesh.WedgeIndices.Num(); j < jc; ++j)
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
