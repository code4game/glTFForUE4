// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEdMaterial.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporterEdTexture.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

#define GLTF_MATERIAL_PBRMETALLICROUGHNESS_ORIGIN   TEXT("/glTFForUE4/Materials/M_PBRMetallicRoughnessOrigin.M_PBRMetallicRoughnessOrigin")
#define GLTF_MATERIAL_PBRSPECULARGLOSSINESS_ORIGIN  TEXT("/glTFForUE4/Materials/M_PBRSpecularGlossinessOrigin.M_PBRSpecularGlossinessOrigin")

namespace glTFForUE4Ed
{
    template<typename TMaterialExpression>
    TMaterialExpression* FindExpressionParameterByGUID(UMaterial* InMaterial, const FGuid& InGuid)
    {
        if (!InMaterial || !InGuid.IsValid()) return nullptr;

        for (UMaterialExpression* Expression : InMaterial->Expressions)
        {
            if (!Expression || !UMaterial::IsParameter(Expression)) continue;
            TMaterialExpression* ExpressionTemp = Cast<TMaterialExpression>(Expression);
            if (!ExpressionTemp || ExpressionTemp->ExpressionGUID != InGuid) continue;
            return ExpressionTemp;
        }
        return nullptr;
    }
}

TSharedPtr<FglTFImporterEdMaterial> FglTFImporterEdMaterial::Get(UFactory* InFactory, UObject* InParent, FName InName, EObjectFlags InFlags, FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEdMaterial> glTFImporterEdMaterial = MakeShareable(new FglTFImporterEdMaterial);
    glTFImporterEdMaterial->Set(InParent, InName, InFlags, InFeedbackContext);
    glTFImporterEdMaterial->InputFactory = InFactory;
    return glTFImporterEdMaterial;
}

FglTFImporterEdMaterial::FglTFImporterEdMaterial()
    : Super()
{
    //
}

FglTFImporterEdMaterial::~FglTFImporterEdMaterial()
{
    //
}

