// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTFBinaryFactory.h"

#include "glTFImporterEd.h"

#include "Engine/StaticMesh.h"
#if ENGINE_MINOR_VERSION < 14
#include "Misc/CoreMisc.h"
#else
#include "Misc/FileHelper.h"
#endif
#include "Misc/Paths.h"

#define GLTF_FILE_SIZE_MAX      0x7FFFFFFF
#define GLTF_ASCII_UINT32(x)    ((((x) & 0xFF000000) >> 24) | (((x) & 0x000000FF) << 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8))

UglTFBinaryFactory::UglTFBinaryFactory(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    SupportedClass = UStaticMesh::StaticClass();
    if (Formats.Num() > 0) Formats.Empty();
    Formats.Add(TEXT("glb;glTF 2.0 binary"));

    bCreateNew = false;
    bText = false;
    bEditorImport = true;
}

bool UglTFBinaryFactory::FactoryCanImport(const FString& InFilename)
{
    return FPaths::GetExtension(InFilename).Equals(TEXT("glb"), ESearchCase::IgnoreCase);
}

UObject* UglTFBinaryFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, const TCHAR* InType, const uint8*& InBuffer, const uint8* InBufferEnd, FFeedbackContext* InWarn)
{
    if (!InClass || !InParent || !InBuffer || !InBufferEnd || InBuffer == InBufferEnd) return nullptr;

    uint64 BuffSize = InBufferEnd - InBuffer;
    if (BuffSize <= 0 || BuffSize >= GLTF_FILE_SIZE_MAX)
    {
        //TODO:
        return nullptr;
    }

    struct FglTFFileHeader
    {
        uint32 Magic;
        uint32 Version;
        uint32 Length;
    };

    uint64 Offset = 0;
    if (BuffSize < sizeof(FglTFFileHeader))
    {
        //TODO:
        return nullptr;
    }

    FglTFFileHeader glTFFileHeader;
    FMemory::Memcpy(&glTFFileHeader, InBuffer + Offset, sizeof(FglTFFileHeader));
    Offset += sizeof(FglTFFileHeader);

    if (glTFFileHeader.Magic != GLTF_ASCII_UINT32('glTF')
        || glTFFileHeader.Version != 2
        || glTFFileHeader.Length != BuffSize)
    {
        //TODO:
        return nullptr;
    }

    struct FglTFChunkHeader
    {
        uint32 Length;
        uint32 Type;
    };

    FString glTFJson;
    TSharedPtr<FglTFBuffers> glTFBuffers = MakeShareable(new FglTFBuffers(true));
    uint32 BinaryIndex = 0;
    while (Offset < BuffSize)
    {
        FglTFChunkHeader glTFChunkHeader;
        FMemory::Memcpy(&glTFChunkHeader, InBuffer + Offset, sizeof(FglTFChunkHeader));
        Offset += sizeof(FglTFChunkHeader);

        if ((Offset + glTFChunkHeader.Length) > BuffSize)
        {
            //TODO:
            return nullptr;
        }

        if (glTFChunkHeader.Type == GLTF_ASCII_UINT32('JSON'))
        {
            if (glTFJson.IsEmpty())
            {
                FFileHelper::BufferToString(glTFJson, InBuffer + Offset, glTFChunkHeader.Length);
                if (glTFJson.IsEmpty())
                {
                    //TODO:
                    return nullptr;
                }
            }
            else
            {
                //TODO:
                return nullptr;
            }
        }
        else if (glTFChunkHeader.Type == GLTF_ASCII_UINT32('BIN\0'))
        {
            TArray<uint8> ChunkData;
            ChunkData.SetNumZeroed(glTFChunkHeader.Length);
            FMemory::Memcpy(ChunkData.GetData(), InBuffer + Offset, glTFChunkHeader.Length);
            glTFBuffers->CacheBinary(BinaryIndex++, ChunkData);
        }

        Offset += glTFChunkHeader.Length;
    }

    return FactoryCreate(InClass, InParent, InName, InFlags, InContext, InType, InWarn, glTFJson, glTFBuffers);
}
