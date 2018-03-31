// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImporter.h"

#include "glTF/glTFImportOptions.h"

#include "Misc/Base64.h"
#include "Misc/SecureHash.h"

#if defined(ERROR)
#define DRACO_MACRO_TEMP_ERROR      ERROR
#undef ERROR
#endif

#include "draco/compression/decode.h"

#if defined(DRACO_MACRO_TEMP_ERROR)
#define ERROR           DRACO_MACRO_TEMP_ERROR
#undef DRACO_MACRO_TEMP_ERROR
#endif

namespace glTFForUE4
{
    FFeedbackTaskWrapper::FFeedbackTaskWrapper(FFeedbackContext* InFeedbackContext, const FText& InTask, bool InShowProgressDialog)
        : FeedbackContext(InFeedbackContext)
    {
        if (FeedbackContext)
        {
            FeedbackContext->BeginSlowTask(InTask, InShowProgressDialog);
        }
    }

    FFeedbackTaskWrapper::~FFeedbackTaskWrapper()
    {
        if (FeedbackContext)
        {
            FeedbackContext->EndSlowTask();
            FeedbackContext = nullptr;
        }
    }

    const FFeedbackTaskWrapper& FFeedbackTaskWrapper::Log(ELogVerbosity::Type InLogVerbosity, const FText& InMessge) const
    {
        if (FeedbackContext)
        {
            FeedbackContext->Log(InLogVerbosity, *InMessge.ToString());
        }
        return *this;
    }

    const FFeedbackTaskWrapper& FFeedbackTaskWrapper::UpdateProgress(int32 InNumerator, int32 InDenominator) const
    {
        if (FeedbackContext)
        {
            FeedbackContext->UpdateProgress(InNumerator, InDenominator);
        }
        return *this;
    }

    const FFeedbackTaskWrapper& FFeedbackTaskWrapper::StatusUpdate(int32 InNumerator, int32 InDenominator, const FText& InStatusText) const
    {
        if (FeedbackContext)
        {
            FeedbackContext->StatusUpdate(InNumerator, InDenominator, InStatusText);
        }
        return *this;
    }

    const FFeedbackTaskWrapper& FFeedbackTaskWrapper::StatusForceUpdate(int32 InNumerator, int32 InDenominator, const FText& InStatusText) const
    {
        if (FeedbackContext)
        {
            FeedbackContext->StatusForceUpdate(InNumerator, InDenominator, InStatusText);
        }
        return *this;
    }
}

FglTFBufferData::FglTFBufferData(const TArray<uint8>& InData)
    : Data(InData)
    , FilePath(TEXT(""))
{
    //
}

FglTFBufferData::FglTFBufferData(const FString& InFileFolderRoot, const FString& InUri)
    : Data()
    , FilePath(TEXT(""))
    , StreamType(TEXT(""))
    , StreamEncoding(TEXT(""))
{
    static const FString UriStreamHead(TEXT("data:"));
    int32 StreamDataStartIndex = INDEX_NONE;
    if (InUri.StartsWith(UriStreamHead, ESearchCase::IgnoreCase))
    {
        int32 StreamTypeStartIndex = UriStreamHead.Len();
        int32 StreamTypeEndIndex = InUri.Find(TEXT(";"), ESearchCase::IgnoreCase, ESearchDir::FromStart, StreamTypeStartIndex);
        if (StreamTypeEndIndex != INDEX_NONE)
        {
            StreamType = InUri.Mid(StreamTypeStartIndex, StreamTypeEndIndex - StreamTypeStartIndex);
        }
        int32 StreamEncodingStartIndex = StreamTypeEndIndex + 1;
        int32 StreamEncodingEndIndex = InUri.Find(TEXT(","), ESearchCase::IgnoreCase, ESearchDir::FromStart, StreamEncodingStartIndex);
        if (StreamEncodingEndIndex != INDEX_NONE)
        {
            StreamEncoding = InUri.Mid(StreamEncodingStartIndex, StreamEncodingEndIndex - StreamEncodingStartIndex);

            StreamDataStartIndex = StreamEncodingEndIndex + 1;
        }
    }

    if (StreamDataStartIndex != INDEX_NONE)
    {
        if (!FBase64::Decode(InUri.RightChop(StreamDataStartIndex), Data))
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Failed to decode the base64 data!"));
        }
    }
    else
    {
        if (FFileHelper::LoadFileToArray(Data, *(InFileFolderRoot / InUri)))
        {
            FilePath = InFileFolderRoot / InUri;
        }
    }
}

