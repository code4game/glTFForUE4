// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEd.h"

#include "glTF/glTFImportOptions.h"

#include "libgltf/libgltf.h"

#include "RenderingThread.h"
#include "RawMesh.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#if (ENGINE_MINOR_VERSION < 18)
#include "ImageWrapper.h"
#else
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#endif

#define LOCTEXT_NAMESPACE "FglTFForUE4EdModule"

#define GLTF_MATERIAL_ORIGIN TEXT("/glTFForUE4/Materials/M_PBRMetallicRoughnessOrigin.M_PBRMetallicRoughnessOrigin")

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

    TextureFilter MagFilterToTextureFilter(int32 InValue)
    {
        switch (InValue)
        {
        case 9728:
            return TF_Nearest;

        default:
            break;
        }
        return TF_Default;
    }

    TextureFilter MinFilterToTextureFilter(int32 InValue)
    {
        switch (InValue)
        {
        case 9728:
            return TF_Nearest;

        case 9729:
            return TF_Default;

        case 9984:
        case 9985:
            return TF_Bilinear;

        case 9986:
        case 9987:
            return TF_Trilinear;

        default:
            break;
        }
        return TF_Default;
    }

    TextureAddress WrapSToTextureAddress(int32 InValue)
    {
        switch (InValue)
        {
        case 33071:
            return TA_Clamp;

        case 33648:
            return TA_Mirror;

        case 10497:
            return TA_Wrap;

        default:
            break;
        }
        return TA_Wrap;
    }

    TextureAddress WrapTToTextureAddress(int32 InValue)
    {
        return WrapSToTextureAddress(InValue);
    }

    FString SanitizeObjectName(const FString& InObjectName)
    {
        FString SanitizedName;
        FString InvalidChars = INVALID_OBJECTNAME_CHARACTERS;

        // See if the name contains invalid characters.
        FString Char;
        for (int32 CharIdx = 0; CharIdx < InObjectName.Len(); ++CharIdx)
        {
            Char = InObjectName.Mid(CharIdx, 1);

            if (InvalidChars.Contains(*Char))
            {
                SanitizedName += TEXT("_");
            }
            else
            {
                SanitizedName += Char;
            }
        }

        return SanitizedName;
    }

    template<typename TMaterialExpression>
    TMaterialExpression* FindExpressionParameterByGUID(UMaterial* InMaterial, const FGuid& InGuid)
    {
        if (!InMaterial || !InGuid.IsValid()) return nullptr;

        for (UMaterialExpression* Expression : InMaterial->Expressions)
        {
            if (!Expression || !UMaterial::IsParameter(Expression)) continue;
            TMaterialExpression* ExpressionTemp  = Cast<TMaterialExpression>(Expression);
            if (!ExpressionTemp || ExpressionTemp->ExpressionGUID != InGuid) continue;
            return ExpressionTemp;
        }
        return nullptr;
    }
}

FglTFImporterEd::FglTFMaterialInfo::FglTFMaterialInfo(int32 InId, FString InPrimitiveName)
    : Id(InId)
    , PrimitiveName(InPrimitiveName)
{
    //
}

TSharedPtr<FglTFImporterEd> FglTFImporterEd::Get(UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEd> glTFImporterEd = MakeShareable(new FglTFImporterEd);
    glTFImporterEd->Set(InClass, InParent, InName, InFlags, InFeedbackContext);
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

    const TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();

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
            if (!GenerateRawMesh(InGlTF, FMatrix::Identity, InGlTF->nodes[NodeId], InBufferFiles, NewRawMesh, glTFMaterialInfos, FeedbackTaskWrapper))
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
            FeedbackTaskWrapper.Log(ELogVerbosity::Warning, LOCTEXT("StaticMeshBuildHasError", "Failed to build the static mesh!"));
            for (const FText& BuildError : BuildErrors)
            {
                FeedbackTaskWrapper.Log(ELogVerbosity::Warning, BuildError);
            }
        }

        FMeshSectionInfoMap NewMap;
        UMaterial* glTFMaterialOrigin = LoadObject<UMaterial>(nullptr, GLTF_MATERIAL_ORIGIN);
        if (!glTFMaterialOrigin)
        {
            FeedbackTaskWrapper.Log(ELogVerbosity::Error, LOCTEXT("glTFMaterialOriginHasError", "The glTF material origin must be a `UMaterial`!"));
        }
        static UMaterial* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
        TMap<FString, UTexture*> TextureLibrary;
        for (int32 i = 0; i < glTFMaterialInfos.Num(); ++i)
        {
            const FglTFMaterialInfo& glTFMaterialInfo = glTFMaterialInfos[i];
            UMaterialInterface* NewMaterial = nullptr;
            if (glTFImportOptions->bImportMaterial)
            {
                NewMaterial = CreateMaterial(InglTFImportOptions, InGlTF, glTFMaterialInfo, glTFMaterialOrigin, TextureLibrary);
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

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FMatrix& InMatrix, const std::shared_ptr<libgltf::SNode>& InNode, const FglTFBufferFiles& InBufferFiles, FRawMesh& OutRawMesh, TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
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

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FMatrix& InMatrix, const std::shared_ptr<libgltf::SMesh>& InMesh, const FglTFBufferFiles& InBufferFiles, FRawMesh& OutRawMesh, TArray<FglTFMaterialInfo>& InOutglTFMaterialInfos, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
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
        PrimitiveName = glTFForUE4Ed::SanitizeObjectName(PrimitiveName);
        InOutglTFMaterialInfos.Add(FglTFMaterialInfo(MaterialId, PrimitiveName));
    }
    return true;
}

