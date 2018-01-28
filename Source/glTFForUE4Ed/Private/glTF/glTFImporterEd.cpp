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

    UObject* StaticMesh = nullptr;
    if (InGlTF->scene)
    {
        const std::shared_ptr<libgltf::SScene>& Scene = InGlTF->scenes[(int32)(*InGlTF->scene)];
        if (Scene)
        {
            if (0)
            {
                StaticMesh = CreateStaticMesh(InglTFImportOptions, InGlTF, Scene, BufferFiles); //TEST
            }
            else
            {
                TArray<UStaticMesh*> StaticMeshes;
                if (CreateNode(InglTFImportOptions, Scene->nodes, InGlTF, BufferFiles, FText::FromString(FolderPathInOS), StaticMeshes)
                    && StaticMeshes.Num() > 0)
                {
                    StaticMesh = StaticMeshes[0];
                }
            }
        }
        else
        {
            UE_LOG(LogglTFForUE4Ed, Error, TEXT("Invalid scene!"));
        }
    }
    else if (InGlTF->scenes.size() > 0)
    {
        for (const std::shared_ptr<libgltf::SScene>& Scene : InGlTF->scenes)
        {
            TArray<UStaticMesh*> StaticMeshes;
            if (CreateNode(InglTFImportOptions, Scene->nodes, InGlTF, BufferFiles, FText::FromString(FolderPathInOS), StaticMeshes)
                && StaticMeshes.Num() > 0)
            {
                StaticMesh = StaticMeshes[0];
            }
        }
    }
    else
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("No scene!"));
    }
    return StaticMesh;
}

