#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporter.h"

#include "glTF/glTFImportOptions.h"

#include "libgltf/libgltf.h"

#include "RenderingThread.h"
#include "RawMesh.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Misc/Base64.h"
#include "Misc/SecureHash.h"
#include "Runtime/Launch/Resources/Version.h"

const FglTFImporter& FglTFImporter::Get()
{
    static const FglTFImporter glTFImporterInstance;
    return glTFImporterInstance;
}

FglTFImporter::FglTFImporter()
{
    //
}

FglTFImporter::~FglTFImporter()
{
    //
}

class FBufferFiles
{
public:
    explicit FBufferFiles(const FString& InFileFolderPath, const std::vector<std::shared_ptr<libgltf::SBuffer>>& InBuffers)
    {
        for (int32 i = 0; i < static_cast<int32>(InBuffers.size()); ++i)
        {
            const std::shared_ptr<libgltf::SBuffer>& Buffer = InBuffers[i];
            if (!Buffer) continue;
            const FString BufferUri = Buffer->uri.c_str();

            FString BufferFileName = BufferUri;
            FString BufferStream = BufferUri;

            if (BufferStream.RemoveFromStart(TEXT("data:application/octet-stream;")))
            {
                BufferFileName = FMD5::HashAnsiString(*BufferStream);
                if (BufferFiles.Find(BufferFileName) != nullptr) continue;

                FString StreamType;
                int32 FirstCommaIndex = 0;
                if (BufferStream.FindChar(TEXT(','), FirstCommaIndex))
                {
                    StreamType = BufferStream.Left(FirstCommaIndex);
                    BufferStream = BufferStream.Right(BufferStream.Len() - (FirstCommaIndex + 1));
                }

                TArray<uint8> BufferData;
                if (StreamType.Equals(TEXT("base64"), ESearchCase::IgnoreCase))
                {
                    if (!FBase64::Decode(BufferStream, BufferData))
                    {
                        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Failed to decode the base64 data!"));
                    }
                    if (BufferData.Num() != Buffer->byteLength)
                    {
                        BufferData.Empty();
                        UE_LOG(LogglTFForUE4Ed, Error, TEXT("The size of data is not same as buffer"));
                    }
                }
                else
                {
                    UE_LOG(LogglTFForUE4Ed, Error, TEXT("Can't decode the type(%s) of the string"), *StreamType);
                }

                IndexToUri.Add(i, BufferFileName);
                BufferFiles.Add(BufferFileName, BufferData);
            }
            else
            {
                if (BufferFiles.Find(BufferFileName) != nullptr) continue;
                const FString BufferFilePath = InFileFolderPath / BufferFileName;
                TArray<uint8> BufferData;
                if (!FFileHelper::LoadFileToArray(BufferData, *BufferFilePath)) continue;
                IndexToUri.Add(i, BufferFileName);
                BufferFiles.Add(BufferFileName, BufferData);
            }
        }
    }

public:
    const TArray<uint8>& operator[](const FString& InKey) const
    {
        const TArray<uint8>* FoundBufferFile = BufferFiles.Find(InKey);
        if (!FoundBufferFile) return EmptyBufferFile;
        return *FoundBufferFile;
    }

    const TArray<uint8>& operator[](int32 InIndex) const
    {
        const FString* UriPtr = IndexToUri.Find(InIndex);
        if (!UriPtr) return EmptyBufferFile;
        return (*this)[*UriPtr];
    }

    template<typename TElem>
    bool Get(int32 InIndex, int32 InStart, int32 InCount, int32 InStride, TArray<TElem>& OutBufferSegment) const
    {
        if (InStride == 0) InStride = sizeof(TElem);
        checkfSlow(sizeof(TElem) > InStride, TEXT("Stride is too smaller!"));
        if (sizeof(TElem) > InStride) return false;
        if (InStart < 0 || InCount <= 0) return false;
        const TArray<uint8>& BufferSegment = (*this)[InIndex];
        if (BufferSegment.Num() < (InStart + InCount * InStride)) return false;

        OutBufferSegment.SetNumUninitialized(InCount);
        if (InStride == sizeof(TElem))
        {
            FMemory::Memcpy((void*)OutBufferSegment.GetData(), (void*)(BufferSegment.GetData() + InStart), InCount * sizeof(TElem));
        }
        else
        {
            for (int32 i = 0; i < InCount; ++i)
            {
                FMemory::Memcpy((void*)(OutBufferSegment.GetData() + i), (void*)(BufferSegment.GetData() + InStart + i * InStride), InStride);
            }
        }
        return true;
    }

private:
    TMap<int32, FString> IndexToUri;
    TMap<FString, TArray<uint8>> BufferFiles;
    const TArray<uint8> EmptyBufferFile;
};

