// Copyright(c) 2016 - 2023 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEdAnimationSequence.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporter.h"

#if GLTFFORUE_ENGINE_VERSION < 501
#    include <AssetRegistryModule.h>
#else
#    include <AssetRegistry/AssetRegistryModule.h>
#endif
#include <ComponentReregisterContext.h>
#include <AnimationBlueprintLibrary.h>
#include <Animation/AnimCurveCompressionSettings.h>

#if GLTFFORUE_ENGINE_VERSION < 413
#else
#include <Animation/AnimSequence.h>
#endif

#define LOCTEXT_NAMESPACE "glTFForUE4EdModule"

TSharedPtr<FglTFImporterEdAnimationSequence> FglTFImporterEdAnimationSequence::Get(UFactory* InFactory, UObject* InParent, FName InName, EObjectFlags InFlags, FFeedbackContext* InFeedbackContext)
{
    TSharedPtr<FglTFImporterEdAnimationSequence> glTFImporterEdAnimationSequence = MakeShareable(new FglTFImporterEdAnimationSequence);
    glTFImporterEdAnimationSequence->Set(InParent, InName, InFlags, InFeedbackContext);
    glTFImporterEdAnimationSequence->InputFactory = InFactory;
    return glTFImporterEdAnimationSequence;
}

FglTFImporterEdAnimationSequence::FglTFImporterEdAnimationSequence()
    : Super()
{
    //
}

FglTFImporterEdAnimationSequence::~FglTFImporterEdAnimationSequence()
{
    //
}

