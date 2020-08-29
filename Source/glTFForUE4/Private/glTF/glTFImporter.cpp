// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImporter.h"

#include "glTF/glTFImporterOptions.h"

#include <Misc/Base64.h>
#include <Misc/SecureHash.h>
#include <Misc/FeedbackContext.h>
#if ENGINE_MINOR_VERSION <= 13
#include <Misc/CoreMisc.h>
#else
#include <Misc/FileHelper.h>
#endif
#include <Misc/Paths.h>

#if defined(ERROR)
#define DRACO_MACRO_TEMP_ERROR      ERROR
#undef ERROR
#endif
#include <draco/compression/decode.h>
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
    Datas.Add(MakeShareable(new FglTFBufferData(InFileFolderRoot, GLTF_GLTFSTRING_TO_TCHAR(InImage->uri.c_str()))));
    return true;
}

bool FglTFBuffers::CacheBuffers(uint32 InIndex, const FString& InFileFolderRoot, const std::shared_ptr<libgltf::SBuffer>& InBuffer)
{
    if (!InBuffer) return false;
    IndexToIndex[EglTFBufferSource::Buffers].Add(InIndex, Datas.Num());
    Datas.Add(MakeShareable(new FglTFBufferData(InFileFolderRoot, GLTF_GLTFSTRING_TO_TCHAR(InBuffer->uri.c_str()))));
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
    template<uint32 TexCoordNumber, uint32 JointNumber>
    static void GetAttributes(const libgltf::SKHR_draco_mesh_compressionextension* InExtensionDraco, const std::unique_ptr<draco::Mesh>& InDracoMesh, const draco::PointAttribute*& OutAttributePosition, const draco::PointAttribute*& OutAttributeNormal, const draco::PointAttribute*& OutAttributeColor, const draco::PointAttribute* OutAttributeTexCoords[TexCoordNumber], const draco::PointAttribute* OutAttributeJointIndeies[JointNumber + 1], const draco::PointAttribute* OutAttributeJointWeights[JointNumber + 1])
    {
        OutAttributePosition = nullptr;
        OutAttributeNormal = nullptr;
        OutAttributeColor = nullptr;
        for (uint32 i = 0; i < TexCoordNumber; ++i)
        {
            OutAttributeTexCoords[i] = nullptr;
        }
        for (uint32 i = 0; i < JointNumber; ++i)
        {
            OutAttributeJointIndeies[i] = nullptr;
            OutAttributeJointWeights[i] = nullptr;
        }
        if (!InExtensionDraco || !InDracoMesh) return;

        typedef std::map<GLTFString, std::shared_ptr<libgltf::SGlTFId>> TAttributes;

        {
            const GLTFString extension_attribute = GLTF_TCHAR_TO_GLTFSTRING(TEXT("POSITION"));
            TAttributes::const_iterator AttributeCIt = InExtensionDraco->attributes.find(extension_attribute);
            if (AttributeCIt != InExtensionDraco->attributes.cend())
            {
                int32_t UniqueId = *(AttributeCIt->second);
                if (UniqueId >= 0)
                {
                    OutAttributePosition = InDracoMesh->GetAttributeByUniqueId(static_cast<uint32_t>(UniqueId));
                }
            }
            else
            {
                OutAttributePosition = InDracoMesh->GetNamedAttribute(draco::GeometryAttribute::POSITION);
            }
        }

        {
            const GLTFString extension_attribute = GLTF_TCHAR_TO_GLTFSTRING(TEXT("NORMAL"));
            TAttributes::const_iterator AttributeCIt = InExtensionDraco->attributes.find(extension_attribute);
            if (AttributeCIt != InExtensionDraco->attributes.cend())
            {
                int32_t UniqueId = *(AttributeCIt->second);
                if (UniqueId >= 0)
                {
                    OutAttributeNormal = InDracoMesh->GetAttributeByUniqueId(static_cast<uint32_t>(UniqueId));
                }
            }
            else
            {
                OutAttributeNormal = InDracoMesh->GetNamedAttribute(draco::GeometryAttribute::NORMAL);
            }
        }

        {
            const GLTFString extension_attribute = GLTF_TCHAR_TO_GLTFSTRING(TEXT("COLOR"));
            TAttributes::const_iterator AttributeCIt = InExtensionDraco->attributes.find(extension_attribute);
            if (AttributeCIt != InExtensionDraco->attributes.cend())
            {
                int32_t UniqueId = *(AttributeCIt->second);
                if (UniqueId >= 0)
                {
                    OutAttributeColor = InDracoMesh->GetAttributeByUniqueId(static_cast<uint32_t>(UniqueId));
                }
            }
            else
            {
                OutAttributeColor = InDracoMesh->GetNamedAttribute(draco::GeometryAttribute::COLOR);
            }
        }

        {
            for (int32 i = 0; i < TexCoordNumber; ++i)
            {
                const FString ExtensionAttribute = FString::Printf(TEXT("TEXCOORD_%d"), i);
                const GLTFString extension_attribute = GLTF_TCHAR_TO_GLTFSTRING(*ExtensionAttribute);

                TAttributes::const_iterator AttributeCIt = InExtensionDraco->attributes.find(extension_attribute);
                if (AttributeCIt != InExtensionDraco->attributes.cend())
                {
                    int32_t UniqueId = *(AttributeCIt->second);
                    if (UniqueId >= 0)
                    {
                        OutAttributeTexCoords[i] = InDracoMesh->GetAttributeByUniqueId(static_cast<uint32_t>(UniqueId));
                    }
                }
            }
            if (OutAttributeTexCoords[0] == nullptr)
            {
                OutAttributeTexCoords[0] = InDracoMesh->GetNamedAttribute(draco::GeometryAttribute::TEX_COORD);
            }
        }

        {
            for (int32 i = 0; i < JointNumber; ++i)
            {
                const FString ExtensionAttribute = FString::Printf(TEXT("JOINTS_%d"), i);
                const GLTFString extension_attribute = GLTF_TCHAR_TO_GLTFSTRING(*ExtensionAttribute);

                TAttributes::const_iterator AttributeCIt = InExtensionDraco->attributes.find(extension_attribute);
                if (AttributeCIt != InExtensionDraco->attributes.cend())
                {
                    int32_t UniqueId = *(AttributeCIt->second);
                    if (UniqueId >= 0)
                    {
                        OutAttributeJointIndeies[i] = InDracoMesh->GetAttributeByUniqueId(static_cast<uint32_t>(UniqueId));
                    }
                }
            }
        }

        {
            for (int32 i = 0; i < JointNumber; ++i)
            {
                const FString ExtensionAttribute = FString::Printf(TEXT("WEIGHTS_%d"), i);
                const GLTFString extension_attribute = GLTF_TCHAR_TO_GLTFSTRING(*ExtensionAttribute);

                TAttributes::const_iterator AttributeCIt = InExtensionDraco->attributes.find(extension_attribute);
                if (AttributeCIt != InExtensionDraco->attributes.cend())
                {
                    int32_t UniqueId = *(AttributeCIt->second);
                    if (UniqueId >= 0)
                    {
                        OutAttributeJointWeights[i] = InDracoMesh->GetAttributeByUniqueId(static_cast<uint32_t>(UniqueId));
                    }
                }
            }
        }
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, bool& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 1);
        if (!InDracoData) return false;
        OutEngineData = static_cast<bool>(*InDracoData);
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, int8& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 1);
        if (!InDracoData) return false;
        OutEngineData = static_cast<int8>(*InDracoData);
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, uint8& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 1);
        if (!InDracoData) return false;
        OutEngineData = static_cast<uint8>(*InDracoData);
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, int16& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 1);
        if (!InDracoData) return false;
        OutEngineData = static_cast<int16>(*InDracoData);
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, uint16& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 1);
        if (!InDracoData) return false;
        OutEngineData = static_cast<uint16>(*InDracoData);
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, int32& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 1);
        if (!InDracoData) return false;
        OutEngineData = static_cast<int32>(*InDracoData);
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, uint32& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 1);
        if (!InDracoData) return false;
        OutEngineData = static_cast<uint32>(*InDracoData);
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, float& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 1);
        if (!InDracoData) return false;
        OutEngineData = static_cast<float>(*InDracoData);
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ, bool bInverseX>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, FVector2D& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 2);
        if (!InDracoData) return false;
        if (ComponentNum > 0) OutEngineData.X = static_cast<float>(*(InDracoData + 0)) * (bInverseX ? -1.0f : 1.0f);
        if (ComponentNum > 1) OutEngineData.Y = static_cast<float>(*(InDracoData + 1));
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ, bool bInverseX>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, FVector& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 3);
        if (!InDracoData) return false;
        if (!bSwapYZ)
        {
            if (ComponentNum > 0) OutEngineData.X = static_cast<float>(*(InDracoData + 0)) * (bInverseX ? -1.0f : 1.0f);
            if (ComponentNum > 1) OutEngineData.Y = static_cast<float>(*(InDracoData + 1));
            if (ComponentNum > 2) OutEngineData.Z = static_cast<float>(*(InDracoData + 2));
        }
        else
        {
            if (ComponentNum > 0) OutEngineData.X = static_cast<float>(*(InDracoData + 0));
            if (ComponentNum > 2) OutEngineData.Y = static_cast<float>(*(InDracoData + 2)); /// swap y and z
            if (ComponentNum > 1) OutEngineData.Z = static_cast<float>(*(InDracoData + 1)); /// swap y and z
        }
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ, bool bInverseX>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, FVector4& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 4);
        if (!InDracoData) return false;
        if (!bSwapYZ)
        {
            if (ComponentNum > 0) OutEngineData.X = static_cast<float>(*(InDracoData + 0)) * (bInverseX ? -1.0f : 1.0f);
            if (ComponentNum > 1) OutEngineData.Y = static_cast<float>(*(InDracoData + 1));
            if (ComponentNum > 2) OutEngineData.Z = static_cast<float>(*(InDracoData + 2));
        }
        else
        {
            if (ComponentNum > 0) OutEngineData.X = static_cast<float>(*(InDracoData + 0));
            if (ComponentNum > 2) OutEngineData.Y = static_cast<float>(*(InDracoData + 2)); /// swap y and z
            if (ComponentNum > 1) OutEngineData.Z = static_cast<float>(*(InDracoData + 1)); /// swap y and z
        }
        if (ComponentNum > 3) OutEngineData.W = static_cast<float>(*(InDracoData + 3));
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, bool bSwapYZ, bool bInverseX>
    static bool DracoDataToEngineData(const TDracoData* const InDracoData, FQuat& OutEngineData)
    {
        checkSlow(InDracoData && ComponentNum == 4);
        if (!InDracoData) return false;
        if (!bSwapYZ)
        {
            if (ComponentNum > 0) OutEngineData.X = static_cast<float>(*(InDracoData + 0)) * (bInverseX ? -1.0f : 1.0f);
            if (ComponentNum > 1) OutEngineData.Y = static_cast<float>(*(InDracoData + 1));
            if (ComponentNum > 2) OutEngineData.Z = static_cast<float>(*(InDracoData + 2));
            if (ComponentNum > 3) OutEngineData.W = static_cast<float>(*(InDracoData + 3));
        }
        else
        {
            if (ComponentNum > 0) OutEngineData.X = static_cast<float>(*(InDracoData + 0));
            if (ComponentNum > 2) OutEngineData.Y = static_cast<float>(*(InDracoData + 2)); /// swap y and z
            if (ComponentNum > 1) OutEngineData.Z = static_cast<float>(*(InDracoData + 1)); /// swap y and z
            if (ComponentNum > 3) OutEngineData.W = static_cast<float>(*(InDracoData + 3)) * -1.0f;
        }
        return true;
    }

    template<typename TDracoData, uint32 ComponentNum, typename TEngineData, bool bSwapYZ, bool bInverseX>
    static bool GetDatas(const draco::PointAttribute* const InPointAttribute, TArray<TEngineData>& OutDatas)
    {
        OutDatas.Empty();
        if (!InPointAttribute || InPointAttribute->num_components() <= 0) return false;

        OutDatas.SetNumUninitialized(InPointAttribute->indices_map_size());

        TDracoData DracoData[ComponentNum];
        for (draco::PointIndex i(0); i < InPointAttribute->indices_map_size(); ++i)
        {
            InPointAttribute->GetMappedValue(i, DracoData);
            DracoDataToEngineData<TDracoData, ComponentNum, bSwapYZ, bInverseX>(DracoData, OutDatas[i.value()]);
        }
        return true;
    }

    template<typename TDracoData, typename TEngineData, bool bSwapYZ, bool bInverseX>
    static bool GetDatas(const draco::PointAttribute* const InPointAttribute, TArray<TEngineData>& OutDatas)
    {
        OutDatas.Empty();
        if (!InPointAttribute || InPointAttribute->num_components() <= 0) return false;

        switch (InPointAttribute->num_components())
        {
        case 1:
            return GetDatas<TDracoData, 1, TEngineData, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case 2:
            return GetDatas<TDracoData, 2, TEngineData, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case 3:
            return GetDatas<TDracoData, 3, TEngineData, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case 4:
            return GetDatas<TDracoData, 4, TEngineData, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        default:
            break;
        }
        return false;
    }

    template<typename TEngineDataType, bool bSwapYZ, bool bInverseX>
    static bool GetDatas(const draco::PointAttribute* const InPointAttribute, TArray<TEngineDataType>& OutDatas)
    {
        if (!InPointAttribute || InPointAttribute->indices_map_size() <= 0) return false;
        OutDatas.Empty();

        uint32 DataMemorySize = 0;
        draco::DataType AttributeDataType = InPointAttribute->data_type();
        switch (AttributeDataType)
        {
        case draco::DT_BOOL:
            return GetDatas<bool, TEngineDataType, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case draco::DT_INT8:
            return GetDatas<int8, TEngineDataType, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case draco::DT_UINT8:
            return GetDatas<uint8, TEngineDataType, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case draco::DT_INT16:
            return GetDatas<int16, TEngineDataType, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case draco::DT_UINT16:
            return GetDatas<uint16, TEngineDataType, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case draco::DT_INT32:
            return GetDatas<int32, TEngineDataType, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case draco::DT_UINT32:
            return GetDatas<uint32, TEngineDataType, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case draco::DT_FLOAT32:
            return GetDatas<float, TEngineDataType, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case draco::DT_INT64:
            return GetDatas<int64, TEngineDataType, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case draco::DT_UINT64:
            return GetDatas<uint64, TEngineDataType, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        case draco::DT_FLOAT64:
            return GetDatas<double, TEngineDataType, bSwapYZ, bInverseX>(InPointAttribute, OutDatas);

        default:
            break;
        }
        return false;
    }

    template<uint32 TexCoordNumber, uint32 JointNumber, bool bSwapYZ, bool bInverseX>
    static bool Decode(const FglTFBuffers& InBuffers, const libgltf::SKHR_draco_mesh_compressionextension* InExtensionDraco, const TArray<uint8>& InEncodedBuffer, TArray<uint32>& OutTriangleIndices, TArray<FVector>& OutVertexPositions, TArray<FVector>& OutVertexNormals, TArray<FVector4>& OutVertexTangents, TArray<FVector2D> OutVertexTexcoords[TexCoordNumber], TArray<FMatrix>& OutInverseBindMatrices, TArray<FVector4> OutJointIndeies[JointNumber + 1], TArray<FVector4> OutJointWeights[JointNumber + 1])
    {
        if (!InExtensionDraco) return false;

        draco::DecoderBuffer DracoDecoderBuffer;
        DracoDecoderBuffer.Init((const char*)InEncodedBuffer.GetData(), InEncodedBuffer.GetAllocatedSize());
        auto StatusOrGeometryType = draco::Decoder::GetEncodedGeometryType(&DracoDecoderBuffer);
        if (!StatusOrGeometryType.ok()) return false;
        if (StatusOrGeometryType.value() != draco::TRIANGULAR_MESH) return false;
        draco::Decoder DracoDecoder;
        auto DracoStatusOrMesh = DracoDecoder.DecodeMeshFromBuffer(&DracoDecoderBuffer);
        if (!DracoStatusOrMesh.ok()) return false;
        std::unique_ptr<draco::Mesh> DracoMesh = std::move(DracoStatusOrMesh).value();

        const draco::PointAttribute* DracoAttributePosition = nullptr;
        const draco::PointAttribute* DracoAttributeNormal = nullptr;
        const draco::PointAttribute* DracoAttributeColor = nullptr;
        const draco::PointAttribute* DracoAttributeTexCoords[TexCoordNumber] = { nullptr };
        const draco::PointAttribute* DracoAttributeJointIndeies[JointNumber + 1] = { nullptr };
        const draco::PointAttribute* DracoAttributeJointWeights[JointNumber + 1] = { nullptr };
        GetAttributes<TexCoordNumber, JointNumber>(InExtensionDraco, DracoMesh, DracoAttributePosition, DracoAttributeNormal, DracoAttributeColor, DracoAttributeTexCoords, DracoAttributeJointIndeies, DracoAttributeJointWeights);

        if (!DracoAttributePosition) return false;

        uint32 VertexNumber = DracoAttributePosition->size();

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

        if (!GetDatas<FVector, bSwapYZ, bInverseX>(DracoAttributePosition, OutVertexPositions)) return false;
        GetDatas<FVector, bSwapYZ, bInverseX>(DracoAttributeNormal, OutVertexNormals);
        for (uint32 i = 0; i < TexCoordNumber; ++i)
        {
            const draco::PointAttribute* DracoAttributeTexCoord = DracoAttributeTexCoords[i];
            GetDatas<FVector2D, bSwapYZ, bInverseX>(DracoAttributeTexCoord, OutVertexTexcoords[i]);
        }

        for (uint32 i = 0; i < JointNumber; ++i)
        {
            GetDatas<FVector4, bSwapYZ, bInverseX>(DracoAttributeJointIndeies[i], OutJointIndeies[i]);
            GetDatas<FVector4, bSwapYZ, bInverseX>(DracoAttributeJointWeights[i], OutJointWeights[i]);
        }
        return true;
    }
};