UObject* FglTFImporter::Create(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions
    , const std::shared_ptr<libgltf::SGlTF>& InGlTF
    , UClass* InClass, UObject* InParent, FFeedbackContext* InWarn) const
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
    FBufferFiles BufferFiles(FolderPathInOS, InGlTF->buffers);

    FlushRenderingCommands();

    UObject* StaticMesh = nullptr;
    if (!glTFImportOptions->bImportAllScenes && InGlTF->scene)
    {
        const std::shared_ptr<libgltf::SScene>& Scene = InGlTF->scenes[(int32)(*InGlTF->scene)];
        if (Scene)
        {
            TArray<UStaticMesh*> StaticMeshes;
            if (CreateNode(InglTFImportOptions, Scene->nodes, InGlTF, BufferFiles, StaticMeshes, FText::FromString(FolderPathInOS), InWarn)
                && StaticMeshes.Num() > 0)
            {
                StaticMesh = StaticMeshes[0];
            }
        }
    }
    else if (InGlTF->scenes.size() > 0)
    {
        for (const std::shared_ptr<libgltf::SScene>& Scene : InGlTF->scenes)
        {
            TArray<UStaticMesh*> StaticMeshes;
            if (CreateNode(InglTFImportOptions, Scene->nodes, InGlTF, BufferFiles, StaticMeshes, FText::FromString(FolderPathInOS), InWarn)
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

UStaticMesh* FglTFImporter::CreateStaticMesh(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SMesh>& InMesh, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FBufferFiles& InBufferFiles, FFeedbackContext* InWarn) const
{
    if (!InMesh) return nullptr;

    TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();

    FString ImportedBaseFilename = FPaths::GetBaseFilename(glTFImportOptions->FilePathInOS);

    /// Create new package
    FString PackageName = FPackageName::GetLongPackagePath(glTFImportOptions->FilePathInEngine) / (ImportedBaseFilename + TEXT("_") + (InMesh->name.size() <= 0 ? TEXT("none") : InMesh->name.c_str()));
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

    FStaticMeshSourceModel& SrcModel = StaticMesh->SourceModels[0];
    SrcModel.BuildSettings.bUseMikkTSpace = glTFImportOptions->bUseMikkTSpace;
    SrcModel.BuildSettings.bRecomputeNormals = glTFImportOptions->bRecomputeNormals;
    SrcModel.BuildSettings.bRecomputeTangents = glTFImportOptions->bRecomputeTangents;

    StaticMesh->LightingGuid = FGuid::NewGuid();
    StaticMesh->LightMapResolution = 64;
    StaticMesh->LightMapCoordinateIndex = 1;

    FRawMesh NewRawMesh;
    SrcModel.RawMeshBulkData->LoadRawMesh(NewRawMesh);
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
        if (!SrcModel.BuildSettings.bRecomputeNormals
            && MeshPrimitive->attributes.find(TEXT("NORMAL")) != MeshPrimitive->attributes.cend()
            && !GetVertexNormals(InGlTF, InBufferFiles, *MeshPrimitive->attributes[TEXT("NORMAL")], Normals))
        {
            break;
        }

        TArray<FVector4> Tangents;
        if (!SrcModel.BuildSettings.bRecomputeTangents
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
            SrcModel.BuildSettings.bRecomputeNormals = true;
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
            SrcModel.BuildSettings.bRecomputeTangents = true;
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
            Position = Position * glTFImportOptions->MeshScaleRatio * -1.0f;
        }

        SrcModel.RawMeshBulkData->SaveRawMesh(NewRawMesh);

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
            for (const FText& BuildError : BuildErrors)
            {
                InWarn->Log(ELogVerbosity::Warning, BuildError.ToString());
                //UE_LOG(LogglTFForUE4Ed, Warning, TEXT("BuildError: %s"),  *BuildError.ToString());
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

bool FglTFImporter::CreateNode(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::vector<std::shared_ptr<libgltf::SGlTFId>>& InNodeIndices, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FBufferFiles& InBufferFiles, TArray<UStaticMesh*>& OutStaticMeshes, FText InParentNodeName, FFeedbackContext* InWarn) const
{
    FText ImportMessage = FText::Format(NSLOCTEXT("glTFForUE4Ed", "BeginImportingglTFNodeTask", "Importing a node {0}"), InParentNodeName);
    InWarn->BeginSlowTask(ImportMessage, true);

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

        InWarn->UpdateProgress(static_cast<int32>(i), static_cast<int32>(count));

        const std::shared_ptr<libgltf::SGlTFId>& MeshIndex = Node->mesh;
        if (MeshIndex)
        {
            const std::shared_ptr<libgltf::SMesh>& Mesh = InGlTF->meshes[(int32)(*MeshIndex)];

            if (Mesh)
            {
                InWarn->StatusUpdate(static_cast<int32>(i), static_cast<int32>(count), FText::FromString(Mesh->name.c_str()));
            }

            UStaticMesh* NewStaticMesh = CreateStaticMesh(InglTFImportOptions, Mesh, InGlTF, InBufferFiles, InWarn);
            if (NewStaticMesh)
            {
                OutStaticMeshes.Add(NewStaticMesh);
            }
        }

        CreateNode(InglTFImportOptions, Node->children, InGlTF, InBufferFiles, OutStaticMeshes, FText::FromString(Node->name.c_str()), InWarn);

        InWarn->UpdateProgress(static_cast<int32>(i + 1), static_cast<int32>(count));
    }

    InWarn->EndSlowTask();
    return (bIsSuccess && OutStaticMeshes.Num() > 0);
}

template<typename TEngineDataType>
struct TAccessorTypeScale
{
    TEngineDataType X;

    operator TEngineDataType() const
    {
        return X;
    }

    operator FVector2D() const
    {
        throw 1;
        return FVector2D(X, 0.0f);
    }

    operator FVector() const
    {
        throw 1;
        return FVector(X, 0.0f, 0.0f);
    }

    operator FVector4() const
    {
        throw 1;
        return FVector4(X, 0.0f, 0.0f, 0.0f);
    }
};

template<typename TEngineDataType>
struct TAccessorTypeVec2
{
    TEngineDataType X;
    TEngineDataType Y;

    operator TEngineDataType() const
    {
        throw 1;
        return X;
    }

    operator FVector2D() const
    {
        return FVector2D(X, Y);
    }

    operator FVector() const
    {
        throw 1;
        return FVector(X, Y, 0.0f);
    }

    operator FVector4() const
    {
        throw 1;
        return FVector4(X, Y, 0.0f, 0.0f);
    }
};

template<typename TEngineDataType>
struct TAccessorTypeVec3
{
    TEngineDataType X;
    TEngineDataType Y;
    TEngineDataType Z;

    operator TEngineDataType() const
    {
        throw 1;
        return X;
    }

    operator FVector2D() const
    {
        throw 1;
        return FVector2D(X, Y);
    }

    operator FVector() const
    {
        return FVector(X, Y, Z);
    }

    operator FVector4() const
    {
        throw 1;
        return FVector4(X, Y, Z, 0.0f);
    }
};

template<typename TEngineDataType>
struct TAccessorTypeVec4
{
    TEngineDataType X;
    TEngineDataType Y;
    TEngineDataType Z;
    TEngineDataType W;

    operator TEngineDataType() const
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return X;
    }

    operator FVector2D() const
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return FVector2D(X, Y);
    }

    operator FVector() const
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return FVector(X, Y, Z);
    }

    operator FVector4() const
    {
        return FVector4(X, Y, Z, W);
    }
};

template<typename TAccessorDataType, typename TEngineDataType>
bool GetAccessorData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FBufferFiles& InBufferFiles, const std::shared_ptr<libgltf::SAccessor>& InAccessor, TArray<TEngineDataType>& OutDataArray)
{
    if (!InAccessor) return false;

    const std::shared_ptr<libgltf::SBufferView>& BufferView = InGlTF->bufferViews[(int32)(*InAccessor->bufferView)];
    if (!BufferView) return false;

    if (OutDataArray.Num() > 0)
    {
        OutDataArray.Empty();
    }

    if (InAccessor->type == TEXT("SCALAR"))
    {
        TArray<TAccessorTypeScale<TAccessorDataType>> AccessorDataArray;
        if (InBufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset + InAccessor->byteOffset, InAccessor->count, BufferView->byteStride, AccessorDataArray))
        {
            try
            {
                for (const TAccessorTypeScale<TAccessorDataType>& AccessorDataItem : AccessorDataArray)
                {
                    OutDataArray.Add(AccessorDataItem);
                }
            }
            catch (...)
            {
                UE_LOG(LogglTFForUE4Ed, Error, TEXT("Can't convert the accessor's data(TAccessorTypeScale) to the engine's data?"));
                return false;
            }
        }
        else
        {
            UE_LOG(LogglTFForUE4Ed, Error, TEXT("Your glTF file has some errors?"));
            return false;
        }
    }
    else if (InAccessor->type == TEXT("VEC2"))
    {
        TArray<TAccessorTypeVec2<TAccessorDataType>> AccessorDataArray;
        if (InBufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset + InAccessor->byteOffset, InAccessor->count, BufferView->byteStride, AccessorDataArray))
        {
            try
            {
                for (const TAccessorTypeVec2<TAccessorDataType>& AccessorDataItem : AccessorDataArray)
                {
                    OutDataArray.Add(AccessorDataItem);
                }
            }
            catch (...)
            {
                UE_LOG(LogglTFForUE4Ed, Error, TEXT("Can't convert the accessor's data(TAccessorTypeVec2) to the engine's data?"));
                return false;
            }
        }
        else
        {
            UE_LOG(LogglTFForUE4Ed, Error, TEXT("Your glTF file has some errors?"));
            return false;
        }
    }
    else if (InAccessor->type == TEXT("VEC3"))
    {
        TArray<TAccessorTypeVec3<TAccessorDataType>> AccessorDataArray;
        if (InBufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset + InAccessor->byteOffset, InAccessor->count, BufferView->byteStride, AccessorDataArray))
        {
            try
            {
                for (const TAccessorTypeVec3<TAccessorDataType>& AccessorDataItem : AccessorDataArray)
                {
                    OutDataArray.Add(AccessorDataItem);
                }
            }
            catch (...)
            {
                UE_LOG(LogglTFForUE4Ed, Error, TEXT("Can't convert the accessor's data(TAccessorTypeVec3) to the engine's data?"));
                return false;
            }
        }
        else
        {
            UE_LOG(LogglTFForUE4Ed, Error, TEXT("Your glTF file has some errors?"));
            return false;
        }
    }
    else if (InAccessor->type == TEXT("VEC4"))
    {
        TArray<TAccessorTypeVec4<TAccessorDataType>> AccessorDataArray;
        if (InBufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset + InAccessor->byteOffset, InAccessor->count, BufferView->byteStride, AccessorDataArray))
        {
            try
            {
                for (const TAccessorTypeVec4<TAccessorDataType>& AccessorDataItem : AccessorDataArray)
                {
                    OutDataArray.Add(AccessorDataItem);
                }
            }
            catch (...)
            {
                UE_LOG(LogglTFForUE4Ed, Error, TEXT("Can't convert the accessor's data(TAccessorTypeVec4) to the engine's data?"));
                return false;
            }
        }
        else
        {
            UE_LOG(LogglTFForUE4Ed, Error, TEXT("Your glTF file has some errors?"));
            return false;
        }
    }
    else
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Not supports the accessor's type(%s)!"), InAccessor->type.c_str());
        return false;
    }
    return true;
}