UStaticMesh* FglTFImporterEd::CreateStaticMesh(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SScene>& InScene, const FglTFBufferFiles& InBufferFiles) const
{
    if (!InGlTF || !InScene || InScene->nodes.empty()) return nullptr;

    TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();

    FString ImportedBaseFilename = FPaths::GetBaseFilename(glTFImportOptions->FilePathInOS);

    if (InputClass != UStaticMesh::StaticClass() || !InputParent || !InputName.IsValid()) return nullptr;

    FString SceneName(InScene->name.c_str());
    FText TaskName = FText::Format(LOCTEXT("BeginImportMeshTask", "Importing the scene ({0}) as a static mesh ({1})"), FText::FromString(SceneName), FText::FromName(InputName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, TaskName, true);

    /// Create new static mesh
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(InputParent, InputClass, InputName, InputFlags);
    if (!StaticMesh) return nullptr;

    StaticMesh->SourceModels.Empty();
    new(StaticMesh->SourceModels) FStaticMeshSourceModel();

    FStaticMeshSourceModel& SourceModel = StaticMesh->SourceModels[0];
    SourceModel.BuildSettings.bUseMikkTSpace = glTFImportOptions->bUseMikkTSpace;
    SourceModel.BuildSettings.bRecomputeNormals = glTFImportOptions->bRecomputeNormals;
    SourceModel.BuildSettings.bRecomputeTangents = glTFImportOptions->bRecomputeTangents;

    StaticMesh->LightingGuid = FGuid::NewGuid();
    StaticMesh->LightMapResolution = 64;
    StaticMesh->LightMapCoordinateIndex = 1;

    FRawMesh NewRawMesh;
    SourceModel.RawMeshBulkData->LoadRawMesh(NewRawMesh);
    NewRawMesh.Empty();

    TArray<int32> MaterialIds;
    for (const auto& NodeIdPtr : InScene->nodes)
    {
        if (!NodeIdPtr)
        {
            checkSlow(0);
            continue;
        }
        const int32 NodeId = *NodeIdPtr;
        if ( NodeId < 0 || NodeId >= InGlTF->nodes.size())
        {
            checkSlow(0);
            continue;
        }
        if (!GenerateRawMesh(InGlTF, FTransform::Identity, InGlTF->nodes[NodeId], InBufferFiles, NewRawMesh, MaterialIds, FeedbackTaskWrapper))
        {
            checkSlow(0);
        }
    }
    //TODO: gather mesh data

    if (NewRawMesh.IsValidOrFixable())
    {
        SourceModel.RawMeshBulkData->SaveRawMesh(NewRawMesh);

        /// Build the static mesh
        TArray<FText> BuildErrors;
        StaticMesh->Build(false, &BuildErrors);
        if (BuildErrors.Num() <= 0)
        {
            //TODO: create material
        }
        else
        {
            FeedbackTaskWrapper.Log(ELogVerbosity::Warning, FText::FromString(TEXT("Failed to build the static mesh!")));
            for (const FText& BuildError : BuildErrors)
            {
                FeedbackTaskWrapper.Log(ELogVerbosity::Warning, BuildError);
            }
        }

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

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FTransform& InTransform, const std::shared_ptr<libgltf::SNode>& InNode, const FglTFBufferFiles& InBufferFiles, FRawMesh& OutRawMesh, TArray<int32>& InOutMaterialIds, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    if (!InGlTF || !InNode) return false;

    FTransform Transform;
    FMatrix Matrix = InTransform.ToMatrixWithScale();
    if (InNode->matrix.size() == 16)
    {
        for (uint32 j = 0; j < 4; ++j)
        {
            for (uint32 i = 0; i < 4; ++i)
            {
                uint32 Index = i + j * 4;
                Matrix.M[i][j] = InNode->matrix[Index];
            }
        }
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
    Transform.SetFromMatrix(Matrix);

    if (!!(InNode->mesh))
    {
        const int32_t MeshId = *(InNode->mesh);
        if (MeshId < 0 || MeshId >= InGlTF->meshes.size()) return false;
        const auto& Mesh = InGlTF->meshes[MeshId];
        if (!GenerateRawMesh(InGlTF, Transform, Mesh, InBufferFiles, OutRawMesh, InOutMaterialIds, InFeedbackTaskWrapper))
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
        if (!GenerateRawMesh(InGlTF, Transform, NodePtr, InBufferFiles, OutRawMesh, InOutMaterialIds, InFeedbackTaskWrapper))
        {
            checkSlow(0);
            return false;
        }
    }

    return true;
}

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FTransform& InTransform, const std::shared_ptr<libgltf::SMesh>& InMesh, const FglTFBufferFiles& InBufferFiles, FRawMesh& OutRawMesh, TArray<int32>& InOutMaterialIds, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    if (!InMesh) return false;

    for (const auto& Primitive : InMesh->primitives)
    {
        if (!GenerateRawMesh(InGlTF, InTransform, Primitive, InBufferFiles, OutRawMesh, InFeedbackTaskWrapper))
        {
            checkSlow(0);
            continue;
        }
        //
    }
    //
    return true;
}

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FTransform& InTransform, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBufferFiles& InBufferFiles, FRawMesh& OutRawMesh, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
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
    //
    return false;
}

UStaticMesh* FglTFImporterEd::CreateStaticMesh(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SMesh>& InMesh, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles) const
{
    if (!InMesh) return nullptr;

    TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();

    FString ImportedBaseFilename = FPaths::GetBaseFilename(glTFImportOptions->FilePathInOS);

    /// Create new package
    FString MeshName(InMesh->name.c_str());
    MeshName = MeshName.Replace(TEXT("."), TEXT(""));
    MeshName = MeshName.Replace(TEXT("/"), TEXT(""));
    MeshName = MeshName.Replace(TEXT("\\"), TEXT(""));
    FString PackageName = FPackageName::GetLongPackagePath(glTFImportOptions->FilePathInEngine) / (ImportedBaseFilename + TEXT("_") + (MeshName.IsEmpty() ? TEXT("none") : MeshName));
    UPackage* Package = FindPackage(nullptr, *PackageName);
    if (!Package)
    {
        Package = CreatePackage(nullptr, *PackageName);
        if (!Package)
        {
            UE_LOG(LogglTFForUE4Ed, Error, TEXT("Can't create packate - %s"), *PackageName);
            return nullptr;
        }
    }

    FName StaticMeshName = FPackageName::GetShortFName(*PackageName);
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, StaticMeshName, RF_Public | RF_Standalone);
    StaticMesh->SourceModels.Empty();
    new(StaticMesh->SourceModels) FStaticMeshSourceModel();

    FStaticMeshSourceModel& SourceModel = StaticMesh->SourceModels[0];
    SourceModel.BuildSettings.bUseMikkTSpace = glTFImportOptions->bUseMikkTSpace;
    SourceModel.BuildSettings.bRecomputeNormals = glTFImportOptions->bRecomputeNormals;
    SourceModel.BuildSettings.bRecomputeTangents = glTFImportOptions->bRecomputeTangents;

    StaticMesh->LightingGuid = FGuid::NewGuid();
    StaticMesh->LightMapResolution = 64;
    StaticMesh->LightMapCoordinateIndex = 1;

    FRawMesh NewRawMesh;
    SourceModel.RawMeshBulkData->LoadRawMesh(NewRawMesh);
    NewRawMesh.Empty();

    uint32 TriangleIndexStart = 0;
    for (const std::shared_ptr<libgltf::SMeshPrimitive>& MeshPrimitive : InMesh->primitives)
    {
        TArray<uint32> TriangleIndices;
        if (!GetTriangleIndices(InGlTF, InBufferFiles, *MeshPrimitive->indices, TriangleIndices))
        {
            break;
        }

        TArray<FVector> Points;
        if (MeshPrimitive->attributes.find(TEXT("POSITION")) != MeshPrimitive->attributes.cend()
            && !GetVertexPositions(InGlTF, InBufferFiles, *MeshPrimitive->attributes[TEXT("POSITION")], Points))
        {
            break;
        }

        TArray<FVector> Normals;
        if (!SourceModel.BuildSettings.bRecomputeNormals
            && MeshPrimitive->attributes.find(TEXT("NORMAL")) != MeshPrimitive->attributes.cend()
            && !GetVertexNormals(InGlTF, InBufferFiles, *MeshPrimitive->attributes[TEXT("NORMAL")], Normals))
        {
            break;
        }

        TArray<FVector4> Tangents;
        if (!SourceModel.BuildSettings.bRecomputeTangents
            && MeshPrimitive->attributes.find(TEXT("TANGENT")) != MeshPrimitive->attributes.cend()
            && !GetVertexTangents(InGlTF, InBufferFiles, *MeshPrimitive->attributes[TEXT("TANGENT")], Tangents))
        {
            break;
        }

        TArray<FVector2D> TextureCoords[MAX_MESH_TEXTURE_COORDS];
        wchar_t texcoord_str[NAME_SIZE];
        for (int32 i = 0; i < MAX_MESH_TEXTURE_COORDS; ++i)
        {
            std::swprintf(texcoord_str, NAME_SIZE, TEXT("TEXCOORD_%d"), i);
            if (MeshPrimitive->attributes.find(texcoord_str) != MeshPrimitive->attributes.cend()
                && !GetVertexTexcoords(InGlTF, InBufferFiles, *MeshPrimitive->attributes[texcoord_str], TextureCoords[i]))
            {
                break;
            }
        }

        if (Points.Num() <= 0) break;

        for (uint32& TriangleIndex : TriangleIndices)
        {
            if (static_cast<int32>(TriangleIndex) >= Points.Num())
            {
                TriangleIndex = TriangleIndex % Points.Num();
            }
            TriangleIndex += TriangleIndexStart;
        }
        NewRawMesh.WedgeIndices.Append(TriangleIndices);

        NewRawMesh.VertexPositions.Append(Points);
        TriangleIndexStart = NewRawMesh.VertexPositions.Num();

        if (Normals.Num() <= 0)
        {
            SourceModel.BuildSettings.bRecomputeNormals = true;
        }
        else
        {
            for (const FVector& Normal : Normals)
            {
                NewRawMesh.WedgeTangentZ.Add(Normal * -1.0f);
            }
        }

        if (Tangents.Num() <= 0)
        {
            SourceModel.BuildSettings.bRecomputeTangents = true;
        }
        else if (Tangents.Num() == Normals.Num())
        {
            for (const FVector4& Tangent : Tangents)
            {
                NewRawMesh.WedgeTangentX.Add(FVector(Tangent.X, Tangent.Y, Tangent.Z) * -1.0f);
            }

            for (int32 i = 0; i < Tangents.Num(); ++i)
            {
                const FVector4& Tangent = Tangents[i];
                const FVector& Normal = Normals[i];
                NewRawMesh.WedgeTangentY.Add(FVector::CrossProduct(Normal, FVector(Tangent.X, Tangent.Y, Tangent.Z) * -Tangent.W));
            }
        }
        else
        {
            UE_LOG(LogglTFForUE4Ed, Error, TEXT("Why is the number of tangent not equal with the number of normal?"));
        }

        for (int32 i = 0; i < MAX_MESH_TEXTURE_COORDS; ++i)
        {
            if (TextureCoords[i].Num() <= 0) continue;
            NewRawMesh.WedgeTexCoords[i].Append(TextureCoords[i]);
        }
    }

    int32 WedgeIndicesCount = NewRawMesh.WedgeIndices.Num();
    if (WedgeIndicesCount > 0 && (WedgeIndicesCount % 3) == 0)
    {
        int32 TriangleCount = NewRawMesh.WedgeIndices.Num() / 3;
        if (NewRawMesh.FaceMaterialIndices.Num() <= 0)
        {
            NewRawMesh.FaceMaterialIndices.Init(0, TriangleCount);
        }
        if (NewRawMesh.FaceSmoothingMasks.Num() <= 0)
        {
            NewRawMesh.FaceSmoothingMasks.Init(1, TriangleCount);
        }
        if (NewRawMesh.WedgeTexCoords[0].Num() != WedgeIndicesCount)
        {
            if (NewRawMesh.WedgeTexCoords[0].Num() > 0
                && NewRawMesh.WedgeTexCoords[0].Num() == NewRawMesh.VertexPositions.Num())
            {
                TArray<FVector2D> WedgeTexCoords = NewRawMesh.WedgeTexCoords[0];
                NewRawMesh.WedgeTexCoords[0].Empty();
                NewRawMesh.WedgeTexCoords[0].SetNumUninitialized(WedgeIndicesCount);
                for (int32 i = 0; i < NewRawMesh.WedgeIndices.Num(); ++i)
                {
                    //HACK:
                    NewRawMesh.WedgeTexCoords[0][i] = WedgeTexCoords[NewRawMesh.WedgeIndices[i] % WedgeTexCoords.Num()];
                }
            }
            else
            {
                NewRawMesh.WedgeTexCoords[0].Init(FVector2D::ZeroVector, WedgeIndicesCount);
            }
        }
    }

    if (NewRawMesh.IsValidOrFixable())
    {
        /// Invert normal by option
        if (glTFImportOptions->bInvertNormal)
        {
            for (int32 i = 0; i < WedgeIndicesCount; i += 3)
            {
                NewRawMesh.WedgeIndices.Swap(i + 1, i + 2);
                NewRawMesh.WedgeTexCoords[0].Swap(i + 1, i + 2);
            }
            for (FVector& Normal : NewRawMesh.WedgeTangentZ)
            {
                Normal = Normal * -1.0f;
            }
        }

        /// Scale the point position by option
        /// And invert the position
        for (FVector& Position : NewRawMesh.VertexPositions)
        {
            Position = Position * glTFImportOptions->MeshScaleRatio;
            Swap(Position.Y, Position.Z);
        }

        SourceModel.RawMeshBulkData->SaveRawMesh(NewRawMesh);

        if (glTFImportOptions->bImportMaterial)
        {
            //TODO:
            checkf(0, TEXT("This function is in developing"));
        }
        else
        {
#if (ENGINE_MINOR_VERSION < 14)
            StaticMesh->Materials.Add(UMaterial::GetDefaultMaterial(MD_Surface));
#else
            StaticMesh->StaticMaterials.Add(UMaterial::GetDefaultMaterial(MD_Surface));
#endif
        }

        /// Build the static mesh
        TArray<FText> BuildErrors;
        StaticMesh->Build(false, &BuildErrors);
        if (BuildErrors.Num() <= 0)
        {
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
        }
        else
        {
            UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to build the static mesh!"));
            for (const FText& BuildError : BuildErrors)
            {
                UE_LOG(LogglTFForUE4Ed, Error, TEXT("BuildError: %s"), *BuildError.ToString());
            }
        }

        StaticMesh->MarkPackageDirty();
    }
    else
    {
        if (StaticMesh && StaticMesh->IsValidLowLevel())
        {
            StaticMesh->ConditionalBeginDestroy();
            StaticMesh = nullptr;
        }
        if (Package && Package->IsValidLowLevel())
        {
            Package->ConditionalBeginDestroy();
            Package = nullptr;
        }
    }
    return StaticMesh;
}