FglTFBufferData::~FglTFBufferData()
{
    //
}

FglTFBufferData::operator bool() const
{
    return (Data.Num() > 0);
}

const TArray<uint8>& FglTFBufferData::GetData() const
{
    return Data;
}

bool FglTFBufferData::IsFromFile() const
{
    return !FilePath.IsEmpty();
}

const FString& FglTFBufferData::GetFilePath() const
{
    return FilePath;
}

FglTFBuffers::FglTFBuffers(bool InConstructByBinary /*= false*/)
    : bConstructByBinary(InConstructByBinary)
    , Datas()
    , DataEmpty()
{
    //
}

FglTFBuffers::~FglTFBuffers()
{
    //
}

bool FglTFBuffers::CacheBinary(uint32 InIndex, const TArray<uint8>& InData)
{
    IndexToIndex[EglTFBufferSource::Binaries].Add(InIndex, Datas.Num());
    Datas.Add(MakeShareable(new FglTFBufferData(InData)));
    return true;
}

bool FglTFBuffers::CacheImages(uint32 InIndex, const FString& InFileFolderRoot, const std::shared_ptr<libgltf::SImage>& InImage)
{
    if (!InImage) return false;
    IndexToIndex[EglTFBufferSource::Images].Add(InIndex, Datas.Num());
    Datas.Add(MakeShareable(new FglTFBufferData(InFileFolderRoot, InImage->uri.c_str())));
    return true;
}

bool FglTFBuffers::CacheBuffers(uint32 InIndex, const FString& InFileFolderRoot, const std::shared_ptr<libgltf::SBuffer>& InBuffer)
{
    if (!InBuffer) return false;
    IndexToIndex[EglTFBufferSource::Buffers].Add(InIndex, Datas.Num());
    Datas.Add(MakeShareable(new FglTFBufferData(InFileFolderRoot, InBuffer->uri.c_str())));
    return true;
}

bool FglTFBuffers::Cache(const FString& InFileFolderRoot, const std::shared_ptr<libgltf::SGlTF>& InglTF)
{
    if (!InglTF) return false;
    {
        uint32 Index = 0;
        for (const std::shared_ptr<libgltf::SImage>& Image : InglTF->images)
        {
            CacheImages(Index++, InFileFolderRoot, Image);
        }
    }
    {
        uint32 Index = 0;
        for (const std::shared_ptr<libgltf::SBuffer>& Buffer : InglTF->buffers)
        {
            CacheBuffers(Index++, InFileFolderRoot, Buffer);
        }
    }
    return true;
}

class FglTFBufferDecoder
{
    struct FDracoMeshPointIndeies
    {
        FDracoMeshPointIndeies()
            : Vertex(0)
            , Normal(0)
            , TexCoord(0)
            , NewVertex(0)
        {
            //
        }

        uint32 Vertex;
        uint32 Normal;
        uint32 TexCoord;
        uint32 NewVertex;