FglTFAnimationSequenceKeyData::FglTFAnimationSequenceKeyData()
    : Time(0.0f)
    , Transform(FTransform::Identity)
    , TranslationInterpolation(RCIM_Linear)
    , RotationInterpolation(RCIM_Linear)
    , ScaleInterpolation(RCIM_Linear)
{
    //
}

FglTFAnimationSequenceData::FglTFAnimationSequenceData()
    : NodeIndex(INDEX_NONE)
    , KeyDatas()
{
    //
}

FglTFAnimationSequenceKeyData* FglTFAnimationSequenceData::FindOrAddSequenceKeyData(float InTime)
{
    FglTFAnimationSequenceKeyData* KeyDataPtr = KeyDatas.FindByPredicate([InTime](const FglTFAnimationSequenceKeyData& InKeyData) {
        return (InKeyData.Time == InTime);
    });
    if (!KeyDataPtr)
    {
        FglTFAnimationSequenceKeyData KeyData;
        KeyData.Time = InTime;
        KeyDatas.Add(KeyData);
        KeyDataPtr = &(KeyDatas.Last());
    }
    return KeyDataPtr;
}

void FglTFAnimationSequenceData::FindOrAddSequenceKeyDataAndSetTranslation(float InTime, const FVector& InValue, ERichCurveInterpMode InInterpolation)
{
    FglTFAnimationSequenceKeyData* KeyDataPtr = FindOrAddSequenceKeyData(InTime);
    checkSlow(KeyDataPtr);
    if (!KeyDataPtr) return;

    FVector NewValue = KeyDataPtr->Transform.GetTranslation() + InValue;
    KeyDataPtr->Transform.SetTranslation(NewValue);
    KeyDataPtr->TranslationInterpolation = InInterpolation;
}