bool FglTFImporterEd::GenerateRawMesh(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FMatrix& InMatrix, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBufferFiles& InBufferFiles, FRawMesh& OutRawMesh, int32 InMaterialIndex, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
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
            OutRawMesh.FaceMaterialIndices.Init(InMaterialIndex, TriangleCount);
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

UMaterial* FglTFImporterEd::CreateMaterial(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF, const FglTFMaterialInfo& InglTFMaterialInfo, UMaterial* InOrigin, TMap<FString, UTexture*>& InOutTextureLibrary) const
{
    if (!InputParent) return nullptr;
    if (!InglTF || InglTFMaterialInfo.Id < 0 || InglTFMaterialInfo.Id >= InglTF->materials.size() || !InOrigin) return nullptr;

    const TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();

    const std::shared_ptr<libgltf::SMaterial>& glTFMaterial = InglTF->materials[InglTFMaterialInfo.Id];
    if (!glTFMaterial) return nullptr;

    FString MaterialName;
    if (glTFMaterial->name.size() > 0)
    {
        MaterialName = FString::Printf(TEXT("M_%s"), glTFMaterial->name.c_str());
    }
    else
    {
        MaterialName = FString::Printf(TEXT("M_%s"), *InglTFMaterialInfo.PrimitiveName);
    }
    MaterialName = glTFForUE4Ed::SanitizeObjectName(MaterialName);
    FString PackageName = FPackageName::GetLongPackagePath(InputParent->GetPathName()) / MaterialName;
    UPackage* MaterialPackage = FindPackage(nullptr, *PackageName);
    if (!MaterialPackage)
    {
        MaterialPackage = CreatePackage(nullptr, *PackageName);
    }
    if (!MaterialPackage) return nullptr;
    MaterialPackage->FullyLoad();

    UMaterial* NewMaterial = Cast<UMaterial>(StaticDuplicateObject(InOrigin, MaterialPackage, *MaterialName, InputFlags, InOrigin->GetClass()));
    checkSlow(NewMaterial);
    if (!NewMaterial) return nullptr;

    TMap<FName, FGuid> ScalarParameterNameToGuid;
    {
        TArray<FName> ParameterNames;
        TArray<FGuid> ParameterGuids;
#if (ENGINE_MINOR_VERSION < 19)
        NewMaterial->GetAllScalarParameterNames(ParameterNames, ParameterGuids);
#else
        TArray<FMaterialParameterInfo> ParameterInfos;
        NewMaterial->GetAllScalarParameterInfo(ParameterInfos, ParameterGuids);
        for (const FMaterialParameterInfo& ParameterInfo : ParameterInfos)
        {
            ParameterNames.Add(ParameterInfo.Name);
        }
#endif
        if (ParameterNames.Num() == ParameterGuids.Num())
        {
            for (int32 i = 0; i < ParameterNames.Num(); ++i)
            {
                ScalarParameterNameToGuid.FindOrAdd(ParameterNames[i]) = ParameterGuids[i];
            }
        }
    }
    TMap<FName, FGuid> VectorParameterNameToGuid;
    {
        TArray<FName> ParameterNames;
        TArray<FGuid> ParameterGuids;
#if (ENGINE_MINOR_VERSION < 19)
        NewMaterial->GetAllVectorParameterNames(ParameterNames, ParameterGuids);
#else
        TArray<FMaterialParameterInfo> ParameterInfos;
        NewMaterial->GetAllVectorParameterInfo(ParameterInfos, ParameterGuids);
        for (const FMaterialParameterInfo& ParameterInfo : ParameterInfos)
        {
            ParameterNames.Add(ParameterInfo.Name);
        }
#endif
        if (ParameterNames.Num() == ParameterGuids.Num())
        {
            for (int32 i = 0; i < ParameterNames.Num(); ++i)
            {
                VectorParameterNameToGuid.FindOrAdd(ParameterNames[i]) = ParameterGuids[i];
            }
        }
    }
    TMap<FName, FGuid> TextureParameterNameToGuid;
    {
        TArray<FName> ParameterNames;
        TArray<FGuid> ParameterGuids;
#if (ENGINE_MINOR_VERSION < 19)
        NewMaterial->GetAllTextureParameterNames(ParameterNames, ParameterGuids);
#else
        TArray<FMaterialParameterInfo> ParameterInfos;
        NewMaterial->GetAllTextureParameterInfo(ParameterInfos, ParameterGuids);
        for (const FMaterialParameterInfo& ParameterInfo : ParameterInfos)
        {
            ParameterNames.Add(ParameterInfo.Name);
        }
#endif
        if (ParameterNames.Num() == ParameterGuids.Num())
        {
            for (int32 i = 0; i < ParameterNames.Num(); ++i)
            {
                TextureParameterNameToGuid.FindOrAdd(ParameterNames[i]) = ParameterGuids[i];
            }
        }
    }

    if (ScalarParameterNameToGuid.Contains(TEXT("alphaCutoff")))
    {
        if (UMaterialExpressionScalarParameter* ScalarParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionScalarParameter>(NewMaterial, ScalarParameterNameToGuid[TEXT("alphaCutoff")]))
        {
            ScalarParameter->DefaultValue = glTFMaterial->alphaCutoff;
        }
    }
    if (!!(glTFMaterial->emissiveTexture)
        && !!(glTFMaterial->emissiveTexture->index)
        && TextureParameterNameToGuid.Contains(TEXT("emissiveTexture")))
    {
        if (UMaterialExpressionTextureSampleParameter* SampleParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionTextureSampleParameter>(NewMaterial, TextureParameterNameToGuid[TEXT("emissiveTexture")]))
        {
            if (!ConstructSampleParameter(InglTFImportOptions, InglTF, glTFMaterial->emissiveTexture, TEXT("emissiveTexture"), InOutTextureLibrary, SampleParameter))
            {
                UE_LOG(LogglTFForUE4Ed, Warning, TEXT("Failed to construct the `emissiveTexture`"));
            }
        }
    }
    if (!!(glTFMaterial->pbrMetallicRoughness))
    {
        const std::shared_ptr<libgltf::SMaterialPBRMetallicRoughness>& pbrMetallicRoughness = glTFMaterial->pbrMetallicRoughness;
        if (ScalarParameterNameToGuid.Contains(TEXT("roughnessFactor")))
        {
            if (UMaterialExpressionScalarParameter* ScalarParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionScalarParameter>(NewMaterial, ScalarParameterNameToGuid[TEXT("roughnessFactor")]))
            {
                ScalarParameter->DefaultValue = pbrMetallicRoughness->roughnessFactor;
            }
        }
        if (!!(pbrMetallicRoughness->baseColorTexture) && TextureParameterNameToGuid.Contains(TEXT("baseColorTexture")))
        {
            if (UMaterialExpressionTextureSampleParameter* SampleParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionTextureSampleParameter>(NewMaterial, TextureParameterNameToGuid[TEXT("baseColorTexture")]))
            {
                if (!ConstructSampleParameter(InglTFImportOptions, InglTF, pbrMetallicRoughness->baseColorTexture, TEXT("baseColorTexture"), InOutTextureLibrary, SampleParameter))
                {
                    UE_LOG(LogglTFForUE4Ed, Warning, TEXT("Failed to construct the `baseColorTexture`"));
                }
            }
        }
        if (ScalarParameterNameToGuid.Contains(TEXT("metallicFactor")))
        {
            if (UMaterialExpressionScalarParameter* ScalarParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionScalarParameter>(NewMaterial, ScalarParameterNameToGuid[TEXT("metallicFactor")]))
            {
                ScalarParameter->DefaultValue = pbrMetallicRoughness->metallicFactor;
            }
        }
        if (pbrMetallicRoughness->baseColorFactor.size() > 0 && VectorParameterNameToGuid.Contains(TEXT("baseColorFactor")))
        {
            if (UMaterialExpressionVectorParameter* VectorParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionVectorParameter>(NewMaterial, VectorParameterNameToGuid[TEXT("baseColorFactor")]))
            {
                if (pbrMetallicRoughness->baseColorFactor.size() > 0) VectorParameter->DefaultValue.R = pbrMetallicRoughness->baseColorFactor[0];
                if (pbrMetallicRoughness->baseColorFactor.size() > 1) VectorParameter->DefaultValue.G = pbrMetallicRoughness->baseColorFactor[1];
                if (pbrMetallicRoughness->baseColorFactor.size() > 2) VectorParameter->DefaultValue.B = pbrMetallicRoughness->baseColorFactor[2];
                if (pbrMetallicRoughness->baseColorFactor.size() > 3) VectorParameter->DefaultValue.A = pbrMetallicRoughness->baseColorFactor[3];
            }
        }
        if (!!(pbrMetallicRoughness->metallicRoughnessTexture) && TextureParameterNameToGuid.Contains(TEXT("metallicRoughnessTexture")))
        {
            if (UMaterialExpressionTextureSampleParameter* SampleParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionTextureSampleParameter>(NewMaterial, TextureParameterNameToGuid[TEXT("metallicRoughnessTexture")]))
            {
                if (!ConstructSampleParameter(InglTFImportOptions, InglTF, pbrMetallicRoughness->metallicRoughnessTexture, TEXT("metallicRoughnessTexture"), InOutTextureLibrary, SampleParameter))
                {
                    UE_LOG(LogglTFForUE4Ed, Warning, TEXT("Failed to construct the `metallicRoughnessTexture`"));
                }
            }
        }
    }
    if (!!(glTFMaterial->occlusionTexture))
    {
        const std::shared_ptr<libgltf::SMaterialOcclusionTextureInfo>& occlusionTexture = glTFMaterial->occlusionTexture;
        if (TextureParameterNameToGuid.Contains(TEXT("occlusionTexture")))
        {
            if (UMaterialExpressionTextureSampleParameter* SampleParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionTextureSampleParameter>(NewMaterial, TextureParameterNameToGuid[TEXT("occlusionTexture")]))
            {
                if (!ConstructSampleParameter(InglTFImportOptions, InglTF, glTFMaterial->occlusionTexture, TEXT("occlusionTexture"), InOutTextureLibrary, SampleParameter))
                {
                    UE_LOG(LogglTFForUE4Ed, Warning, TEXT("Failed to construct the `occlusionTexture`"));
                }
            }
        }
        if (ScalarParameterNameToGuid.Contains(TEXT("strength")))
        {
            if (UMaterialExpressionScalarParameter* ScalarParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionScalarParameter>(NewMaterial, ScalarParameterNameToGuid[TEXT("strength")]))
            {
                ScalarParameter->DefaultValue = occlusionTexture->strength;
            }
        }
    }

    {
        /// Setup the blend mode
        FString AlphaMode(glTFMaterial->alphaMode.c_str());
        if (AlphaMode.Equals(TEXT("OPAQUE"), ESearchCase::IgnoreCase))
        {
            NewMaterial->BlendMode = BLEND_Opaque;
        }
        else if (AlphaMode.Equals(TEXT("MASK"), ESearchCase::IgnoreCase))
        {
            NewMaterial->BlendMode = BLEND_Masked;
        }
        else if (AlphaMode.Equals(TEXT("BLEND"), ESearchCase::IgnoreCase))
        {
            NewMaterial->BlendMode = BLEND_Translucent;
        }
    }

    NewMaterial->TwoSided = glTFMaterial->doubleSided;

    if (!!glTFMaterial->normalTexture && TextureParameterNameToGuid.Contains(TEXT("normalTexture")))
    {
        if (UMaterialExpressionTextureSampleParameter* SampleParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionTextureSampleParameter>(NewMaterial, TextureParameterNameToGuid[TEXT("normalTexture")]))
        {
            if (!ConstructSampleParameter(InglTFImportOptions, InglTF, glTFMaterial->normalTexture, TEXT("normalTexture"), InOutTextureLibrary, SampleParameter, true))
            {
                UE_LOG(LogglTFForUE4Ed, Warning, TEXT("Failed to construct the `normalTexture`"));
            }
        }
    }

    if (glTFMaterial->emissiveFactor.size() > 0 && VectorParameterNameToGuid.Contains(TEXT("emissiveFactor")))
    {
        if (UMaterialExpressionVectorParameter* VectorParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionVectorParameter>(NewMaterial, VectorParameterNameToGuid[TEXT("emissiveFactor")]))
        {
            if (glTFMaterial->emissiveFactor.size() > 0) VectorParameter->DefaultValue.R = glTFMaterial->emissiveFactor[0];
            if (glTFMaterial->emissiveFactor.size() > 1) VectorParameter->DefaultValue.G = glTFMaterial->emissiveFactor[1];
            if (glTFMaterial->emissiveFactor.size() > 2) VectorParameter->DefaultValue.B = glTFMaterial->emissiveFactor[2];
            if (glTFMaterial->emissiveFactor.size() > 3) VectorParameter->DefaultValue.A = glTFMaterial->emissiveFactor[3];
        }
    }

    NewMaterial->PostEditChange();
    NewMaterial->MarkPackageDirty();
    return NewMaterial;
}

bool FglTFImporterEd::ConstructSampleParameter(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF, const std::shared_ptr<libgltf::STextureInfo>& InglTFTextureInfo, const FString& InParameterName, TMap<FString, UTexture*>& InOutTextureLibrary, class UMaterialExpressionTextureSampleParameter* InSampleParameter, bool InIsNormalmap /*= false*/) const
{
    if (!InglTF || !InglTFTextureInfo || !InSampleParameter) return false;
    if (!(InglTFTextureInfo->index)) return false;
    int32 glTFTextureId = *(InglTFTextureInfo->index);
    if (glTFTextureId < 0 || glTFTextureId >= InglTF->textures.size()) return false;
    const std::shared_ptr<libgltf::STexture>& glTFTexture = InglTF->textures[glTFTextureId];
    if (!glTFTexture) return false;

    FString TextureName = FString::Printf(TEXT("T_%s_%d_%s"), *InputName.ToString(), glTFTextureId, *InParameterName);
    TextureName = glTFForUE4Ed::SanitizeObjectName(TextureName);
    UTexture* Texture = nullptr;
    if (InOutTextureLibrary.Contains(TextureName))
    {
        Texture = InOutTextureLibrary[TextureName];
    }
    else
    {
        //
        Texture = CreateTexture(InglTFImportOptions, InglTF, glTFTexture, TextureName, InIsNormalmap);
        if (Texture)
        {
            InOutTextureLibrary.Add(TextureName, Texture);
        }
    }
    if (Texture)
    {
        InSampleParameter->Texture = Texture;
    }
    //TODO: test the `texCoord`
    InSampleParameter->ConstCoordinate = static_cast<uint32>(InglTFTextureInfo->texCoord);
    return (Texture != nullptr);
}

UTexture* FglTFImporterEd::CreateTexture(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF, const std::shared_ptr<libgltf::STexture>& InglTFTexture, const FString& InTextureName, bool InIsNormalmap /*= false*/) const
{
    if (!InglTF || !InglTFTexture || !(InglTFTexture->source)) return nullptr;
    int32 glTFImageId = (int32)(*(InglTFTexture->source));
    if (glTFImageId < 0 || glTFImageId >= InglTF->images.size()) return nullptr;
    const std::shared_ptr<libgltf::SImage>& glTFImage = InglTF->images[glTFImageId];
    if (!glTFImage) return nullptr;

    const TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();
    FString ImageFilePath = FPaths::GetPath(glTFImportOptions->FilePathInOS) / glTFImage->uri.c_str();

    IFileManager& FileManager = IFileManager::Get();
    if (!FileManager.FileExists(*ImageFilePath)) return nullptr;

    TArray<uint8> ImageFileData;
    if (!FFileHelper::LoadFileToArray(ImageFileData, *ImageFilePath)) return nullptr;

    FString PackageName = FPackageName::GetLongPackagePath(InputParent->GetPathName()) / InTextureName;
    UPackage* TexturePackage = FindPackage(nullptr, *PackageName);
    if (!TexturePackage)
    {
        TexturePackage = CreatePackage(nullptr, *PackageName);
    }
    if (!TexturePackage) return nullptr;
    TexturePackage->FullyLoad();

    UTexture2D* NewTexture = nullptr;

    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
#if (ENGINE_MINOR_VERSION < 16)
    IImageWrapperPtr ImageWrappers[7] = {
        ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::GrayscaleJPEG),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::ICO),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::EXR),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::ICNS),
    };
