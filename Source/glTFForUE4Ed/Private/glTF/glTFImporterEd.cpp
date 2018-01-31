// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEd.h"

#include "glTF/glTFImportOptions.h"

#include "libgltf/libgltf.h"

#include "RenderingThread.h"
#include "RawMesh.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Runtime/Launch/Resources/Version.h"

#define LOCTEXT_NAMESPACE "FglTFForUE4EdModule"

namespace glTFForUE4Ed
{
    void SwapYZ(FVector& InOutValue)
    {
        float Temp = InOutValue.Y;
        InOutValue.Y = InOutValue.Z;
        InOutValue.Z = Temp;
    }

    void SwapYZ(TArray<FVector>& InOutValues)
    {
        for (FVector& Value : InOutValues)
        {
            SwapYZ(Value);
        }
    }

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

TSharedPtr<FglTFImporterEd> FglTFImporterEd::Get(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEd> glTFImporterEd = MakeShareable(new FglTFImporterEd);
    glTFImporterEd->Set(InClass, InParent, InName, InFlags, InFeedbackContext);
    return glTFImporterEd;
}

FglTFImporterEd::FglTFImporterEd()
    : Super()
{
    //
}

FglTFImporterEd::~FglTFImporterEd()
{
    //
}

UObject* FglTFImporterEd::Create(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF) const
{
    if (!InGlTF)
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Invalid InGlTF!"));
        return nullptr;
    }

    if (!InGlTF->asset || InGlTF->asset->version != TEXT("2.0"))
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Invalid version: %s!"), !(InGlTF->asset) ? TEXT("none") : InGlTF->asset->version.c_str());
        return nullptr;
    }

    TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();

    const FString FolderPathInOS = FPaths::GetPath(glTFImportOptions->FilePathInOS);
    FglTFBufferFiles BufferFiles(FolderPathInOS, InGlTF->buffers);

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

    return CreateStaticMesh(InglTFImportOptions, InGlTF, Scenes, BufferFiles);
}