void FglTFAnimationSequenceData::FindOrAddSequenceKeyDataAndSetRotation(float InTime, const FQuat& InValue, ERichCurveInterpMode InInterpolation)
{
    FglTFAnimationSequenceKeyData* KeyDataPtr = FindOrAddSequenceKeyData(InTime);
    checkSlow(KeyDataPtr);
    if (!KeyDataPtr) return;

    FQuat NewValue = KeyDataPtr->Transform.GetRotation() * InValue;
    KeyDataPtr->Transform.SetRotation(NewValue);
    KeyDataPtr->RotationInterpolation = InInterpolation;
}

void FglTFAnimationSequenceData::FindOrAddSequenceKeyDataAndSetScale(float InTime, const FVector& InValue, ERichCurveInterpMode InInterpolation)
{
    FglTFAnimationSequenceKeyData* KeyDataPtr = FindOrAddSequenceKeyData(InTime);
    checkSlow(KeyDataPtr);
    if (!KeyDataPtr) return;

    FVector NewValue = KeyDataPtr->Transform.GetScale3D() * InValue;
    KeyDataPtr->Transform.SetScale3D(NewValue);
    KeyDataPtr->ScaleInterpolation = InInterpolation;
}

FglTFAnimationSequenceDatas::FglTFAnimationSequenceDatas()
    : Datas()
{
    //
}

FglTFAnimationSequenceData* FglTFAnimationSequenceDatas::FindOrAddSequenceData(int32 InNodeIndex)
{
    FglTFAnimationSequenceData* SequenceDataPtr = Datas.FindByPredicate([InNodeIndex](const FglTFAnimationSequenceData& InSequenceData) {
        return (InSequenceData.NodeIndex == InNodeIndex);
    });
    if (!SequenceDataPtr)
    {
        FglTFAnimationSequenceData SequenceData;
        SequenceData.NodeIndex = InNodeIndex;
        Datas.Add(SequenceData);
        SequenceDataPtr = &(Datas.Last());
    }
    return SequenceDataPtr;
}

void FglTFAnimationSequenceDatas::FindOrAddSequenceDataAndSetTranslation(int32 InNodeIndex, float InTime, const FVector& InValue, ERichCurveInterpMode InInterpolation)
{
    FglTFAnimationSequenceData* SequenceDataPtr = FindOrAddSequenceData(InNodeIndex);
    checkSlow(SequenceDataPtr);
    if (!SequenceDataPtr) return;
    SequenceDataPtr->FindOrAddSequenceKeyDataAndSetTranslation(InTime, InValue, InInterpolation);
}

void FglTFAnimationSequenceDatas::FindOrAddSequenceDataAndSetRotation(int32 InNodeIndex, float InTime, const FQuat& InValue, ERichCurveInterpMode InInterpolation)
{
    FglTFAnimationSequenceData* SequenceDataPtr = FindOrAddSequenceData(InNodeIndex);
    checkSlow(SequenceDataPtr);
    if (!SequenceDataPtr) return;
    SequenceDataPtr->FindOrAddSequenceKeyDataAndSetRotation(InTime, InValue, InInterpolation);
}

void FglTFAnimationSequenceDatas::FindOrAddSequenceDataAndSetScale(int32 InNodeIndex, float InTime, const FVector& InValue, ERichCurveInterpMode InInterpolation)
{
    FglTFAnimationSequenceData* SequenceDataPtr = FindOrAddSequenceData(InNodeIndex);
    checkSlow(SequenceDataPtr);
    if (!SequenceDataPtr) return;
    SequenceDataPtr->FindOrAddSequenceKeyDataAndSetScale(InTime, InValue, InInterpolation);
}

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