UMaterial* FglTFImporterEdMaterial::CreateMaterial(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions
    , const std::shared_ptr<libgltf::SGlTF>& InglTF, const int32_t InMaterialId, const FglTFBuffers& InBuffers, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper
    , FglTFImporterCollection& InOutglTFImporterCollection) const
{
    if (!InglTF) return nullptr;
    if (!InputParent) return nullptr;
    if (InOutglTFImporterCollection.Materials.Contains(InMaterialId))
    {
        return InOutglTFImporterCollection.Materials[InMaterialId];
    }
    if (InMaterialId < 0 || InMaterialId >= static_cast<int32>(InglTF->materials.size())) return nullptr;

    const TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();

    const std::shared_ptr<libgltf::SMaterial>& glTFMaterial = InglTF->materials[InMaterialId];
    if (!glTFMaterial) return nullptr;

    const libgltf::SKHR_materials_pbrSpecularGlossinessglTFextension* ExternalMaterialPBRSpecularGlossiness = nullptr;
    {
        const std::shared_ptr<libgltf::SExtension>& Extensions = glTFMaterial->extensions;
        const GLTFString extension_property = GLTF_TCHAR_TO_GLTFSTRING(TEXT("KHR_materials_pbrSpecularGlossiness"));
        if (!!Extensions && (Extensions->properties.find(extension_property) != Extensions->properties.end()))
        {
            ExternalMaterialPBRSpecularGlossiness = (const libgltf::SKHR_materials_pbrSpecularGlossinessglTFextension*)Extensions->properties[extension_property].get();
        }
    }

    const FString MaterialName = FglTFImporter::SanitizeObjectName(!glTFMaterial->name.empty()
        ? FString::Printf(TEXT("M_%s_%d_%s"), *InputName.ToString(), InMaterialId, GLTF_GLTFSTRING_TO_TCHAR(glTFMaterial->name.c_str()))
        : FString::Printf(TEXT("M_%s_%d"), *InputName.ToString()));
    const FString PackageName = FPackageName::GetLongPackagePath(InputParent->GetPathName()) / MaterialName;

    UPackage* MaterialPackage = FindPackage(nullptr, *PackageName);
    if (!MaterialPackage)
    {
        MaterialPackage = CreatePackage(nullptr, *PackageName);
    }
    if (!MaterialPackage) return nullptr;
    MaterialPackage->FullyLoad();

    UMaterial* NewMaterial = FindObject<UMaterial>(MaterialPackage, *MaterialName);
    if (!NewMaterial)
    {
        UMaterial* glTFMaterialOrigin = nullptr;
        if (ExternalMaterialPBRSpecularGlossiness)
        {
            glTFMaterialOrigin = LoadObject<UMaterial>(nullptr, GLTF_MATERIAL_PBRSPECULARGLOSSINESS_ORIGIN);
        }
        else
        {
            glTFMaterialOrigin = LoadObject<UMaterial>(nullptr, GLTF_MATERIAL_PBRMETALLICROUGHNESS_ORIGIN);
        }
        if (!glTFMaterialOrigin) return nullptr;

        NewMaterial = Cast<UMaterial>(StaticDuplicateObject(glTFMaterialOrigin, MaterialPackage, *MaterialName, InputFlags, glTFMaterialOrigin->GetClass()));
        checkSlow(NewMaterial);
        if (NewMaterial) FAssetRegistryModule::AssetCreated(NewMaterial);
    }
    if (!NewMaterial) return nullptr;

    NewMaterial->PreEditChange(nullptr);

    TMap<FName, FGuid> ScalarParameterNameToGuid;
    {
        TArray<FName> ParameterNames;
        TArray<FGuid> ParameterGuids;
#if (ENGINE_MINOR_VERSION <= 18)
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
#if (ENGINE_MINOR_VERSION <= 18)
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
#if (ENGINE_MINOR_VERSION <= 18)
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
            if (!ConstructSampleParameter(InglTFImporterOptions, InglTF, glTFMaterial->emissiveTexture, InBuffers, TEXT("emissiveTexture"), SampleParameter, false, InFeedbackTaskWrapper, InOutglTFImporterCollection))
            {
                InFeedbackTaskWrapper.Log(ELogVerbosity::Warning, LOCTEXT("FailedToConstructTheEmissiveTexture", "Failed to construct the `emissiveTexture`"));
            }
        }
    }
    if (ExternalMaterialPBRSpecularGlossiness)
    {
        if (ExternalMaterialPBRSpecularGlossiness->diffuseFactor.size() == 4
            && VectorParameterNameToGuid.Contains(TEXT("diffuseFactor")))
        {
            if (UMaterialExpressionVectorParameter* VectorParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionVectorParameter>(NewMaterial, VectorParameterNameToGuid[TEXT("diffuseFactor")]))
            {
                VectorParameter->DefaultValue.R = ExternalMaterialPBRSpecularGlossiness->diffuseFactor[0];
                VectorParameter->DefaultValue.G = ExternalMaterialPBRSpecularGlossiness->diffuseFactor[1];
                VectorParameter->DefaultValue.B = ExternalMaterialPBRSpecularGlossiness->diffuseFactor[2];
                VectorParameter->DefaultValue.A = ExternalMaterialPBRSpecularGlossiness->diffuseFactor[3];
            }
        }
        if (!!(ExternalMaterialPBRSpecularGlossiness->diffuseTexture)
            && TextureParameterNameToGuid.Contains(TEXT("diffuseTexture")))
        {
            if (UMaterialExpressionTextureSampleParameter* SampleParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionTextureSampleParameter>(NewMaterial, TextureParameterNameToGuid[TEXT("diffuseTexture")]))
            {
                if (!ConstructSampleParameter(InglTFImporterOptions, InglTF, ExternalMaterialPBRSpecularGlossiness->diffuseTexture, InBuffers, TEXT("diffuseTexture"), SampleParameter, false, InFeedbackTaskWrapper, InOutglTFImporterCollection))
                {
                    InFeedbackTaskWrapper.Log(ELogVerbosity::Warning, LOCTEXT("FailedToConstructTheDiffuseTexture", "Failed to construct the `diffuseTexture`"));
                }
            }
        }
        if (ExternalMaterialPBRSpecularGlossiness->specularFactor.size() == 3
            && VectorParameterNameToGuid.Contains(TEXT("specularGlossinessFactor")))
        {
            if (UMaterialExpressionVectorParameter* VectorParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionVectorParameter>(NewMaterial, VectorParameterNameToGuid[TEXT("specularGlossinessFactor")]))
            {
                VectorParameter->DefaultValue.R = ExternalMaterialPBRSpecularGlossiness->specularFactor[0];
                VectorParameter->DefaultValue.G = ExternalMaterialPBRSpecularGlossiness->specularFactor[1];
                VectorParameter->DefaultValue.B = ExternalMaterialPBRSpecularGlossiness->specularFactor[2];
                VectorParameter->DefaultValue.A = ExternalMaterialPBRSpecularGlossiness->glossinessFactor;
            }
        }
        if (!!(ExternalMaterialPBRSpecularGlossiness->specularGlossinessTexture)
            && TextureParameterNameToGuid.Contains(TEXT("specularGlossinessTexture")))
        {
            if (UMaterialExpressionTextureSampleParameter* SampleParameter = glTFForUE4Ed::FindExpressionParameterByGUID<UMaterialExpressionTextureSampleParameter>(NewMaterial, TextureParameterNameToGuid[TEXT("specularGlossinessTexture")]))
            {
                if (!ConstructSampleParameter(InglTFImporterOptions, InglTF, ExternalMaterialPBRSpecularGlossiness->specularGlossinessTexture, InBuffers, TEXT("specularGlossinessTexture"), SampleParameter, false, InFeedbackTaskWrapper, InOutglTFImporterCollection))
                {
                    InFeedbackTaskWrapper.Log(ELogVerbosity::Warning, LOCTEXT("FailedToConstructTheSpecularGlossinessTexture", "Failed to construct the `specularGlossinessTexture`"));
                }
            }
        }
    }
    else if (!!(glTFMaterial->pbrMetallicRoughness))
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
                if (!ConstructSampleParameter(InglTFImporterOptions, InglTF, pbrMetallicRoughness->baseColorTexture, InBuffers, TEXT("baseColorTexture"), SampleParameter, false, InFeedbackTaskWrapper, InOutglTFImporterCollection))
                {
                    InFeedbackTaskWrapper.Log(ELogVerbosity::Warning, LOCTEXT("FailedToConstructTheBaseColorTexture", "Failed to construct the `baseColorTexture`"));
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
                if (!ConstructSampleParameter(InglTFImporterOptions, InglTF, pbrMetallicRoughness->metallicRoughnessTexture, InBuffers, TEXT("metallicRoughnessTexture"), SampleParameter, false, InFeedbackTaskWrapper, InOutglTFImporterCollection))
                {
                    InFeedbackTaskWrapper.Log(ELogVerbosity::Warning, LOCTEXT("FailedToConstructTheRoughnessTexture", "Failed to construct the `metallicRoughnessTexture`"));
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
                if (!ConstructSampleParameter(InglTFImporterOptions, InglTF, glTFMaterial->occlusionTexture, InBuffers, TEXT("occlusionTexture"), SampleParameter, false, InFeedbackTaskWrapper, InOutglTFImporterCollection))
                {
                    InFeedbackTaskWrapper.Log(ELogVerbosity::Warning, LOCTEXT("FailedToConstructTheOcclusionTexture", "Failed to construct the `occlusionTexture`"));
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
        const FString AlphaMode = GLTF_GLTFSTRING_TO_TCHAR(glTFMaterial->alphaMode.c_str());
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
            if (!ConstructSampleParameter(InglTFImporterOptions, InglTF, glTFMaterial->normalTexture, InBuffers, TEXT("normalTexture"), SampleParameter, true, InFeedbackTaskWrapper, InOutglTFImporterCollection))
            {
                InFeedbackTaskWrapper.Log(ELogVerbosity::Warning, LOCTEXT("FailedToConstructTheNormalTexture", "Failed to construct the `normalTexture`"));
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

    InOutglTFImporterCollection.Materials.Add(InMaterialId, NewMaterial);
    return NewMaterial;
}

bool FglTFImporterEdMaterial::ConstructSampleParameter(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions
    , const std::shared_ptr<libgltf::SGlTF>& InglTF, const std::shared_ptr<libgltf::STextureInfo>& InglTFTextureInfo, const FglTFBuffers& InBuffers
    , const FString& InParameterName, UMaterialExpressionTextureSampleParameter* InSampleParameter, bool InIsNormalmap, const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper
    , FglTFImporterCollection& InOutglTFImporterCollection) const
{
    if (!InglTF || !InglTFTextureInfo || !InSampleParameter) return false;
    if (!(InglTFTextureInfo->index)) return false;

    const TSharedPtr<FglTFImporterOptions> glTFImporterOptions = InglTFImporterOptions.Pin();
    check(glTFImporterOptions->Details);
    if (!glTFImporterOptions->Details->bImportTexture) return false;

    TSharedPtr<FglTFImporterEdTexture> glTFImporterEdTexture = FglTFImporterEdTexture::Get(InputFactory, InputParent, InputName, InputFlags, FeedbackContext);
    UTexture* Texture = glTFImporterEdTexture->CreateTexture(InglTFImporterOptions
        , InglTF, InglTFTextureInfo->index, InBuffers, InIsNormalmap, InFeedbackTaskWrapper
        , InOutglTFImporterCollection);
    if (!Texture) return false;

    InSampleParameter->Texture = Texture;
    InSampleParameter->ConstCoordinate = static_cast<uint32>(InglTFTextureInfo->texCoord);
    return true;
}

#undef LOCTEXT_NAMESPACE