        bool operator==(const FDracoMeshPointIndeies& InOther) const
        {
            return InOther.Vertex == Vertex && InOther.Normal == Vertex && InOther.TexCoord == TexCoord;
        }
    };

public:
    static bool Decode(const FglTFBuffers& InBuffers, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const libgltf::SKHR_draco_mesh_compressionextension* const InExtensionDraco, const TArray<uint8>& InEncodedBuffer, TArray<uint32>& OutTriangleIndices, TArray<FVector>& OutVertexPositions, TArray<FVector>& OutVertexNormals, TArray<FVector4>& OutVertexTangents, TArray<FVector2D> OutVertexTexcoords[MAX_STATIC_TEXCOORDS])
    {
        if (!InMeshPrimitive || !InExtensionDraco) return false;

        draco::DecoderBuffer DracoDecoderBuffer;
        DracoDecoderBuffer.Init((const char*)InEncodedBuffer.GetData(), InEncodedBuffer.GetAllocatedSize());
        auto StatusOrGeometryType = draco::Decoder::GetEncodedGeometryType(&DracoDecoderBuffer);
        if (!StatusOrGeometryType.ok()) return false;
        if (StatusOrGeometryType.value() != draco::TRIANGULAR_MESH) return false;
        draco::Decoder DracoDecoder;
        auto DracoStatusOrMesh = DracoDecoder.DecodeMeshFromBuffer(&DracoDecoderBuffer);
        if (!DracoStatusOrMesh.ok()) return false;
        std::unique_ptr<draco::Mesh> DracoMesh = std::move(DracoStatusOrMesh).value();

        const draco::PointAttribute* const DracoPointAttributePosition = DracoMesh->GetNamedAttribute(draco::GeometryAttribute::POSITION);
        if (!DracoPointAttributePosition) return false;

        uint32 VertexNumber = DracoPointAttributePosition->size();

        const draco::PointAttribute* const DracoPointAttributeNormal = DracoMesh->GetNamedAttribute(draco::GeometryAttribute::NORMAL);
        const draco::PointAttribute* const DracoPointAttributeTexCoord = DracoMesh->GetNamedAttribute(draco::GeometryAttribute::TEX_COORD);

        TArray<uint32> PositionIndeies;
        OutTriangleIndices.SetNumUninitialized(DracoMesh->num_faces() * 3);
        for (draco::FaceIndex i(0); i < DracoMesh->num_faces(); ++i)
        {
            const draco::Mesh::Face& DracoFace = DracoMesh->face(i);

            for (uint32 j = 0; j < 3; ++j)
            {
                const uint32 DracoPointAttributePositionIndex = DracoFace[j].value();
                OutTriangleIndices[i.value() * 3 + j] = DracoPointAttributePositionIndex;
                PositionIndeies.AddUnique(DracoPointAttributePositionIndex);
            }
        }

        {
            float Position[3] = { 0.0f, 0.0f, 0.0f };
            OutVertexPositions.SetNumUninitialized(DracoPointAttributePosition->indices_map_size());
            for (draco::PointIndex i(0); i < DracoPointAttributePosition->indices_map_size(); ++i)
            {
                DracoPointAttributePosition->GetMappedValue(i, Position);
                OutVertexPositions[i.value()] = FVector(Position[0], Position[1], Position[2]);
            }
        }

        if (DracoPointAttributeNormal)
        {
            float Normal[3] = { 0.0f, 0.0f, 0.0f };
            OutVertexNormals.SetNumUninitialized(DracoPointAttributeNormal->indices_map_size());
            for (draco::PointIndex i(0); i < DracoPointAttributeNormal->indices_map_size(); ++i)
            {
                DracoPointAttributeNormal->GetMappedValue(i, Normal);
                OutVertexNormals[i.value()] = FVector(Normal[0], Normal[1], Normal[2]);
            }
        }

        if (DracoPointAttributeTexCoord)
        {
            float TexCoord[2] = { 0.0f, 0.0f };
            OutVertexTexcoords[0].SetNumUninitialized(DracoPointAttributeTexCoord->indices_map_size());
            for (draco::PointIndex i(0); i < DracoPointAttributeTexCoord->indices_map_size(); ++i)
            {
                DracoPointAttributeTexCoord->GetMappedValue(i, TexCoord);
                OutVertexTexcoords[0][i.value()] = FVector2D(TexCoord[0], TexCoord[1]);
            }
        }

        return true;
    }
};