bool FglTFImporterEd::CreateNode(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::vector<std::shared_ptr<libgltf::SGlTFId>>& InNodeIndices, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, FText InParentNodeName, TArray<UStaticMesh*>& OutStaticMeshes) const
{
    FText ImportMessage = FText::Format(LOCTEXT("BeginImportingglTFNodeTask", "Importing a node {0}"), InParentNodeName);

    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, ImportMessage, true);

    bool bIsSuccess = true;
    for (size_t i = 0, count = InNodeIndices.size(); i < count; ++i)
    {
        const std::shared_ptr<libgltf::SGlTFId>& NodeIndex = InNodeIndices[i];
        if (!NodeIndex)
        {
            bIsSuccess = false;
            break;
        }
        const std::shared_ptr<libgltf::SNode>& Node = InGlTF->nodes[(int32)(*NodeIndex)];
        if (!Node)
        {
            bIsSuccess = false;
            break;
        }

        FeedbackTaskWrapper.UpdateProgress(static_cast<int32>(i), static_cast<int32>(count));

        const std::shared_ptr<libgltf::SGlTFId>& MeshIndex = Node->mesh;
        if (MeshIndex)
        {
            const std::shared_ptr<libgltf::SMesh>& Mesh = InGlTF->meshes[(int32)(*MeshIndex)];

            if (Mesh)
            {
                FeedbackTaskWrapper.StatusUpdate(static_cast<int32>(i), static_cast<int32>(count), FText::FromString(Mesh->name.c_str()));
            }

            UStaticMesh* NewStaticMesh = CreateStaticMesh(InglTFImportOptions, Mesh, InGlTF, InBufferFiles);
            if (NewStaticMesh)
            {
                OutStaticMeshes.Add(NewStaticMesh);
            }
        }

        CreateNode(InglTFImportOptions, Node->children, InGlTF, InBufferFiles, FText::FromString(Node->name.c_str()), OutStaticMeshes);

        FeedbackTaskWrapper.UpdateProgress(static_cast<int32>(i + 1), static_cast<int32>(count));
    }

    return (bIsSuccess && OutStaticMeshes.Num() > 0);
}

#undef LOCTEXT_NAMESPACE