UStaticMesh* FglTFImporterEd::CreateStaticMesh(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::vector<std::shared_ptr<libgltf::SScene>>& InScenes, const FglTFBufferFiles& InBufferFiles) const
{
    if (!InGlTF || InScenes.empty()) return nullptr;
    if (InputClass != UStaticMesh::StaticClass() || !InputParent || !InputName.IsValid()) return nullptr;

    TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();
    FString ImportedBaseFilename = FPaths::GetBaseFilename(glTFImportOptions->FilePathInOS);

    FText TaskName = FText::Format(LOCTEXT("BeginImportMeshTask", "Importing the glTF ({0}) as a static mesh ({1})"), FText::FromName(InputName), FText::FromName(InputName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, TaskName, true);

    /// Create new static mesh
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(InputParent, InputClass, InputName, InputFlags);
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

    TArray<int32> MaterialIds;
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
            if (!GenerateRawMesh(InGlTF, FMatrix::Identity, InGlTF->nodes[NodeId], InBufferFiles, NewRawMesh, MaterialIds, FeedbackTaskWrapper))
            {
                checkSlow(0);
            }
        }
    }

    if (NewRawMesh.IsValidOrFixable())
    {
        /// Swap two axises between Y and Z
        glTFForUE4Ed::SwapYZ(NewRawMesh.VertexPositions);
        glTFForUE4Ed::SwapYZ(NewRawMesh.WedgeTangentX);
        glTFForUE4Ed::SwapYZ(NewRawMesh.WedgeTangentY);
        glTFForUE4Ed::SwapYZ(NewRawMesh.WedgeTangentZ);

        for (FVector& Position : NewRawMesh.VertexPositions)
        {
            Position = Position * glTFImportOptions->MeshScaleRatio;
        }

        SourceModel.BuildSettings.bRecomputeNormals = (glTFImportOptions->bRecomputeNormals || NewRawMesh.WedgeTangentZ.Num() != NewRawMesh.VertexPositions.Num());
        SourceModel.BuildSettings.bRecomputeTangents = (glTFImportOptions->bRecomputeTangents || NewRawMesh.WedgeTangentX.Num() != NewRawMesh.VertexPositions.Num() || NewRawMesh.WedgeTangentY.Num() != NewRawMesh.VertexPositions.Num());
        SourceModel.RawMeshBulkData->SaveRawMesh(NewRawMesh);

        /// Build the static mesh
        TArray<FText> BuildErrors;
        StaticMesh->Build(false, &BuildErrors);
        if (BuildErrors.Num() > 0)
        {
            FeedbackTaskWrapper.Log(ELogVerbosity::Warning, FText::FromString(TEXT("Failed to build the static mesh!")));
            for (const FText& BuildError : BuildErrors)
            {
                FeedbackTaskWrapper.Log(ELogVerbosity::Warning, BuildError);
            }
        }

        //TODO: generate material
        for (int32 MaterialId : MaterialIds)
        {
#if (ENGINE_MINOR_VERSION < 14)
            StaticMesh->Materials.Add(UMaterial::GetDefaultMaterial(MD_Surface));
#else
            StaticMesh->StaticMaterials.Add(UMaterial::GetDefaultMaterial(MD_Surface));
#endif
        }

        // this is damage control. After build, we'd like to absolutely sure that 
        // all index is pointing correctly and they're all used. Otherwise we remove them
        FMeshSectionInfoMap OldSectionInfoMap = StaticMesh->SectionInfoMap;
        StaticMesh->SectionInfoMap.Clear();
        // fix up section data
        for (int32 LODResoureceIndex = 0; LODResoureceIndex < StaticMesh->RenderData->LODResources.Num(); ++LODResoureceIndex)
        {
            FStaticMeshLODResources& LOD = StaticMesh->RenderData->LODResources[LODResoureceIndex];
            int32 NumSections = LOD.Sections.Num();
            for (int32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
            {
                FMeshSectionInfo Info = OldSectionInfoMap.Get(LODResoureceIndex, SectionIndex);
#if (ENGINE_MINOR_VERSION < 14)
                if (StaticMesh->Materials.IsValidIndex(Info.MaterialIndex))
#else
                if (StaticMesh->StaticMaterials.IsValidIndex(Info.MaterialIndex))
#endif
                {
                    StaticMesh->SectionInfoMap.Set(LODResoureceIndex, SectionIndex, Info);
                }
            }
        }

        StaticMesh->MarkPackageDirty();

        if (StaticMesh->AssetImportData)
        {
            StaticMesh->AssetImportData->Update(glTFImportOptions->FilePathInOS);
        }
    }
    else
    {
        StaticMesh->ConditionalBeginDestroy();
        StaticMesh = nullptr;
    }
    return StaticMesh;
}

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FMatrix& InMatrix, const std::shared_ptr<libgltf::SNode>& InNode, const FglTFBufferFiles& InBufferFiles, FRawMesh& OutRawMesh, TArray<int32>& InOutMaterialIds, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
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
        Matrix *= FTranslationMatrix::Make(Translation);
    }
    if (InNode->scale.size() == 3)
    {
        FVector Scale;
        Scale.X = InNode->scale[0];
        Scale.Y = InNode->scale[1];
        Scale.Z = InNode->scale[2];
        Matrix *= FScaleMatrix::Make(Scale);
    }
    if (InNode->rotation.size() == 4)
    {
        FQuat Rotation;
        Rotation.X = InNode->rotation[0];
        Rotation.Y = InNode->rotation[1];
        Rotation.Z = InNode->rotation[2];
        Rotation.W = InNode->rotation[3];
        Matrix *= FQuatRotationMatrix::Make(Rotation);
    }

    if (!!(InNode->mesh))
    {
        const int32_t MeshId = *(InNode->mesh);
        if (MeshId < 0 || MeshId >= InGlTF->meshes.size()) return false;
        const auto& Mesh = InGlTF->meshes[MeshId];
        if (!GenerateRawMesh(InGlTF, Matrix, Mesh, InBufferFiles, OutRawMesh, InOutMaterialIds, InFeedbackTaskWrapper))
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
        if (!GenerateRawMesh(InGlTF, Matrix, NodePtr, InBufferFiles, OutRawMesh, InOutMaterialIds, InFeedbackTaskWrapper))
        {
            checkSlow(0);
            return false;
        }
    }

    return true;
}

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FMatrix& InMatrix, const std::shared_ptr<libgltf::SMesh>& InMesh, const FglTFBufferFiles& InBufferFiles, FRawMesh& OutRawMesh, TArray<int32>& InOutMaterialIds, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    if (!InMesh) return false;

    for (const auto& Primitive : InMesh->primitives)
    {
        FRawMesh NewRawMesh;
        int32 MaterialId = 0;
        {
            if (InOutMaterialIds.Num() > 0)
            {
                MaterialId = InOutMaterialIds.Last() + 1;
            }
        }
        if (!GenerateRawMesh(InGlTF, InMatrix, Primitive, InBufferFiles, NewRawMesh, MaterialId, InFeedbackTaskWrapper))
        {
            checkSlow(0);
            continue;
        }
        if (!glTFForUE4Ed::CheckAndMerge(NewRawMesh, OutRawMesh))
        {
            checkSlow(0);
            continue;
        }
        InOutMaterialIds.Add(MaterialId);
    }
    return true;
}

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FMatrix& InMatrix, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBufferFiles& InBufferFiles, FRawMesh& OutRawMesh, int32 InMaterialId, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    if (!InMeshPrimitive)
    {
        checkSlow(0);
        return false;
    }

    TArray<uint32> TriangleIndices;
    if (!GetTriangleIndices(InGlTF, InBufferFiles, *InMeshPrimitive->indices, TriangleIndices))
    {
        return false;
    }

    TArray<FVector> Points;
    if (InMeshPrimitive->attributes.find(TEXT("POSITION")) != InMeshPrimitive->attributes.cend()
        && !GetVertexPositions(InGlTF, InBufferFiles, *InMeshPrimitive->attributes[TEXT("POSITION")], Points))
    {
        return false;
    }

    TArray<FVector> Normals;
    if (InMeshPrimitive->attributes.find(TEXT("NORMAL")) != InMeshPrimitive->attributes.cend()
        && !GetVertexNormals(InGlTF, InBufferFiles, *InMeshPrimitive->attributes[TEXT("NORMAL")], Normals))
    {
        return false;
    }

    TArray<FVector4> Tangents;
    if (InMeshPrimitive->attributes.find(TEXT("TANGENT")) != InMeshPrimitive->attributes.cend()
        && !GetVertexTangents(InGlTF, InBufferFiles, *InMeshPrimitive->attributes[TEXT("TANGENT")], Tangents))
    {
        return false;
    }

    TArray<FVector2D> TextureCoords[MAX_MESH_TEXTURE_COORDS];
    wchar_t texcoord_str[NAME_SIZE];
    for (int32 i = 0; i < MAX_MESH_TEXTURE_COORDS; ++i)
    {
        std::swprintf(texcoord_str, NAME_SIZE, TEXT("TEXCOORD_%d"), i);
        if (InMeshPrimitive->attributes.find(texcoord_str) != InMeshPrimitive->attributes.cend()
            && !GetVertexTexcoords(InGlTF, InBufferFiles, *InMeshPrimitive->attributes[texcoord_str], TextureCoords[i]))
        {
            break;
        }
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

    for (FVector& Normal : Normals)
    {
        Normal = Transform.TransformVectorNoScale(Normal);
    }
    OutRawMesh.WedgeTangentZ.Append(Normals);

    if (Tangents.Num() == Normals.Num())
    {
        for (int32 i = 0; i < Tangents.Num(); ++i)
        {
            const FVector4& Tangent = Tangents[i];

            FVector WedgeTangentX(Tangent.X, Tangent.Y, Tangent.Z);
            WedgeTangentX = Transform.TransformVectorNoScale(WedgeTangentX);
            OutRawMesh.WedgeTangentX.Add(WedgeTangentX);

            const FVector& Normal = Normals[i];
            OutRawMesh.WedgeTangentY.Add(FVector::CrossProduct(Normal, WedgeTangentX * Tangent.W));
        }
    }
    else if (Tangents.Num() > 0)
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Why is the number of tangent not equal with the number of normal?"));
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
            OutRawMesh.FaceMaterialIndices.Init(InMaterialId, TriangleCount);
        }
        if (OutRawMesh.FaceSmoothingMasks.Num() <= 0)
        {
            OutRawMesh.FaceSmoothingMasks.Init(1, TriangleCount);
        }
        for (int32 i = 0; i < MAX_MESH_TEXTURE_COORDS; ++i)
        {
            if (OutRawMesh.WedgeTexCoords[i].Num() <= 0) continue;
            if (OutRawMesh.WedgeTexCoords[i].Num() > 0 && OutRawMesh.WedgeTexCoords[i].Num() == OutRawMesh.VertexPositions.Num())
            {
                TArray<FVector2D> WedgeTexCoords = OutRawMesh.WedgeTexCoords[i];
                OutRawMesh.WedgeTexCoords[i].Empty();
                OutRawMesh.WedgeTexCoords[i].SetNumUninitialized(WedgeIndicesCount);
                for (int32 j = 0; j < OutRawMesh.WedgeIndices.Num(); ++j)
                {
                    //HACK:
                    OutRawMesh.WedgeTexCoords[i][j] = WedgeTexCoords[OutRawMesh.WedgeIndices[j] % WedgeTexCoords.Num()];
                }
            }
            else
            {
                OutRawMesh.WedgeTexCoords[0].Init(FVector2D::ZeroVector, WedgeIndicesCount);
            }
        }
    }
    return OutRawMesh.IsValidOrFixable();
}

#undef LOCTEXT_NAMESPACE