UObject* FglTFImporter::Create(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InglTFBuffers) const
{
    if (!InGlTF)
    {
        UE_LOG(LogglTFForUE4, Error, TEXT("Invalid glTF!"));
        return nullptr;
    }

    if (!InGlTF->asset || InGlTF->asset->version != GLTF_TCHAR_TO_GLTFSTRING(TEXT("2.0")))
    {
        const FString AssetVersion = (InGlTF->asset != nullptr) ? GLTF_GLTFSTRING_TO_TCHAR(InGlTF->asset->version.c_str()) : TEXT("none");
        UE_LOG(LogglTFForUE4, Error, TEXT("Invalid version: %s!"), *AssetVersion);
        return nullptr;
    }

    TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();

    const FString FolderPathInOS = FPaths::GetPath(glTFImporterOptions->FilePathInOS);
    FglTFBuffers glTFBuffers;
    glTFBuffers.Cache(FolderPathInOS, InGlTF);

    //TODO: generate the procedural mesh

    return nullptr;
}

namespace glTFImporter
{
    template<typename TEngineDataType>
    struct TAccessorTypeScale
    {
        TEngineDataType V[1];

        operator TEngineDataType() const
        {
            return V[0];
        }

        operator FVector2D() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            return FVector2D(V[0], 0.0f);
        }

