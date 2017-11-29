#include "glTFForUE4PrivatePCH.h"
#include "glTF/glTFImporter.h"

#include "glTF/glTFImportOptions.h"

#include "libgltf/libgltf.h"

#include "Misc/Base64.h"
#include "Misc/SecureHash.h"

FglTFBufferFiles::FglTFBufferFiles(const FString& InFileFolderPath, const std::vector<std::shared_ptr<libgltf::SBuffer>>& InBuffers)
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
                    UE_LOG(LogglTFForUE4, Error, TEXT("Failed to decode the base64 data!"));
                }
                if (BufferData.Num() != Buffer->byteLength)
                {
                    BufferData.Empty();
                    UE_LOG(LogglTFForUE4, Error, TEXT("The size of data is not same as buffer"));
                }
            }
            else
            {
                UE_LOG(LogglTFForUE4, Error, TEXT("Can't decode the type(%s) of the string"), *StreamType);
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

const TArray<uint8>& FglTFBufferFiles::operator[](const FString& InKey) const
{
    const TArray<uint8>* FoundBufferFile = BufferFiles.Find(InKey);
    if (!FoundBufferFile) return EmptyBufferFile;
    return *FoundBufferFile;
}

const TArray<uint8>& FglTFBufferFiles::operator[](int32 InIndex) const
{
    const FString* UriPtr = IndexToUri.Find(InIndex);
    if (!UriPtr) return EmptyBufferFile;
    return (*this)[*UriPtr];
}

const FglTFImporter& FglTFImporter::Get(FFeedbackContext* InFeedbackContext)
{
    static const FglTFImporter glTFImporterInstance(InFeedbackContext);
    return glTFImporterInstance;
}

FglTFImporter::FglTFImporter(FFeedbackContext* InFeedbackContext)
    : FeedbackContext(InFeedbackContext)
{
    check(FeedbackContext);
}

FglTFImporter::~FglTFImporter()
{
    //
}

UObject* FglTFImporter::Create(const TWeakPtr<FglTFImportOptions>& InglTFImportOptions
    , const std::shared_ptr<libgltf::SGlTF>& InGlTF
    , UClass* InClass, UObject* InParent) const
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
    FglTFBufferFiles BufferFiles(FolderPathInOS, InGlTF->buffers);

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
bool GetAccessorData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, const std::shared_ptr<libgltf::SAccessor>& InAccessor, TArray<TEngineDataType>& OutDataArray)
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
        if (InBufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset + InAccessor->byteOffset, InAccessor->count, BufferView->byteStride, AccessorDataArray))
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
        if (InBufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset + InAccessor->byteOffset, InAccessor->count, BufferView->byteStride, AccessorDataArray))
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
        if (InBufferFiles.Get((int32)(*BufferView->buffer), BufferView->byteOffset + InAccessor->byteOffset, InAccessor->count, BufferView->byteStride, AccessorDataArray))
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
bool GetAccessorData(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, const std::shared_ptr<libgltf::SAccessor>& InAccessor, TArray<TEngineDataType>& OutDataArray)
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
        UE_LOG(LogglTFForUE4, Error, TEXT("Not support the accessor's componetType(%d)?"), InAccessor->componentType);
        break;
    }
    return false;
}

bool FglTFImporter::GetTriangleIndices(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<uint32>& OutTriangleIndices)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutTriangleIndices);
}

bool FglTFImporter::GetVertexPositions(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector>& OutVertexPositions)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexPositions);
}

bool FglTFImporter::GetVertexNormals(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector>& OutVertexNormals)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexNormals);
}

bool FglTFImporter::GetVertexTangents(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector4>& OutVertexTangents)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexTangents);
}

bool FglTFImporter::GetVertexTexcoords(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector2D>& OutVertexTexcoords)
{
    if (!InGlTF) return false;

    const std::shared_ptr<libgltf::SAccessor>& Accessor = InGlTF->accessors[InAccessorIndex];
    return GetAccessorData(InGlTF, InBufferFiles, Accessor, OutVertexTexcoords);
}
