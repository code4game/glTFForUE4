// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEdTexture.h"

#include "glTF/glTFImporterOptions.h"

#if (ENGINE_MINOR_VERSION < 18)
#include "ImageWrapper.h"
#else
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#endif
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

TSharedPtr<FglTFImporterEdTexture> FglTFImporterEdTexture::Get(UFactory* InFactory, UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEdTexture> glTFImporterEdTexture = MakeShareable(new FglTFImporterEdTexture);
    glTFImporterEdTexture->Set(InClass, InParent, InName, InFlags, InFeedbackContext);
    glTFImporterEdTexture->InputFactory = InFactory;
    return glTFImporterEdTexture;
}

FglTFImporterEdTexture::FglTFImporterEdTexture()
{
    //
}

FglTFImporterEdTexture::~FglTFImporterEdTexture()
{
    //
}

UTexture* FglTFImporterEdTexture::CreateTexture(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF, const std::shared_ptr<libgltf::STexture>& InglTFTexture, const FglTFBuffers& InBuffers, const FString& InTextureName, bool InIsNormalmap, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper) const
{
    if (!InglTF || !InglTFTexture || !(InglTFTexture->source)) return nullptr;
    int32 ImageIndex = (int32)(*(InglTFTexture->source));
    FString ImageFilePath;
    TArray<uint8> ImageFileData;
    if (!InBuffers.GetImageData(InglTF, ImageIndex, ImageFileData, ImageFilePath)
        || ImageFileData.Num() <= 0)
    {
        return nullptr;
    }

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
#else
    TSharedPtr<IImageWrapper> ImageWrappers[7] = {
#endif
        ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::GrayscaleJPEG),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::ICO),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::EXR),
        ImageWrapperModule.CreateImageWrapper(EImageFormat::ICNS),
    };

    for (auto ImageWrapper : ImageWrappers)
    {
        if (!ImageWrapper.IsValid()) continue;
        if (!ImageWrapper->SetCompressed(ImageFileData.GetData(), ImageFileData.Num())) continue;

        ETextureSourceFormat TextureFormat = TSF_Invalid;

        int32 Width = ImageWrapper->GetWidth();
        int32 Height = ImageWrapper->GetHeight();

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
            InFeedbackTaskWrapper.Log(ELogVerbosity::Error, LOCTEXT("UnsupportedImageFormat", "It is an unsupported image format."));
            break;
        }

        NewTexture = NewObject<UTexture2D>(TexturePackage, UTexture2D::StaticClass(), *InTextureName, InputFlags);
        checkSlow(NewTexture);
        if (!NewTexture) break;
        FAssetRegistryModule::AssetCreated(NewTexture);

        NewTexture->PreEditChange(nullptr);

        if (FMath::IsPowerOfTwo(Width) && FMath::IsPowerOfTwo(Height))
        {
            NewTexture->Source.Init2DWithMipChain(Width, Height, TextureFormat);
        }
        else
        {
            NewTexture->Source.Init(Width, Height, 1, 1, TextureFormat);
            NewTexture->MipGenSettings = TMGS_NoMipmaps;
        }
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
        if (glTFSamplerId >= 0 && glTFSamplerId < static_cast<int32>(InglTF->samplers.size()))
        {
            const std::shared_ptr<libgltf::SSampler>& glTFSampler = InglTF->samplers[glTFSamplerId];
            if (glTFSampler)
            {
                NewTexture->Filter = FglTFImporter::MinFilterToTextureFilter(glTFSampler->minFilter);
                NewTexture->AddressX = FglTFImporter::WrapSToTextureAddress(glTFSampler->wrapS);
                NewTexture->AddressY = FglTFImporter::WrapSToTextureAddress(glTFSampler->wrapT);
            }
        }
    }
    if (NewTexture)
    {
        NewTexture->UpdateResource();
        if (!ImageFilePath.IsEmpty())
        {
            NewTexture->AssetImportData->Update(*ImageFilePath);
        }

        NewTexture->PostEditChange();
        NewTexture->MarkPackageDirty();
    }
    return NewTexture;
}

#undef LOCTEXT_NAMESPACE
