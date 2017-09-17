#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporter.h"

#include "glTF/glTFImportOptions.h"

#include "libgltf/libgltf.h"

#include "RenderingThread.h"
#include "RawMesh.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"

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
            const FString BufferFileName = Buffer->uri.c_str();
            if (BufferFiles.Find(BufferFileName) != nullptr) continue;
            const FString BufferFilePath = InFileFolderPath / BufferFileName;
            TArray<uint8> BufferFileData;
            if (!FFileHelper::LoadFileToArray(BufferFileData, *BufferFilePath)) continue;
            IndexToUri.Add(i, BufferFileName);
            BufferFiles.Add(BufferFileName, BufferFileData);
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
    bool Get(int32 InIndex, int32 InStart, int32 InLength, TArray<TElem>& OutBufferSegment)
    {
        if ((sizeof(TElem) % sizeof(uint8)) != 0) return false;
        if (InStart < 0 || InLength <= 0 || (InLength % (sizeof(TElem) / sizeof(uint8))) != 0) return false;
        const TArray<uint8>& BufferSegment = (*this)[InIndex];
        if (BufferSegment.Num() < (InStart + InLength)) return false;

        OutBufferSegment.SetNumUninitialized(InLength / (sizeof(TElem) / sizeof(uint8)));
        FMemory::Memcpy((void*)OutBufferSegment.GetData(), (void*)(BufferSegment.GetData() + InStart), sizeof(uint8) * InLength);
        return true;
    }

private:
    TMap<int32, FString> IndexToUri;
    TMap<FString, TArray<uint8>> BufferFiles;
    const TArray<uint8> EmptyBufferFile;
};