        operator FVector() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            return FVector(V[0], 0.0f, 0.0f);
        }

        operator FVector4() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            return FVector4(V[0], 0.0f, 0.0f, 0.0f);
        }

        operator FQuat() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            return FQuat(V[0], 0.0f, 0.0f, -1.0f);
        }

        operator FMatrix() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            return FMatrix::Identity;
        }
    };

    template<typename TEngineDataType, bool bSwapYZ, bool bInverseX>
    struct TAccessorTypeVec2
    {
        TEngineDataType V[2];

        operator TEngineDataType() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return V[0] * (bInverseX ? -1.0f : 1.0f);
            return V[0];
        }

        operator FVector2D() const
        {
            if (!bSwapYZ) return FVector2D(V[0] * (bInverseX ? -1.0f : 1.0f), V[1]);
            return FVector2D(V[0], V[1]);
        }

        operator FVector() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return FVector(V[0] * (bInverseX ? -1.0f : 1.0f), V[1], 0.0f);
            return FVector(V[0], V[1], 0.0f);
        }

        operator FVector4() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return FVector4(V[0] * (bInverseX ? -1.0f : 1.0f), V[1], 0.0f, 0.0f);
            return FVector4(V[0], V[1], 0.0f, 0.0f);
        }

        operator FQuat() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return FQuat(V[0] * (bInverseX ? -1.0f : 1.0f), V[1], 0.0f, 1.0f);
            return FQuat(V[0], 0.0f, V[1], -1.0f);    /// swap y and z
        }

        operator FMatrix() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            return FMatrix::Identity;
        }
    };

    template<typename TEngineDataType, bool bSwapYZ, bool bInverseX>
    struct TAccessorTypeVec3
    {
        TEngineDataType V[3];

        operator TEngineDataType() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return V[0] * (bInverseX ? -1.0f : 1.0f);
            return V[0];
        }

        operator FVector2D() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return FVector2D(V[0] * (bInverseX ? -1.0f : 1.0f), V[1]);
            return FVector2D(V[0], V[2]);               /// swap y and z
        }

        operator FVector() const
        {
            if (!bSwapYZ) return FVector(V[0] * (bInverseX ? -1.0f : 1.0f), V[1], V[2]);
            return FVector(V[0], V[2], V[1]);           /// swap y and z
        }

        operator FVector4() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return FVector4(V[0] * (bInverseX ? -1.0f : 1.0f), V[1], V[2], 0.0f);
            return FVector4(V[0], V[2], V[1], 0.0f);    /// swap y and z
        }

        operator FQuat() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return FQuat(V[0] * (bInverseX ? -1.0f : 1.0f), V[1], V[2], 1.0f);
            return FQuat(V[0], V[2], V[1], -1.0f);    /// swap y and z
        }

        operator FMatrix() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            return FMatrix::Identity;
        }
    };

    template<typename TEngineDataType, bool bSwapYZ, bool bInverseX>
    struct TAccessorTypeVec4
    {
        TEngineDataType V[4];

        operator TEngineDataType() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return V[0] * (bInverseX ? -1.0f : 1.0f);
            return V[0];
        }

        operator FVector2D() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return FVector2D(V[0] * (bInverseX ? -1.0f : 1.0f), V[1]);
            return FVector2D(V[0], V[2]);               /// swap y and z
        }

        operator FVector() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return FVector(V[0] * (bInverseX ? -1.0f : 1.0f), V[1], V[2]);
            return FVector(V[0], V[2], V[1]);           /// swap y and z
        }

        operator FVector4() const
        {
            if (!bSwapYZ) return FVector4(V[0] * (bInverseX ? -1.0f : 1.0f), V[1], V[2], V[3]);
            return FVector4(V[0], V[2], V[1], V[3]);    /// swap y and z
        }

        operator FQuat() const
        {
            if (!bSwapYZ) return FQuat(V[0] * (bInverseX ? -1.0f : 1.0f), V[1], V[2], V[3]);
            return FQuat(V[0], V[2], V[1], V[3] * -1.0f);    /// swap y and z
        }

        operator FMatrix() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            return FMatrix::Identity;
        }
    };

    template<typename TEngineDataType, bool bSwapYZ, bool bInverseX>
    struct TAccessorTypeMat4x4
    {
        TEngineDataType V[4][4];

        operator TEngineDataType() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return V[0][0] * (bInverseX ? -1.0f : 1.0f);
            return V[0][0];
        }

        operator FVector2D() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ)return FVector2D(V[0][0] * (bInverseX ? -1.0f : 1.0f), V[0][1]);
            return FVector2D(V[0][0], V[0][2]);                     /// swap y and z
        }

        operator FVector() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return FVector(V[0][0] * (bInverseX ? -1.0f : 1.0f), V[0][1], V[0][2]);
            return FVector(V[0][0], V[0][2], V[0][1]);              /// swap y and z
        }

        operator FVector4() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return FVector4(V[0][0] * (bInverseX ? -1.0f : 1.0f), V[0][1], V[0][2], V[0][3]);
            return FVector4(V[0][0], V[0][2], V[0][1], V[0][3]);     /// swap y and z
        }

        operator FQuat() const
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Can't convert the accessor's data to the engine's data?"));
            if (!bSwapYZ) return FQuat(V[0][0] * (bInverseX ? -1.0f : 1.0f), V[0][1], V[0][2], V[0][3]);
            return FQuat(V[0][0], V[0][2], V[0][1], V[0][3] * -1.0f);    /// swap y and z
        }

        operator FMatrix() const
        {
            /// swap y and z
            FMatrix Matrix = FMatrix::Identity;
            for (uint8 j = 0; j < 4; ++j)
            {
                for (uint8 i = 0; i < 4; ++i)
                {
                    Matrix.M[j][i] = static_cast<TEngineDataType>(V[j][i]);
                }
            }
            FglTFImporter::ConvertToUnrealSpace(Matrix, bSwapYZ, bInverseX);
            return Matrix;
        }
    };

    template<typename TAccessorDataType, typename TEngineDataType, bool bSwapYZ, bool bInverseX>
    bool GetAccessorData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InBuffers, const std::shared_ptr<libgltf::SAccessor>& InAccessor, TArray<TEngineDataType>& OutDataArray)
    {
        if (!InAccessor || !InAccessor->bufferView)
        {
            UE_LOG(LogglTFForUE4, Error, TEXT("Invalid accessor!"));
            return false;
        }

        int32 BufferViewIndex = (*InAccessor->bufferView);

        OutDataArray.Empty();

        FString FilePath;
        if (InAccessor->type == GLTF_TCHAR_TO_GLTFSTRING(TEXT("SCALAR")))
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
        else if (InAccessor->type == GLTF_TCHAR_TO_GLTFSTRING(TEXT("VEC2")))
        {
            TArray<TAccessorTypeVec2<TAccessorDataType, bSwapYZ, bInverseX>> AccessorDataArray;
            if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, AccessorDataArray, FilePath, InAccessor->byteOffset, InAccessor->count))
            {
                for (const TAccessorTypeVec2<TAccessorDataType, bSwapYZ, bInverseX>& AccessorDataItem : AccessorDataArray)
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
        else if (InAccessor->type == GLTF_TCHAR_TO_GLTFSTRING(TEXT("VEC3")))
        {
            TArray<TAccessorTypeVec3<TAccessorDataType, bSwapYZ, bInverseX>> AccessorDataArray;
            if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, AccessorDataArray, FilePath, InAccessor->byteOffset, InAccessor->count))
            {
                for (const TAccessorTypeVec3<TAccessorDataType, bSwapYZ, bInverseX>& AccessorDataItem : AccessorDataArray)
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
        else if (InAccessor->type == GLTF_TCHAR_TO_GLTFSTRING(TEXT("VEC4")))
        {
            TArray<TAccessorTypeVec4<TAccessorDataType, bSwapYZ, bInverseX>> AccessorDataArray;
            if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, AccessorDataArray, FilePath, InAccessor->byteOffset, InAccessor->count))
            {
                for (const TAccessorTypeVec4<TAccessorDataType, bSwapYZ, bInverseX>& AccessorDataItem : AccessorDataArray)
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
        else if (InAccessor->type == GLTF_TCHAR_TO_GLTFSTRING(TEXT("MAT4")))
        {
            TArray<TAccessorTypeMat4x4<TAccessorDataType, bSwapYZ, bInverseX>> AccessorDataArray;
            if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, AccessorDataArray, FilePath, InAccessor->byteOffset, InAccessor->count))
            {
                for (const TAccessorTypeMat4x4<TAccessorDataType, bSwapYZ, bInverseX>& AccessorDataItem : AccessorDataArray)
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
            const FString AccessorType = WCHAR_TO_TCHAR(InAccessor->type.c_str());
            UE_LOG(LogglTFForUE4, Error, TEXT("Not supports the accessor's type(%s)!"), *AccessorType);
            return false;
        }
        return true;
    }

    template<typename TEngineDataType, bool bSwapYZ, bool bInverseX>
    bool GetAccessorData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InBuffers, const std::shared_ptr<libgltf::SAccessor>& InAccessor, TArray<TEngineDataType>& OutDataArray)
    {
        if (!InAccessor) return false;

        switch (InAccessor->componentType)
        {
        case 5120:
            return GetAccessorData<int8, TEngineDataType, bSwapYZ, bInverseX>(InGlTF, InBuffers, InAccessor, OutDataArray);

        case 5121:
            return GetAccessorData<uint8, TEngineDataType, bSwapYZ, bInverseX>(InGlTF, InBuffers, InAccessor, OutDataArray);

        case 5122:
            return GetAccessorData<int16, TEngineDataType, bSwapYZ, bInverseX>(InGlTF, InBuffers, InAccessor, OutDataArray);

        case 5123:
            return GetAccessorData<uint16, TEngineDataType, bSwapYZ, bInverseX>(InGlTF, InBuffers, InAccessor, OutDataArray);

        case 5125:
            return GetAccessorData<int32, TEngineDataType, bSwapYZ, bInverseX>(InGlTF, InBuffers, InAccessor, OutDataArray);

        case 5126:
            return GetAccessorData<float, TEngineDataType, bSwapYZ, bInverseX>(InGlTF, InBuffers, InAccessor, OutDataArray);

        default:
            UE_LOG(LogglTFForUE4, Error, TEXT("Not support the accessor's componetType(%d)?"), InAccessor->componentType);
            break;
        }
        return false;
    }

    template<bool bSwapYZ>
    bool GetTriangleIndices(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<uint32>& OutTriangleIndices)
    {
        if (!InGlTF || !InMeshPrimitive) return false;

        const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->indices)];
        return GetAccessorData<uint32, bSwapYZ, false>(InGlTF, InBufferFiles, Accessor, OutTriangleIndices);
    }

    template<bool bSwapYZ, bool bInverseX>
    bool GetVertexPositions(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<FVector>& OutVertexPositions)
    {
        if (!InGlTF || !InMeshPrimitive) return false;
        const GLTFString get_key = GLTF_TCHAR_TO_GLTFSTRING(TEXT("POSITION"));
        if (InMeshPrimitive->attributes.find(get_key) == InMeshPrimitive->attributes.cend()) return true;

        const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->attributes[get_key])];
        return GetAccessorData<FVector, bSwapYZ, bInverseX>(InGlTF, InBufferFiles, Accessor, OutVertexPositions);
    }

    template<bool bSwapYZ, bool bInverseX>
    bool GetVertexNormals(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<FVector>& OutVertexNormals)
    {
        if (!InGlTF || !InMeshPrimitive) return false;
        const GLTFString get_key = GLTF_TCHAR_TO_GLTFSTRING(TEXT("NORMAL"));
        if (InMeshPrimitive->attributes.find(get_key) == InMeshPrimitive->attributes.cend()) return true;

        const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->attributes[get_key])];
        return GetAccessorData<FVector, bSwapYZ, bInverseX>(InGlTF, InBufferFiles, Accessor, OutVertexNormals);
    }

    template<bool bSwapYZ, bool bInverseX>
    bool GetVertexTangents(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<FVector4>& OutVertexTangents)
    {
        if (!InGlTF || !InMeshPrimitive) return false;
        const GLTFString get_key = GLTF_TCHAR_TO_GLTFSTRING(TEXT("TANGENT"));
        if (InMeshPrimitive->attributes.find(get_key) == InMeshPrimitive->attributes.cend()) return true;

        const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->attributes[get_key])];
        return GetAccessorData<FVector4, bSwapYZ, bInverseX>(InGlTF, InBufferFiles, Accessor, OutVertexTangents);
    }

    template<int32 TexCoordNumber>
    bool GetVertexTexcoords(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<FVector2D> OutVertexTexcoords[TexCoordNumber])
    {
        if (!InGlTF || !InMeshPrimitive) return false;

        for (int32 i = 0; i < TexCoordNumber; ++i)
        {
            OutVertexTexcoords[i].Empty();

            const FString PrimitiveAttribute = FString::Printf(TEXT("TEXCOORD_%d"), i);
            const GLTFString primitive_attribute = GLTF_TCHAR_TO_GLTFSTRING(*PrimitiveAttribute);
            if (InMeshPrimitive->attributes.find(primitive_attribute) == InMeshPrimitive->attributes.cend())
            {
                continue;
            }

            const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->attributes[primitive_attribute])];
            if (GetAccessorData<FVector2D, false, false>(InGlTF, InBufferFiles, Accessor, OutVertexTexcoords[i]))
            {
                continue;
            }
            return false;
        }
        return true;
    }

    template<bool bSwapYZ, bool bInverseX>
    bool GetInverseBindMatrices(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SSkin>& InSkin, const FglTFBuffers& InBuffers, TArray<FMatrix>& OutInverseBindMatrices)
    {
        if (!InGlTF || !InSkin || !InSkin->inverseBindMatrices) return false;

        const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InSkin->inverseBindMatrices)];
        return GetAccessorData<FMatrix, bSwapYZ, bInverseX>(InGlTF, InBuffers, Accessor, OutInverseBindMatrices);
    }

    bool GetJointIndeies(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, int32 InIndex, const FglTFBuffers& InBufferFiles, TArray<FVector4>& OutJointIndeies)
    {
        if (!InGlTF || !InMeshPrimitive) return false;
        const GLTFString JointName = GLTF_TCHAR_TO_GLTFSTRING(*FString::Printf(TEXT("JOINTS_%d"), InIndex));
        if (InMeshPrimitive->attributes.find(JointName) == InMeshPrimitive->attributes.cend()) return true;

        const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->attributes[JointName])];
        return GetAccessorData<FVector4, false, false>(InGlTF, InBufferFiles, Accessor, OutJointIndeies);
    }

    bool GetJointWeights(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, int32 InIndex, const FglTFBuffers& InBufferFiles, TArray<FVector4>& OutJointWeights)
    {
        if (!InGlTF || !InMeshPrimitive) return false;
        const GLTFString JointName = GLTF_TCHAR_TO_GLTFSTRING(*FString::Printf(TEXT("WEIGHTS_%d"), InIndex));
        if (InMeshPrimitive->attributes.find(JointName) == InMeshPrimitive->attributes.cend()) return true;

        const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[(int32)*(InMeshPrimitive->attributes[JointName])];
        return GetAccessorData<FVector4, false, false>(InGlTF, InBufferFiles, Accessor, OutJointWeights);
    }

    template<int32 TexCoordNumber, int32 JointNumber, bool bSwapYZ, bool bInverseX>
    bool GetMeshData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const std::shared_ptr<libgltf::SSkin>& InSkin, const FglTFBuffers& InBufferFiles, TArray<uint32>& OutTriangleIndices, TArray<FVector>& OutVertexPositions, TArray<FVector>& OutVertexNormals, TArray<FVector4>& OutVertexTangents, TArray<FVector2D> OutVertexTexcoords[TexCoordNumber], TArray<FMatrix>& OutInverseBindMatrices, TArray<FVector4> OutJointIndeies[JointNumber + 1], TArray<FVector4> OutJointWeights[JointNumber + 1])
    {
        OutTriangleIndices.Empty();
        OutVertexPositions.Empty();
        OutVertexNormals.Empty();
        OutVertexTangents.Empty();
        for (uint32 i = 0; i < TexCoordNumber; ++i)
        {
            OutVertexTexcoords[i].Empty();
        }
        OutInverseBindMatrices.Empty();
        for (uint32 i = 0; i < JointNumber; ++i)
        {
            OutJointIndeies[i].Empty();
            OutJointWeights[i].Empty();
        }

        if (!InGlTF || !InMeshPrimitive) return false;

        const libgltf::SKHR_draco_mesh_compressionextension* ExtensionDraco = nullptr;
        {
            const std::shared_ptr<libgltf::SExtension>& Extensions = InMeshPrimitive->extensions;
            const GLTFString extension_property = GLTF_TCHAR_TO_GLTFSTRING(TEXT("KHR_draco_mesh_compression"));
            if (!!Extensions && (Extensions->properties.find(extension_property) != Extensions->properties.end()))
            {
                ExtensionDraco = (const libgltf::SKHR_draco_mesh_compressionextension*)Extensions->properties[extension_property].get();
            }
        }

        if (ExtensionDraco)
        {
            int32 BufferViewIndex = *(ExtensionDraco->bufferView);
            TArray<uint8> EncodeBuffer;
            FString BufferFilePath;
            if (!InBufferFiles.GetBufferViewData(InGlTF, BufferViewIndex, EncodeBuffer, BufferFilePath)
                || !FglTFBufferDecoder::Decode<TexCoordNumber, JointNumber, bSwapYZ, bInverseX>(InBufferFiles, ExtensionDraco, EncodeBuffer, OutTriangleIndices, OutVertexPositions, OutVertexNormals, OutVertexTangents, OutVertexTexcoords, OutInverseBindMatrices, OutJointIndeies, OutJointWeights))
            {
                return false;
            }
        }
        else
        {
            if (!GetTriangleIndices<bSwapYZ>(InGlTF, InMeshPrimitive, InBufferFiles, OutTriangleIndices))
            {
                return false;
            }

            if (!GetVertexPositions<bSwapYZ, bInverseX>(InGlTF, InMeshPrimitive, InBufferFiles, OutVertexPositions))
            {
                return false;
            }

            if (!GetVertexNormals<bSwapYZ, bInverseX>(InGlTF, InMeshPrimitive, InBufferFiles, OutVertexNormals))
            {
                return false;
            }

            if (!GetVertexTangents<bSwapYZ, bInverseX>(InGlTF, InMeshPrimitive, InBufferFiles, OutVertexTangents))
            {
                return false;
            }

            if (!GetVertexTexcoords<TexCoordNumber>(InGlTF, InMeshPrimitive, InBufferFiles, OutVertexTexcoords))
            {
                return false;
            }

            for (int32 i = 0; i < JointNumber; ++i)
            {
                GetJointIndeies(InGlTF, InMeshPrimitive, i, InBufferFiles, OutJointIndeies[i]);
            }

            for (int32 i = 0; i < JointNumber; ++i)
            {
                GetJointWeights(InGlTF, InMeshPrimitive, i, InBufferFiles, OutJointWeights[i]);
            }
        }

        GetInverseBindMatrices<bSwapYZ, bInverseX>(InGlTF, InSkin, InBufferFiles, OutInverseBindMatrices);
        return true;
    }
}