TSharedPtr<FglTFImporter> FglTFImporter::Get(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporter> glTFImporter = MakeShareable(new FglTFImporter);
    glTFImporter->Set(InClass, InParent, InName, InFlags, InFeedbackContext);
    return glTFImporter;
}

FglTFImporter::FglTFImporter()
    : InputClass(nullptr)
    , InputParent(nullptr)
    , InputName()
    , InputFlags(RF_NoFlags)
    , FeedbackContext(nullptr)
{
    //
}

FglTFImporter::~FglTFImporter()
{
    //
}

FglTFImporter& FglTFImporter::Set(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext)
{
    InputClass = InClass;
    InputParent = InParent;
    InputName = InName;
    InputFlags = InFlags;
    FeedbackContext = InFeedbackContext;
    return *this;
}

UObject* FglTFImporter::Create(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InglTFBuffers) const
{
    if (!InGlTF)
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Invalid InGlTF!"));
        return nullptr;
    }

    if (!InGlTF->asset || InGlTF->asset->version != TEXT("2.0"))
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Invalid version: %s!"), !(InGlTF->asset) ? TEXT("none") : InGlTF->asset->version.c_str());
        return nullptr;
    }

    TSharedPtr<FglTFImportOptions> glTFImportOptions = InglTFImportOptions.Pin();

    const FString FolderPathInOS = FPaths::GetPath(glTFImportOptions->FilePathInOS);
    FglTFBuffers glTFBuffers;
    glTFBuffers.Cache(FolderPathInOS, InGlTF);

    //TODO: generate the procedural mesh

    return nullptr;
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
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return FVector2D(X, 0.0f);
    }

    operator FVector() const
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return FVector(X, 0.0f, 0.0f);
    }

    operator FVector4() const
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
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
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return X;
    }

    operator FVector2D() const
    {
        return FVector2D(X, Y);
    }

    operator FVector() const
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return FVector(X, Y, 0.0f);
    }

    operator FVector4() const
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
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
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return X;
    }

    operator FVector2D() const
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return FVector2D(X, Y);
    }

    operator FVector() const
    {
        return FVector(X, Y, Z);
    }

    operator FVector4() const
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
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
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return X;
    }

    operator FVector2D() const
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return FVector2D(X, Y);
    }

    operator FVector() const
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
        return FVector(X, Y, Z);
    }

    operator FVector4() const
    {
        return FVector4(X, Y, Z, W);
    }
};

template<typename TAccessorDataType, typename TEngineDataType>
bool GetAccessorData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InBuffers, const std::shared_ptr<libgltf::SAccessor>& InAccessor, TArray<TEngineDataType>& OutDataArray)
{
    if (!InAccessor || !InAccessor->bufferView) return false;

    int32 BufferViewIndex = (*InAccessor->bufferView);

    OutDataArray.Empty();

    FString FilePath;
    if (InAccessor->type == TEXT("SCALAR"))
    {
        TArray<TAccessorTypeScale<TAccessorDataType>> AccessorDataArray;
        if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, AccessorDataArray, FilePath, InAccessor->byteOffset, InAccessor->count))
        {
            for (const TAccessorTypeScale<TAccessorDataType>& AccessorDataItem : AccessorDataArray)
            {
                OutDataArray.Add(AccessorDataItem);
            }
        }
        else
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Your glTF file has some errors?"));
            return false;
        }
    }
    else if (InAccessor->type == TEXT("VEC2"))
    {
        TArray<TAccessorTypeVec2<TAccessorDataType>> AccessorDataArray;
        if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, AccessorDataArray, FilePath, InAccessor->byteOffset, InAccessor->count))
        {
            for (const TAccessorTypeVec2<TAccessorDataType>& AccessorDataItem : AccessorDataArray)
            {
                OutDataArray.Add(AccessorDataItem);
            }
        }
        else
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Your glTF file has some errors?"));
            return false;
        }
    }
    else if (InAccessor->type == TEXT("VEC3"))
    {
        TArray<TAccessorTypeVec3<TAccessorDataType>> AccessorDataArray;
        if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, AccessorDataArray, FilePath, InAccessor->byteOffset, InAccessor->count))
        {
            for (const TAccessorTypeVec3<TAccessorDataType>& AccessorDataItem : AccessorDataArray)
            {
                OutDataArray.Add(AccessorDataItem);
            }
        }
        else
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Your glTF file has some errors?"));
            return false;
        }
    }
    else if (InAccessor->type == TEXT("VEC4"))
    {
        TArray<TAccessorTypeVec4<TAccessorDataType>> AccessorDataArray;
        if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, AccessorDataArray, FilePath, InAccessor->byteOffset, InAccessor->count))
        {
            for (const TAccessorTypeVec4<TAccessorDataType>& AccessorDataItem : AccessorDataArray)
            {
                OutDataArray.Add(AccessorDataItem);
            }
        }
        else
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Your glTF file has some errors?"));
            return false;
        }
    }
    else
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Not supports the accessor's type(%s)!"), InAccessor->type.c_str());
        return false;
    }
    return true;
}