UAnimSequence* FglTFImporterEdAnimationSequence::CreateAnimationSequence(const TWeakPtr<FglTFImporterOptions>& InglTFImporterOptions, const std::shared_ptr<libgltf::SGlTF>& InglTF
    , const FglTFBuffers& InBuffers, const TMap<int32, FString>& InNodeIndexToBoneNames, const TArray<FString>& InMorphTargetNames
    , USkeletalMesh* InSkeletalMesh, USkeleton* InSkeleton
    , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper
    , FglTFImporterCollection& InOutglTFImporterCollection) const
{
    if (!InglTF || InglTF->animations.empty() || !InSkeleton) return nullptr;

    FText TaskName = FText::Format(LOCTEXT("BeginImportSkeletalAnimationTask", "Importing the skeletal animation ({0})"), FText::FromName(InputName));
    glTFForUE4::FFeedbackTaskWrapper FeedbackTaskWrapper(FeedbackContext, TaskName, true);

    FString AnimationObjectName = FString::Printf(TEXT("%s_AnimationSequence"), *InputName.ToString());

    UAnimSequence* AnimSequence = LoadObject<UAnimSequence>(InputParent, *AnimationObjectName);
    if (!AnimSequence)
    {
        AnimSequence = NewObject<UAnimSequence>(InputParent, UAnimSequence::StaticClass(), *AnimationObjectName, InputFlags);
        checkSlow(AnimSequence);
        if (AnimSequence) FAssetRegistryModule::AssetCreated(AnimSequence);
    }
    else
    {
        UAnimationBlueprintLibrary::RemoveAllCurveData(AnimSequence);
#if GLTFFORUE_ENGINE_VERSION < 500
        AnimSequence->CleanAnimSequenceForImport();
#else
        AnimSequence->GetController().ResetModel();
#endif
    }
    //WARN:
    if (!AnimSequence) return nullptr;

    AnimSequence->SetSkeleton(InSkeleton);
    AnimSequence->CreateAnimation(InSkeletalMesh);
    
#if GLTFFORUE_ENGINE_VERSION < 500
#else
    UAnimDataModel* AnimDataModelPtr = AnimSequence->GetDataModel();
    IAnimationDataController& AnimationDataControllerRef = AnimSequence->GetController();
#endif

    FeedbackTaskWrapper.StatusUpdate(0, 2, LOCTEXT("SetSkeleton", "Set the skeleton to an animation sequence"));

    TArray<FglTFAnimationSequenceDatas> glTFAnimationSequenceDatasArray;

    FeedbackTaskWrapper.StatusUpdate(0, 2, LOCTEXT("GetAnimationSequenceData", "1/2 Get the animation sequence data from glTF"));

    for (int32 i = 0; i < static_cast<int32>(InglTF->animations.size()); ++i)
    {
        const std::shared_ptr<libgltf::SAnimation>& glTFAnimationPtr = InglTF->animations[i];
        FglTFAnimationSequenceDatas glTFAnimationSequenceDatas;
        if (!FglTFImporter::GetAnimationSequenceData(InglTF, glTFAnimationPtr, InBuffers, InMorphTargetNames.Num(), glTFAnimationSequenceDatas))
        {
            //WARN:
            continue;
        }
        glTFAnimationSequenceDatasArray.Add(glTFAnimationSequenceDatas);

        FeedbackTaskWrapper.StatusUpdate(i, static_cast<int32>(InglTF->animations.size()), LOCTEXT("GetAnimationSequenceData", "1/2 Push the animation sequence data from glTF to a array"));
    }

    /// recalculate the transform for the animation sequence.
    for (FglTFAnimationSequenceDatas& SequenceDatas : glTFAnimationSequenceDatasArray)
    {
        for (FglTFAnimationSequenceData& SequenceData : SequenceDatas.Datas)
        {
            for (FglTFAnimationSequenceKeyData& SequenceKeyData : SequenceData.KeyDatas)
            {
                SequenceKeyData.Transform *= InOutglTFImporterCollection.FindNodeInfo(SequenceData.NodeIndex).RelativeTransform.Inverse();
            }
        }
    }

    FeedbackTaskWrapper.StatusUpdate(1, 2, LOCTEXT("AddKeyToSequence", "2/2 Add the key to sequence"));
    
    int32 NumFrames = 0;
    float SequenceLength = 0.0f;
    for (int32 i = 0; i < glTFAnimationSequenceDatasArray.Num(); ++i)
    {
        const FglTFAnimationSequenceDatas& glTFAnimationSequenceDatas = glTFAnimationSequenceDatasArray[i];
        for (const FglTFAnimationSequenceData& glTFAnimationSequenceData : glTFAnimationSequenceDatas.Datas)
        {
            if (glTFAnimationSequenceData.NodeIndex < 0)
            {
                //WARN:
                continue;
            }
            if (!InOutglTFImporterCollection.NodeInfos.Contains(glTFAnimationSequenceData.NodeIndex))
            {
                //WARN:
                continue;
            }
            if (!InNodeIndexToBoneNames.Contains(glTFAnimationSequenceData.NodeIndex))
            {
                //WARN:
                continue;
            }

            const FName BoneName = FName(*(InNodeIndexToBoneNames[glTFAnimationSequenceData.NodeIndex]));
            FName CurveName = BoneName;
#if GLTFFORUE_ENGINE_VERSION < 414
            FSmartNameMapping::UID CurveUID = FSmartNameMapping::MaxUID;
#else
            SmartName::UID_Type CurveUID = SmartName::MaxUID;
#endif
#if GLTFFORUE_ENGINE_VERSION < 411
            FSmartNameMapping* NameMapping = InSkeleton->SmartNames.GetContainer(USkeleton::AnimTrackCurveMappingName);
            if (NameMapping == nullptr)
            {
                InSkeleton->Modify(true);
                InSkeleton->SmartNames.AddContainer(USkeleton::AnimTrackCurveMappingName);
                NameMapping = InSkeleton->SmartNames.GetContainer(USkeleton::AnimTrackCurveMappingName);
                check(NameMapping);
            }
            if (!NameMapping->AddOrFindName(CurveName, CurveUID))
            {
                //WARN:
                continue;
            }
#elif GLTFFORUE_ENGINE_VERSION < 413
            const FSmartNameMapping* NameMapping = InSkeleton->GetOrAddSmartNameContainer(USkeleton::AnimTrackCurveMappingName);
            check(NameMapping);
            if (!InSkeleton->AddSmartNameAndModify(USkeleton::AnimTrackCurveMappingName, CurveName, CurveUID))
            {
                //WARN:
                continue;
            }
#else
            FSmartName NewSmartName;
            if (!InSkeleton->AddSmartNameAndModify(USkeleton::AnimTrackCurveMappingName, CurveName, NewSmartName))
            {
                if (!InSkeleton->GetSmartNameByName(USkeleton::AnimTrackCurveMappingName, CurveName, NewSmartName))
                {
                    //WARN:
                    continue;
                }
            }
            CurveUID = NewSmartName.UID;
#endif

            TArray<FName> MorphTargetNames;
            MorphTargetNames.SetNum(InMorphTargetNames.Num());
            for (int32 j = 0, jc = InMorphTargetNames.Num(); j < jc; ++j)
            {
                MorphTargetNames[j] = FName(*InMorphTargetNames[j]);
                UAnimationBlueprintLibrary::AddCurve(AnimSequence, MorphTargetNames[j]);
            }

            for (const FglTFAnimationSequenceKeyData& KeyData : glTFAnimationSequenceData.KeyDatas)
            {
                if (KeyData.Flags & FglTFAnimationSequenceKeyData::EFlag_Transform)
                {
#if GLTFFORUE_ENGINE_VERSION < 500
                    AnimSequence->bNeedsRebake = true;
                    AnimSequence->AddKeyToSequence(KeyData.Time, CurveName, KeyData.Transform);

#if GLTFFORUE_ENGINE_VERSION < 416
                    FTransformCurve* TransformCurve = static_cast<FTransformCurve*>(AnimSequence->RawCurveData.GetCurveData(CurveUID, FRawCurveTracks::TransformType));
#else
                    FTransformCurve* TransformCurve = static_cast<FTransformCurve*>(AnimSequence->RawCurveData.GetCurveData(CurveUID, ERawCurveTrackTypes::RCT_Transform));
#endif
                    check(TransformCurve);
                    {
                        FKeyHandle KeyHandle = TransformCurve->TranslationCurve.FloatCurves->FindKey(KeyData.Time);
                        TransformCurve->TranslationCurve.FloatCurves->SetKeyInterpMode(KeyHandle, KeyData.TranslationInterpolation);
                    }
                    {
                        FKeyHandle KeyHandle = TransformCurve->RotationCurve.FloatCurves->FindKey(KeyData.Time);
                        TransformCurve->RotationCurve.FloatCurves->SetKeyInterpMode(KeyHandle, KeyData.RotationInterpolation);
                        //TransformCurve->RotationCurve.FloatCurves->SetKeyTangentMode(KeyHandle, ERichCurveTangentMode::RCTM_Auto);
                        //TransformCurve->RotationCurve.FloatCurves->SetKeyTangentWeightMode(KeyHandle, ERichCurveTangentWeightMode::RCTWM_WeightedBoth);
                    }
                    {
                        FKeyHandle KeyHandle = TransformCurve->ScaleCurve.FloatCurves->FindKey(KeyData.Time);
                        TransformCurve->ScaleCurve.FloatCurves->SetKeyInterpMode(KeyHandle, KeyData.ScaleInterpolation);
                    }
#else
                    const FAnimationCurveIdentifier AnimationCurveIdentifier(NewSmartName, ERawCurveTrackTypes::RCT_Transform);
                    if (!AnimDataModelPtr->FindTransformCurve(AnimationCurveIdentifier))
                        AnimationDataControllerRef.AddCurve(AnimationCurveIdentifier);
                    AnimationDataControllerRef.SetTransformCurveKey(AnimationCurveIdentifier, KeyData.Time, KeyData.Transform);
#endif
                }
                if ((InMorphTargetNames.Num() == KeyData.Weights.Num()) &&
                    (KeyData.Flags & FglTFAnimationSequenceKeyData::EFlag_Weights))
                {
#if GLTFFORUE_ENGINE_VERSION < 500
                    AnimSequence->bNeedsRebake = true;
#endif

                    /// set the morph target
                    for (int32 j = 0, jc = InMorphTargetNames.Num(); j < jc; ++j)
                    {
                        UAnimationBlueprintLibrary::AddFloatCurveKey(AnimSequence, MorphTargetNames[j], KeyData.Time, KeyData.Weights[j]);
                    }
                }

                SequenceLength = FMath::Max(SequenceLength, KeyData.Time);
            }
        }
        NumFrames = FMath::Max(NumFrames, glTFAnimationSequenceDatas.Datas.Num());

        FeedbackTaskWrapper.StatusUpdate(i + glTFAnimationSequenceDatasArray.Num(), glTFAnimationSequenceDatasArray.Num() * 2, LOCTEXT("AddKeyToSequence", "2/2 Add the key to sequence"));
    }
    
    if (SequenceLength == 0)
    {
        NumFrames = 1;
    }

#if GLTFFORUE_ENGINE_VERSION < 500
#   if GLTFFORUE_ENGINE_VERSION < 422
    AnimSequence->NumFrames = NumFrames;
#   else
    AnimSequence->SetRawNumberOfFrame(NumFrames);
#   endif
    AnimSequence->SequenceLength = SequenceLength;
#else
    AnimationDataControllerRef.SetFrameRate(FFrameRate(NumFrames, 1));
    AnimationDataControllerRef.SetPlayLength(SequenceLength);
#endif
    
#if GLTFFORUE_ENGINE_VERSION < 500
    if (AnimSequence->HasSourceRawData())
    {
        AnimSequence->BakeTrackCurvesToRawAnimation();
    }
    else
    {
        AnimSequence->PostProcessSequence();
    }
#endif
    AnimSequence->Modify(true);
    AnimSequence->MarkPackageDirty();
#if GLTFFORUE_ENGINE_VERSION < 500
    AnimSequence->MarkRawDataAsModified();
#endif

    // Reregister skeletal mesh components so they reflect the updated animation
    for (TObjectIterator<USkeletalMeshComponent> Iter; Iter; ++Iter)
    {
        FComponentReregisterContext ReregisterContext(*Iter);
    }
    return AnimSequence;
}

#undef LOCTEXT_NAMESPACE