bool FglTFImporter::GetStaticMeshData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const FglTFBuffers& InBufferFiles, TArray<uint32>& OutTriangleIndices, TArray<FVector>& OutVertexPositions, TArray<FVector>& OutVertexNormals, TArray<FVector4>& OutVertexTangents, TArray<FVector2D> OutVertexTexcoords[MAX_STATIC_TEXCOORDS], bool bSwapYZ /*= true*/)
{
    std::shared_ptr<libgltf::SSkin> Skin = nullptr;
    TArray<FMatrix> InverseBindMatrices;
    TArray<FVector4> JointIndeies[1];
    TArray<FVector4> JointWeights[1];
    return bSwapYZ
        ? glTFImporter::GetMeshData<MAX_TEXCOORDS, 0, true, false>(InGlTF, InMeshPrimitive, Skin, InBufferFiles, OutTriangleIndices, OutVertexPositions, OutVertexNormals, OutVertexTangents, OutVertexTexcoords, InverseBindMatrices, JointIndeies, JointWeights)
        : glTFImporter::GetMeshData<MAX_TEXCOORDS, 0, false, true>(InGlTF, InMeshPrimitive, Skin, InBufferFiles, OutTriangleIndices, OutVertexPositions, OutVertexNormals, OutVertexTangents, OutVertexTexcoords, InverseBindMatrices, JointIndeies, JointWeights);
}

bool FglTFImporter::GetSkeletalMeshData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SMeshPrimitive>& InMeshPrimitive, const std::shared_ptr<libgltf::SSkin>& InSkin, const FglTFBuffers& InBufferFiles, TArray<uint32>& OutTriangleIndices, TArray<FVector>& OutVertexPositions, TArray<FVector>& OutVertexNormals, TArray<FVector4>& OutVertexTangents, TArray<FVector2D> OutVertexTexcoords[MAX_TEXCOORDS], TArray<FMatrix>& OutInverseBindMatrices, TArray<FVector4> OutJointIndeies[GLTF_JOINT_LAYERS_NUM_MAX], TArray<FVector4> OutJointWeights[GLTF_JOINT_LAYERS_NUM_MAX], bool bSwapYZ /*= true*/)
{
    return bSwapYZ
        ? glTFImporter::GetMeshData<MAX_TEXCOORDS, GLTF_JOINT_LAYERS_NUM_MAX, true, false>(InGlTF, InMeshPrimitive, InSkin, InBufferFiles, OutTriangleIndices, OutVertexPositions, OutVertexNormals, OutVertexTangents, OutVertexTexcoords, OutInverseBindMatrices, OutJointIndeies, OutJointWeights)
        : glTFImporter::GetMeshData<MAX_TEXCOORDS, GLTF_JOINT_LAYERS_NUM_MAX, false, true>(InGlTF, InMeshPrimitive, InSkin, InBufferFiles, OutTriangleIndices, OutVertexPositions, OutVertexNormals, OutVertexTangents, OutVertexTexcoords, OutInverseBindMatrices, OutJointIndeies, OutJointWeights);
}

