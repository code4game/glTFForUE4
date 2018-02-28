// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImporter.h"

#include "glTF/glTFImportOptions.h"

#include "libgltf/libgltf.h"

#include "Misc/Base64.h"
#include "Misc/SecureHash.h"

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
        if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, InAccessor->byteOffset, InAccessor->count, AccessorDataArray, FilePath))
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
        if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, InAccessor->byteOffset, InAccessor->count, AccessorDataArray, FilePath))
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
        if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, InAccessor->byteOffset, InAccessor->count, AccessorDataArray, FilePath))
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
        if (InBuffers.GetBufferViewData(InGlTF, BufferViewIndex, InAccessor->byteOffset, InAccessor->count, AccessorDataArray, FilePath))
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

bool FglTFImporter::GetTriangleIndices(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InBuffers, int32 InAccessorIndex, TArray<uint32>& OutTriangleIndices)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBuffers, Accessor, OutTriangleIndices);
}

bool FglTFImporter::GetVertexPositions(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InBuffers, int32 InAccessorIndex, TArray<FVector>& OutVertexPositions)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBuffers, Accessor, OutVertexPositions);
}

bool FglTFImporter::GetVertexNormals(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InBuffers, int32 InAccessorIndex, TArray<FVector>& OutVertexNormals)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBuffers, Accessor, OutVertexNormals);
}

bool FglTFImporter::GetVertexTangents(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InBuffers, int32 InAccessorIndex, TArray<FVector4>& OutVertexTangents)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBuffers, Accessor, OutVertexTangents);
}

bool FglTFImporter::GetVertexTexcoords(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBuffers& InBuffers, int32 InAccessorIndex, TArray<FVector2D>& OutVertexTexcoords)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBuffers, Accessor, OutVertexTexcoords);
}