template<typename TEngineDataType>
bool GetAccessorData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InBuffers, const std::shared_ptr<libgltf::SAccessor>& InAccessor, TArray<TEngineDataType>& OutDataArray)
{
    if (!InAccessor) return false;

    switch (InAccessor->componentType)
    {
    case 5120:
        return GetAccessorData<int8>(InGlTF, InBuffers, InAccessor, OutDataArray);

    case 5121:
        return GetAccessorData<uint8>(InGlTF, InBuffers, InAccessor, OutDataArray);

    case 5122:
        return GetAccessorData<int16>(InGlTF, InBuffers, InAccessor, OutDataArray);

    case 5123:
        return GetAccessorData<uint16>(InGlTF, InBuffers, InAccessor, OutDataArray);

    case 5125:
        return GetAccessorData<int32>(InGlTF, InBuffers, InAccessor, OutDataArray);

    case 5126:
        return GetAccessorData<float>(InGlTF, InBuffers, InAccessor, OutDataArray);

    default:
        UE_LOG(LogglTFForUE4, Error, TEXT("Not support the accessor's componetType(%d)?"), InAccessor->componentType);
        break;
    }
    return false;
}

bool FglTFImporter::GetMeshData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<uint32>& OutTriangleIndices, TArray<FVector>& OutVertexPositions, TArray<FVector>& OutVertexNormals, TArray<FVector4>& OutVertexTangents, TArray<FVector2D> OutVertexTexcoords[MAX_STATIC_TEXCOORDS])
{
    OutTriangleIndices.Empty();
    OutVertexPositions.Empty();
    OutVertexNormals.Empty();
    OutVertexTangents.Empty();
    for (uint32 i = 0; i < MAX_STATIC_TEXCOORDS; ++i)
    {
        OutVertexTexcoords[i].Empty();
    }

    if (!InGlTF || !InMeshPrimitive) return false;

    const libgltf::SKHR_draco_mesh_compressionextension* ExtensionDraco = nullptr;
    {
        const std::shared_ptr<libgltf::SExtension>& Extensions = InMeshPrimitive->extensions;
        if (!!Extensions && (Extensions->properties.find(TEXT("KHR_draco_mesh_compression")) != Extensions->properties.end()))
        {
            ExtensionDraco = (const libgltf::SKHR_draco_mesh_compressionextension*)Extensions->properties[TEXT("KHR_draco_mesh_compression")].get();
        }
    }
    if (ExtensionDraco)
    {
        int32 BufferViewIndex = *(ExtensionDraco->bufferView);
        TArray<uint8> EncodeBuffer;
        FString BufferFilePath;
        return InBufferFiles.GetBufferViewData(InGlTF, BufferViewIndex, EncodeBuffer, BufferFilePath)
            && FglTFBufferDecoder::Decode(InBufferFiles, InMeshPrimitive, ExtensionDraco, EncodeBuffer, OutTriangleIndices, OutVertexPositions, OutVertexNormals, OutVertexTangents, OutVertexTexcoords);
    }
    else
    {
        if (!GetTriangleIndices(InGlTF, InMeshPrimitive, InBufferFiles, OutTriangleIndices))
        {
            return false;
        }
        if (!GetVertexPositions(InGlTF, InMeshPrimitive, InBufferFiles, OutVertexPositions))
        {
            return false;
        }
        if (!GetVertexNormals(InGlTF, InMeshPrimitive, InBufferFiles, OutVertexNormals))
        {
            return false;
        }
        if (!GetVertexTangents(InGlTF, InMeshPrimitive, InBufferFiles, OutVertexTangents))
        {
            return false;
        }
        if (!GetVertexTexcoords(InGlTF, InMeshPrimitive, InBufferFiles, OutVertexTexcoords))
        {
            return false;
        }
    }
    return true;
}