UObject* FglTFImporter::CreateMesh(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions
    , const std::shared_ptr<libgltf::SGlTF>& InGlTF
    , UClass* InClass, UObject* InParent) const
{
    if (!InGlTF)
    {
        UE_LOG(LogglTFForUE4Ed, Error, TEXT("Invalid InGlTF!"));
        return nullptr;
    }

    TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();

    FlushRenderingCommands();

    /// Create new package
    FString PackageName = glTFImportOptions->FilePathInEngine;
    UPackage* Package = FindPackage(nullptr, *PackageName);
    if (!Package)
    {
        Package = CreatePackage(nullptr, *PackageName);
    }

    FName StaticMeshName = FPackageName::GetShortFName(*PackageName);
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, StaticMeshName, RF_Public | RF_Standalone);
    StaticMesh->SourceModels.Empty();
    new(StaticMesh->SourceModels) FStaticMeshSourceModel();

    FStaticMeshSourceModel& SrcModel = StaticMesh->SourceModels[0];
    SrcModel.BuildSettings.bRecomputeNormals = glTFImportOptions->bRecomputeNormals;
    SrcModel.BuildSettings.bRecomputeTangents = glTFImportOptions->bRecomputeTangents;

    StaticMesh->LightingGuid = FGuid::NewGuid();
    StaticMesh->LightMapResolution = 64;
    StaticMesh->LightMapCoordinateIndex = 1;

    FRawMesh NewRawMesh;
    SrcModel.RawMeshBulkData->LoadRawMesh(NewRawMesh);
    NewRawMesh.Empty();

    const FString FolderPathInOS = FPaths::GetPath(glTFImportOptions->FilePathInOS);
    FBufferFiles BufferFiles(FolderPathInOS, InGlTF->buffers);

    if (InGlTF->scene)
    {
        const std::shared_ptr<libgltf::SScene>& Scene = InGlTF->scenes[(int32)(*InGlTF->scene)];
        for (const std::shared_ptr<libgltf::SGlTFId>& NodeIndex : Scene->nodes)
        {
            if (!NodeIndex) continue;
            const std::shared_ptr<libgltf::SNode>& Node = InGlTF->nodes[(int32)(*NodeIndex)];
            if (!Node) continue;
            const std::shared_ptr<libgltf::SGlTFId>& MeshIndex = Node->mesh;
            if (!MeshIndex) continue;
            const std::shared_ptr<libgltf::SMesh>& Mesh = InGlTF->meshes[(int32)(*MeshIndex)];
            for (const std::shared_ptr<libgltf::SMeshPrimitive>& MeshPrimitive : Mesh->primitives)
            {
                {
                    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)(*MeshPrimitive->indices)];
                    const std::shared_ptr<libgltf::SBufferView>& BufferView = InGlTF->bufferViews[(int32)(*Accessor->bufferView)];
                    if (Accessor->componentType == 5120)
                    {
                        TArray<int8> WedgeIndices;
                        BufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset, BufferView->byteLength, WedgeIndices);
                        NewRawMesh.WedgeIndices = TArray<uint32>(WedgeIndices);
                    }
                    else if (Accessor->componentType == 5121)
                    {
                        TArray<uint8> WedgeIndices;
                        BufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset, BufferView->byteLength, WedgeIndices);
                        NewRawMesh.WedgeIndices = TArray<uint32>(WedgeIndices);
                    }
                    else if (Accessor->componentType == 5122)
                    {
                        TArray<int16> WedgeIndices;
                        BufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset, BufferView->byteLength, WedgeIndices);
                        NewRawMesh.WedgeIndices = TArray<uint32>(WedgeIndices);
                    }
                    else if (Accessor->componentType == 5123)
                    {
                        TArray<uint16> WedgeIndices;
                        BufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset, BufferView->byteLength, WedgeIndices);
                        NewRawMesh.WedgeIndices = TArray<uint32>(WedgeIndices);
                    }
                    else if (Accessor->componentType == 5125)
                    {
                        BufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset, BufferView->byteLength, NewRawMesh.WedgeIndices);
                    }
                }
                if (MeshPrimitive->attributes.find(TEXT("POSITION")) != MeshPrimitive->attributes.cend())
                {
                    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)(*MeshPrimitive->attributes[TEXT("POSITION")])];
                    const std::shared_ptr<libgltf::SBufferView>& BufferView = InGlTF->bufferViews[(int32)(*Accessor->bufferView)];
                    BufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset, BufferView->byteLength, NewRawMesh.VertexPositions);
                }
                if (MeshPrimitive->attributes.find(TEXT("NORMAL")) != MeshPrimitive->attributes.cend())
                {
                    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)(*MeshPrimitive->attributes[TEXT("NORMAL")])];
                    const std::shared_ptr<libgltf::SBufferView>& BufferView = InGlTF->bufferViews[(int32)(*Accessor->bufferView)];
                    BufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset, BufferView->byteLength, NewRawMesh.WedgeTangentZ);
                }
                if (MeshPrimitive->attributes.find(TEXT("TANGENT")) != MeshPrimitive->attributes.cend())
                {
                    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)(*MeshPrimitive->attributes[TEXT("TANGENT")])];
                    const std::shared_ptr<libgltf::SBufferView>& BufferView = InGlTF->bufferViews[(int32)(*Accessor->bufferView)];
                    TArray<FVector4> Tangent;
                    if (BufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset, BufferView->byteLength, Tangent))
                    {
                        //
                    }
                }
                if (MeshPrimitive->attributes.find(TEXT("TEXCOORD_0")) != MeshPrimitive->attributes.cend())
                {
                    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)(*MeshPrimitive->attributes[TEXT("TEXCOORD_0")])];
                    const std::shared_ptr<libgltf::SBufferView>& BufferView = InGlTF->bufferViews[(int32)(*Accessor->bufferView)];
                    BufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset, BufferView->byteLength, NewRawMesh.WedgeTexCoords[0]);
                }
                break;
            }
            break;
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
            if (NewRawMesh.WedgeTexCoords[0].Num() == NewRawMesh.VertexPositions.Num())
            {
                TArray<FVector2D> WedgeTexCoords = NewRawMesh.WedgeTexCoords[0];
                NewRawMesh.WedgeTexCoords[0].Empty();
                NewRawMesh.WedgeTexCoords[0].SetNumUninitialized(WedgeIndicesCount);
                for (int32 i = 0; i < NewRawMesh.WedgeIndices.Num(); ++i)
                {
                    NewRawMesh.WedgeTexCoords[0][i] = WedgeTexCoords[NewRawMesh.WedgeIndices[i]];
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
            StaticMesh->Materials.Add(UMaterial::GetDefaultMaterial(MD_Surface));
        }
        else
        {
            StaticMesh->Materials.Add(UMaterial::GetDefaultMaterial(MD_Surface));
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
                    if (StaticMesh->Materials.IsValidIndex(Info.MaterialIndex))
                    {
                        StaticMesh->SectionInfoMap.Set(LODResoureceIndex, SectionIndex, Info);
                    }
                }
            }
        }
        else
        {
            StaticMesh = nullptr;
        }
    }
    else
    {
        StaticMesh = nullptr;
    }
    return StaticMesh;
}