template<typename TEngineDataType>
bool GetAccessorData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FBufferFiles& InBufferFiles, const std::shared_ptr<libgltf::SAccessor>& InAccessor, TArray<TEngineDataType>& OutDataArray)
{
    if (!InAccessor) return false;

    switch (InAccessor->componentType)
    {
    case 5120:
        return GetAccessorData<int8>(InGlTF, InBufferFiles, InAccessor, OutDataArray);

    case 5121:
        return GetAccessorData<uint8>(InGlTF, InBufferFiles, InAccessor, OutDataArray);

    case 5122:
        return GetAccessorData<int16>(InGlTF, InBufferFiles, InAccessor, OutDataArray);

    case 5123:
        return GetAccessorData<uint16>(InGlTF, InBufferFiles, InAccessor, OutDataArray);

    case 5125:
        return GetAccessorData<int32>(InGlTF, InBufferFiles, InAccessor, OutDataArray);

    case 5126:
        return GetAccessorData<float>(InGlTF, InBufferFiles, InAccessor, OutDataArray);

    default:
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Not support the accessor's componetType(%d)?"), InAccessor->componentType);
        break;
    }
    return false;
}

bool FglTFImporter::GetTriangleIndices(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<uint32>& OutTriangleIndices)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutTriangleIndices);
}

bool FglTFImporter::GetVertexPositions(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector>& OutVertexPositions)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexPositions);
}

bool FglTFImporter::GetVertexNormals(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector>& OutVertexNormals)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexNormals);
}

bool FglTFImporter::GetVertexTangents(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector4>& OutVertexTangents)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexTangents);
}

bool FglTFImporter::GetVertexTexcoords(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector2D>& OutVertexTexcoords)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexTexcoords);
}