bool FglTFImporter::GetTriangleIndices(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<uint32>& OutTriangleIndices)
{
    if (!InGlTF || !InMeshPrimitive) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->indices)];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutTriangleIndices);
}

bool FglTFImporter::GetVertexPositions(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<FVector>& OutVertexPositions)
{
    if (!InGlTF || !InMeshPrimitive) return false;
    if (InMeshPrimitive->attributes.find(TEXT("POSITION")) == InMeshPrimitive->attributes.cend()) return true;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->attributes[TEXT("POSITION")])];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexPositions);
}

bool FglTFImporter::GetVertexNormals(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<FVector>& OutVertexNormals)
{
    if (!InGlTF || !InMeshPrimitive) return false;
    if (InMeshPrimitive->attributes.find(TEXT("NORMAL")) == InMeshPrimitive->attributes.cend()) return true;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->attributes[TEXT("NORMAL")])];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexNormals);
}

bool FglTFImporter::GetVertexTangents(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<FVector4>& OutVertexTangents)
{
    if (!InGlTF || !InMeshPrimitive) return false;
    if (InMeshPrimitive->attributes.find(TEXT("TANGENT")) == InMeshPrimitive->attributes.cend()) return true;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->attributes[TEXT("TANGENT")])];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexTangents);
}

bool FglTFImporter::GetVertexTexcoords(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<FVector2D> OutVertexTexcoords[MAX_STATIC_TEXCOORDS])
{
    if (!InGlTF || !InMeshPrimitive) return false;

    wchar_t texcoord_str[NAME_SIZE];
    for (int32 i = 0; i < MAX_STATIC_TEXCOORDS; ++i)
    {
        OutVertexTexcoords[i].Empty();

        std::swprintf(texcoord_str, NAME_SIZE, TEXT("TEXCOORD_%d\0"), i);
        if (InMeshPrimitive->attributes.find(texcoord_str) == InMeshPrimitive->attributes.cend())
        {
            /// the texcoords should be start from 0 and be sequence
            break;
        }

        const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->attributes[texcoord_str])];
        if (GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexTexcoords[i]))
        {
            /// the texcoords should be start from 0 and be sequence
            break;
        }
        return false;
    }
    return true;
}

void FglTFImporter::SwapYZ(FVector& InOutValue)
{
    float Temp = InOutValue.Y;
    InOutValue.Y = InOutValue.Z;
    InOutValue.Z = Temp;
}

void FglTFImporter::SwapYZ(TArray<FVector>& InOutValues)
{
    for (FVector& Value : InOutValues)
    {
        SwapYZ(Value);
    }
}

FString FglTFImporter::SanitizeObjectName(const FString& InObjectName)
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

TextureFilter FglTFImporter::MagFilterToTextureFilter(int32 InValue)
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

TextureFilter FglTFImporter::MinFilterToTextureFilter(int32 InValue)
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

TextureAddress FglTFImporter::WrapSToTextureAddress(int32 InValue)
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

TextureAddress FglTFImporter::WrapTToTextureAddress(int32 InValue)
{
    return WrapSToTextureAddress(InValue);
}