#else
    TSharedPtr<IImageWrapper> ImageWrappers[7] = {
        ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::GrayscaleJPEG),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::ICO),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::EXR),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::ICNS),
    };
#endif

    for (auto ImageWrapper : ImageWrappers)
    {
        if (!ImageWrapper.IsValid()) continue;
        if (!ImageWrapper->SetCompressed(ImageFileData.GetData(), ImageFileData.Num())) continue;

        ETextureSourceFormat TextureFormat = TSF_Invalid;

        int32 Width = ImageWrapper->GetWidth();
        int32 Height = ImageWrapper->GetHeight();

        // TODO: Do create a texture with bad dimensions.
        if ((Width & (Width - 1)) || (Height & (Height - 1)))
        {
            break;
        }

        int32 BitDepth = ImageWrapper->GetBitDepth();
#if (ENGINE_MINOR_VERSION < 18)
        ERGBFormat::Type ImageFormat = ImageWrapper->GetFormat();
#else
        ERGBFormat ImageFormat = ImageWrapper->GetFormat();
#endif

        if (ImageFormat == ERGBFormat::Gray)
        {
            if (BitDepth <= 8)
            {
                TextureFormat = TSF_G8;
                ImageFormat = ERGBFormat::Gray;
                BitDepth = 8;
            }
            else if (BitDepth == 16)
            {
                // TODO: TSF_G16?
                TextureFormat = TSF_RGBA16;
                ImageFormat = ERGBFormat::RGBA;
                BitDepth = 16;
            }
        }
        else if (ImageFormat == ERGBFormat::RGBA || ImageFormat == ERGBFormat::BGRA)
        {
            if (BitDepth <= 8)
            {
                TextureFormat = TSF_BGRA8;
                ImageFormat = ERGBFormat::BGRA;
                BitDepth = 8;
            }
            else if (BitDepth == 16)
            {
                TextureFormat = TSF_RGBA16;
                ImageFormat = ERGBFormat::RGBA;
                BitDepth = 16;
            }
        }

        if (TextureFormat == TSF_Invalid)
        {
            UE_LOG(LogglTFForUE4Ed, Error, TEXT("It is an unsupported image format."));
            break;
        }

        NewTexture = NewObject<UTexture2D>(TexturePackage, UTexture2D::StaticClass(), *InTextureName, InputFlags);
        checkSlow(NewTexture);
        if (!NewTexture) break;
        NewTexture->Source.Init2DWithMipChain(Width, Height, TextureFormat);
        NewTexture->SRGB = !InIsNormalmap;
        NewTexture->CompressionSettings = !InIsNormalmap ? TC_Default : TC_Normalmap;
        const TArray<uint8>* RawData = nullptr;
        if (ImageWrapper->GetRaw(ImageFormat, BitDepth, RawData))
        {
            uint8* MipData = NewTexture->Source.LockMip(0);
            FMemory::Memcpy(MipData, RawData->GetData(), RawData->Num());
            NewTexture->Source.UnlockMip(0);
        }
        break;
    }

    if (NewTexture && !!(InglTFTexture->sampler))
    {
        int32 glTFSamplerId = *(InglTFTexture->sampler);
        if (glTFSamplerId >= 0 && glTFSamplerId < InglTF->samplers.size())
        {
            const std::shared_ptr<libgltf::SSampler>& glTFSampler = InglTF->samplers[glTFSamplerId];
            if (glTFSampler)
            {
                NewTexture->Filter = glTFForUE4Ed::MinFilterToTextureFilter(glTFSampler->minFilter);
                NewTexture->AddressX = glTFForUE4Ed::WrapSToTextureAddress(glTFSampler->wrapS);
                NewTexture->AddressY = glTFForUE4Ed::WrapSToTextureAddress(glTFSampler->wrapT);
            }
        }
    }
    if (NewTexture)
    {
        NewTexture->UpdateResource();
        NewTexture->AssetImportData->Update(*ImageFilePath);

        NewTexture->PostEditChange();
        NewTexture->MarkPackageDirty();
    }
    return NewTexture;
}

#undef LOCTEXT_NAMESPACE