bool FglTFImporter::GetAnimationSequenceData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const std::shared_ptr<libgltf::SAnimation>& InglTFAnimation
    , const FglTFBuffers& InBufferFiles
    , FglTFAnimationSequenceDatas& OutAnimationSequenceDatas, bool bSwapYZ /*= true*/)
{
    if (!InGlTF || !InglTFAnimation) return false;

    OutAnimationSequenceDatas.Datas.Empty();

    for (const std::shared_ptr<libgltf::SAnimationChannel>& glTFAnimationChannelPtr : InglTFAnimation->channels)
    {
        if (!glTFAnimationChannelPtr) continue;

        const std::shared_ptr<libgltf::SGlTFId>& glTFAnimationChannelSamplerIndexPtr = glTFAnimationChannelPtr->sampler;
        if (!glTFAnimationChannelSamplerIndexPtr) continue;
        int32 SamplerIndex = *glTFAnimationChannelSamplerIndexPtr;
        if (SamplerIndex < 0 || SamplerIndex >= static_cast<int32>(InglTFAnimation->samplers.size())) continue;

        const std::shared_ptr<libgltf::SAnimationSampler>& glTFAnimationSamplerPtr = InglTFAnimation->samplers[SamplerIndex];
        if (!glTFAnimationSamplerPtr) continue;

        const std::shared_ptr<libgltf::SAnimationChannelTarget>& glTFAnimationChannelTargetPtr = glTFAnimationChannelPtr->target;
        if (!glTFAnimationChannelTargetPtr) continue;

        const std::shared_ptr<libgltf::SGlTFId>& glTFAnimationChannelTargetNodeIndexPtr = glTFAnimationChannelTargetPtr->node;
        if (!glTFAnimationChannelTargetNodeIndexPtr) continue;
        int32 NodeIndex = *glTFAnimationChannelTargetNodeIndexPtr;

        const std::shared_ptr<libgltf::SGlTFId>& glTFAnimationSamplerInputAccessorIndexPtr = glTFAnimationSamplerPtr->input;
        if (!glTFAnimationSamplerInputAccessorIndexPtr) continue;
        int32 InputAccessorIndex = *glTFAnimationSamplerInputAccessorIndexPtr;
        if (InputAccessorIndex < 0 || InputAccessorIndex >= static_cast<int32>(InGlTF->accessors.size())) continue;
        const std::shared_ptr<libgltf::SAccessor>& glTFInputAccessorPtr = InGlTF->accessors[InputAccessorIndex];
        if (!glTFInputAccessorPtr) continue;

        const std::shared_ptr<libgltf::SGlTFId>& glTFAnimationSamplerOutputAccessorIndexPtr = glTFAnimationSamplerPtr->output;
        if (!glTFAnimationSamplerOutputAccessorIndexPtr) continue;
        int32 OutputAccessorIndex = *glTFAnimationSamplerOutputAccessorIndexPtr;
        if (OutputAccessorIndex < 0 || OutputAccessorIndex >= static_cast<int32>(InGlTF->accessors.size())) continue;
        const std::shared_ptr<libgltf::SAccessor>& glTFOutputAccessorPtr = InGlTF->accessors[OutputAccessorIndex];
        if (!glTFOutputAccessorPtr) continue;

        TArray<float> Times;
        if (!glTFImporter::GetAccessorData<float, false, false>(InGlTF, InBufferFiles, glTFInputAccessorPtr, Times)) continue;

        const FString glTFAnimationSamplerInterpolation = GLTF_GLTFSTRING_TO_TCHAR(glTFAnimationSamplerPtr->interpolation.c_str());
        ERichCurveInterpMode Interpolation = StringToRichCurveInterpMode(glTFAnimationSamplerInterpolation);

        TArray<FVector> Translations;
        TArray<FQuat> Rotations;
        TArray<FVector> Scales;
        const FString glTFAnimationChannelTargetPath = GLTF_GLTFSTRING_TO_TCHAR(glTFAnimationChannelTargetPtr->path.c_str());
        if (glTFAnimationChannelTargetPath.Equals(TEXT("translation"), ESearchCase::IgnoreCase))
        {
            if (bSwapYZ)
            {
                if (!glTFImporter::GetAccessorData<FVector, true, false>(InGlTF, InBufferFiles, glTFOutputAccessorPtr, Translations)) continue;
            }
            else
            {
                if (!glTFImporter::GetAccessorData<FVector, false, false>(InGlTF, InBufferFiles, glTFOutputAccessorPtr, Translations)) continue;
            }
        }
        else if (glTFAnimationChannelTargetPath.Equals(TEXT("rotation"), ESearchCase::IgnoreCase))
        {
            if (bSwapYZ)
            {
                if (!glTFImporter::GetAccessorData<FQuat, true, false>(InGlTF, InBufferFiles, glTFOutputAccessorPtr, Rotations)) continue;
            }
            else
            {
                if (!glTFImporter::GetAccessorData<FQuat, false, false>(InGlTF, InBufferFiles, glTFOutputAccessorPtr, Rotations)) continue;
            }
        }
        else if (glTFAnimationChannelTargetPath.Equals(TEXT("scale"), ESearchCase::IgnoreCase))
        {
            if (bSwapYZ)
            {
                if (!glTFImporter::GetAccessorData<FVector, true, false>(InGlTF, InBufferFiles, glTFOutputAccessorPtr, Scales)) continue;
            }
            else
            {
                if (!glTFImporter::GetAccessorData<FVector, false, false>(InGlTF, InBufferFiles, glTFOutputAccessorPtr, Scales)) continue;
            }
        }

        if (Translations.Num() == Times.Num())
        {
            for (int32 i = 0; i < Translations.Num(); ++i)
            {
                OutAnimationSequenceDatas.FindOrAddSequenceDataAndSetTranslation(NodeIndex, Times[i], Translations[i], Interpolation);
            }
        }
        else if (Rotations.Num() == Times.Num())
        {
            for (int32 i = 0; i < Rotations.Num(); ++i)
            {
                OutAnimationSequenceDatas.FindOrAddSequenceDataAndSetRotation(NodeIndex, Times[i], Rotations[i], Interpolation);
            }
        }
        else if (Scales.Num() == Times.Num())
        {
            for (int32 i = 0; i < Scales.Num(); ++i)
            {
                OutAnimationSequenceDatas.FindOrAddSequenceDataAndSetScale(NodeIndex, Times[i], Scales[i], Interpolation);
            }
        }
    }

    return (OutAnimationSequenceDatas.Datas.Num() > 0);
}

bool FglTFImporter::GetNodeParentIndices(const std::shared_ptr<libgltf::SGlTF>& InGlTF, TArray<int32>& OutParentIndices)
{
    if (InGlTF->nodes.empty()) return false;
    OutParentIndices.SetNumUninitialized(static_cast<int32>(InGlTF->nodes.size()));

    for (int32& ParentIndex : OutParentIndices)
    {
        ParentIndex = INDEX_NONE;
    }

    for (int32 i = 0; i < static_cast<int32>(InGlTF->nodes.size()); ++i)
    {
        const std::shared_ptr<libgltf::SNode>& NodePtr = InGlTF->nodes[i];
        if (!NodePtr) return false;

        for (const std::shared_ptr<libgltf::SGlTFId>& ChildNodeIdPtr : NodePtr->children)
        {
            if (!ChildNodeIdPtr) return false;
            int32 ChildNodeId = *ChildNodeIdPtr;
            checkSlow(ChildNodeId >= 0 && ChildNodeId < OutParentIndices.Num());
            if (ChildNodeId < 0 || ChildNodeId >= OutParentIndices.Num()) return false;
            OutParentIndices[ChildNodeId] = i;
        }
    }
    return true;
}

