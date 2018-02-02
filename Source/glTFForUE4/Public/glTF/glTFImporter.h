// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include <vector>
#include <map>
#include <memory>

namespace libgltf
{
    struct SGlTF;
    struct SGlTFId;
    struct SScene;
    struct SNode;
    struct SMesh;
    struct SMeshPrimitive;
    struct SBuffer;
    struct STextureInfo;
    struct STexture;
}

namespace glTFForUE4
{
    class GLTFFORUE4_API FFeedbackTaskWrapper
    {
    public:
        explicit FFeedbackTaskWrapper(class FFeedbackContext* InFeedbackContext, const FText& InTask, bool InShowProgressDialog);
        virtual ~FFeedbackTaskWrapper();

    public:
        const FFeedbackTaskWrapper& Log(ELogVerbosity::Type InLogVerbosity, const FText& InMessge) const;
        const FFeedbackTaskWrapper& UpdateProgress(int32 InNumerator, int32 InDenominator) const;
        const FFeedbackTaskWrapper& StatusUpdate(int32 InNumerator, int32 InDenominator, const FText& InStatusText) const;
        const FFeedbackTaskWrapper& StatusForceUpdate(int32 InNumerator, int32 InDenominator, const FText& InStatusText) const;

    private:
        class FFeedbackContext* FeedbackContext;
    };
}

class GLTFFORUE4_API FglTFBufferFiles
{
public:
    explicit FglTFBufferFiles(const FString& InFileFolderPath, const std::vector<std::shared_ptr<libgltf::SBuffer>>& InBuffers);

public:
    const TArray<uint8>& operator[](const FString& InKey) const;
    const TArray<uint8>& operator[](int32 InIndex) const;

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

class GLTFFORUE4_API FglTFImporter
{
public:
    static TSharedPtr<FglTFImporter> Get(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);

public:
    FglTFImporter();
    virtual ~FglTFImporter();

public:
    virtual FglTFImporter& Set(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, class FFeedbackContext* InFeedbackContext);
    virtual class UObject* Create(const TWeakPtr<struct FglTFImportOptions>& InglTFImportOptions, const std::shared_ptr<libgltf::SGlTF>& InGlTF) const;

protected:
    UClass* InputClass;
    UObject* InputParent;
    FName InputName;
    EObjectFlags InputFlags;
    class FFeedbackContext* FeedbackContext;

public:
    static bool GetTriangleIndices(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<uint32>& OutTriangleIndices);
    static bool GetVertexPositions(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector>& OutVertexPositions);
    static bool GetVertexNormals(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector>& OutVertexNormals);
    static bool GetVertexTangents(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector4>& OutVertexTangents);
    static bool GetVertexTexcoords(const std::shared_ptr<libgltf::SGlTF>& InGlTF, const FglTFBufferFiles& InBufferFiles, int32 InAccessorIndex, TArray<FVector2D>& OutVertexTexcoords);
};