bool FglTFImporter::GetNodeRelativeTransforms(const std::shared_ptr<libgltf::SGlTF>& InGlTF, TArray<FTransform>& OutRelativeTransforms, bool bSwapYZ /*= true*/)
{
    if (InGlTF->nodes.empty()) return false;

    OutRelativeTransforms.SetNumUninitialized(static_cast<int32>(InGlTF->nodes.size()));

    FMatrix Matrix;
    for (int32 i = 0; i < static_cast<int32>(InGlTF->nodes.size()); ++i)
    {
        const std::shared_ptr<libgltf::SNode>& NodePtr = InGlTF->nodes[i];
        if (!NodePtr) return false;

        FTransform& Transform = OutRelativeTransforms[i];
        Transform.SetIdentity();

        if (NodePtr->matrix.size() == 16)
        {
            Matrix.SetIdentity();

            for (uint32 j = 0; j < 4; ++j)
            {
                for (uint32 k = 0; k < 4; ++k)
                {
                    uint8 Index = k + j * 4;
                    Matrix.M[j][k] = NodePtr->matrix[Index];
                }
            }

            FglTFImporter::ConvertToUnrealSpace(Matrix, bSwapYZ, !bSwapYZ);
            Transform.SetFromMatrix(Matrix);
        }

        /// check the TRS
        FVector Translation(FVector::ZeroVector);
        if (NodePtr->translation.size() == 3)
        {
            Translation.X = NodePtr->translation[0];
            Translation.Y = NodePtr->translation[1];
            Translation.Z = NodePtr->translation[2];
        }

        FQuat Rotation(FQuat::Identity);
        if (NodePtr->rotation.size() == 4)
        {
            Rotation.X = NodePtr->rotation[0];
            Rotation.Y = NodePtr->rotation[1];
            Rotation.Z = NodePtr->rotation[2];
            Rotation.W = NodePtr->rotation[3];
        }

        FVector Scale(1.0f);
        if (NodePtr->scale.size() == 3)
        {
            Scale.X = NodePtr->scale[0];
            Scale.Y = NodePtr->scale[1];
            Scale.Z = NodePtr->scale[2];
        }

        if (Translation != FVector::ZeroVector || Rotation != FQuat::Identity || Scale != FVector(1.0f))
        {
            /// use TRS
            Transform = FTransform(Rotation, Translation, Scale);
            FglTFImporter::ConvertToUnrealSpace(Transform, bSwapYZ, !bSwapYZ);
        }
    }

    return true;
}

bool FglTFImporter::GetNodeParentIndicesAndTransforms(const std::shared_ptr<libgltf::SGlTF>& InGlTF, TArray<int32>& OutParentIndices, TArray<FTransform>& OutRelativeTransforms, TArray<FTransform>& OutAbsoluteTransforms, bool bSwapYZ /*= true*/)
{
    if (!GetNodeParentIndices(InGlTF, OutParentIndices)) return false;
    if (!GetNodeRelativeTransforms(InGlTF, OutRelativeTransforms, bSwapYZ)) return false;
    if (OutParentIndices.Num() != OutRelativeTransforms.Num()) return false;

    OutAbsoluteTransforms = OutRelativeTransforms;

    TArray<int32> VisitedIndices;
    TArray<int32> ParentIndices;
    do
    {
        TArray<int32> ChildIndices;

        for (int32 i = 0; i < OutParentIndices.Num(); ++i)
        {
            int32 ParentIndex = OutParentIndices[i];
            if (ParentIndices.Num() <= 0)
            {
                if (ParentIndex == INDEX_NONE)
                {
                    ChildIndices.Add(i);
                }
            }
            else if (ParentIndices.Contains(ParentIndex))
            {
                ChildIndices.Add(i);
            }
        }

        for (int32 NextTargetIndex : ChildIndices)
        {
            check(NextTargetIndex != INDEX_NONE);
            if (OutParentIndices[NextTargetIndex] == INDEX_NONE) continue;
            OutAbsoluteTransforms[NextTargetIndex] = OutRelativeTransforms[NextTargetIndex] * OutAbsoluteTransforms[OutParentIndices[NextTargetIndex]];
        }

        VisitedIndices.Append(ChildIndices);

        ParentIndices = ChildIndices;

    } while (ParentIndices.Num() > 0 && VisitedIndices.Num() < OutParentIndices.Num());
    return true;
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

const FMatrix& FglTFImporter::GetglTFSpaceToUnrealSpace(bool bSwapYZ /*= true*/, bool bInverseX /*= false*/)
{
    static const FMatrix GglTFSpaceToUnrealSpacePX(
          FVector( 1.0f,  0.0f,  0.0f)
        , FVector( 0.0f,  1.0f,  0.0f)
        , FVector( 0.0f,  0.0f,  1.0f)
        , FVector( 0.0f,  0.0f,  0.0f)
    );
    static const FMatrix GglTFSpaceToUnrealSpaceNX(
          FVector(-1.0f,  0.0f,  0.0f)
        , FVector( 0.0f,  1.0f,  0.0f)
        , FVector( 0.0f,  0.0f,  1.0f)
        , FVector( 0.0f,  0.0f,  0.0f)
    );
    static const FMatrix GglTFSpaceToUnrealSpaceSwapYZAndPX(
          FVector( 1.0f,  0.0f,  0.0f)
        , FVector( 0.0f,  0.0f,  1.0f)
        , FVector( 0.0f,  1.0f,  0.0f)
        , FVector( 0.0f,  0.0f,  0.0f)
    );
    static const FMatrix GglTFSpaceToUnrealSpaceSwapYZAndNX(
          FVector(-1.0f,  0.0f,  0.0f)
        , FVector( 0.0f,  0.0f,  1.0f)
        , FVector( 0.0f,  1.0f,  0.0f)
        , FVector( 0.0f,  0.0f,  0.0f)
    );
    return (!bSwapYZ
        ? (!bInverseX ? GglTFSpaceToUnrealSpacePX : GglTFSpaceToUnrealSpaceNX)
        : (!bInverseX ? GglTFSpaceToUnrealSpaceSwapYZAndPX : GglTFSpaceToUnrealSpaceSwapYZAndNX)
        );
}

FMatrix& FglTFImporter::ConvertToUnrealSpace(FMatrix& InOutValue, bool bSwapYZ /*= true*/, bool bInverseX /*= false*/)
{
    if (!bSwapYZ && !bInverseX) return InOutValue;
    FMatrix Convert(FglTFImporter::GetglTFSpaceToUnrealSpace(bSwapYZ, bInverseX));
    InOutValue = Convert * InOutValue * Convert;
    return InOutValue;
}

FTransform& FglTFImporter::ConvertToUnrealSpace(FTransform& InOutValue, bool bSwapYZ /*= true*/, bool bInverseX /*= false*/)
{
    if (!bSwapYZ && !bInverseX) return InOutValue;
    FTransform Convert(FglTFImporter::GetglTFSpaceToUnrealSpace(bSwapYZ, bInverseX));
    InOutValue = Convert * InOutValue * Convert;
    return InOutValue;
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

ERichCurveInterpMode FglTFImporter::StringToRichCurveInterpMode(const FString& InInterpolation)
{
    ERichCurveInterpMode RichCurveInterpMode = RCIM_None;
    if (InInterpolation.Equals(TEXT("LINEAR"), ESearchCase::IgnoreCase))
    {
        RichCurveInterpMode = RCIM_Linear;
    }
    else if (InInterpolation.Equals(TEXT("STEP"), ESearchCase::IgnoreCase))
    {
        RichCurveInterpMode = RCIM_Constant;
    }
    else if (InInterpolation.Equals(TEXT("CUBICSPLINE"), ESearchCase::IgnoreCase))
    {
        RichCurveInterpMode = RCIM_Cubic;
    }
    //WARN:
    return RichCurveInterpMode;
}
