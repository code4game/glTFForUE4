// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImporterEdAnimationSequence.h"

#include "glTF/glTFImporterOptions.h"
#include "glTF/glTFImporter.h"

#include <AssetRegistryModule.h>
#include <ComponentReregisterContext.h>
#include <AnimationBlueprintLibrary.h>

#if ENGINE_MINOR_VERSION <= 12
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
    , const FglTFBuffers& InBuffers, const TMap<int32, FString>& InNodeIndexToBoneNames
    , USkeletalMesh* InSkeletalMesh, USkeleton* InSkeleton
    , const glTFForUE4::FFeedbackTaskWrapper& InFeedbackTaskWrapper
    , FglTFImporterCollection& InOutglTFImporterCollection) const
{
    if (!InglTF || InglTF->animations.empty() || InglTF->skins.empty() || !InSkeleton) return nullptr;

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
        AnimSequence->CleanAnimSequenceForImport();
    }
    //WARN:
    if (!AnimSequence) return nullptr;

    AnimSequence->SetSkeleton(InSkeleton);
    AnimSequence->CreateAnimation(InSkeletalMesh);

    FeedbackTaskWrapper.StatusUpdate(0, 2, LOCTEXT("SetSkeleton", "Set the skeleton to an animation sequence"));

    TArray<FglTFAnimationSequenceDatas> glTFAnimationSequenceDatasArray;

    FeedbackTaskWrapper.StatusUpdate(0, 2, LOCTEXT("GetAnimationSequenceData", "1/2 Get the animation sequence data from glTF"));

    for (int32 i = 0; i < static_cast<int32>(InglTF->animations.size()); ++i)
    {
        const std::shared_ptr<libgltf::SAnimation>& glTFAnimationPtr = InglTF->animations[i];
        FglTFAnimationSequenceDatas glTFAnimationSequenceDatas;
        if (!FglTFImporter::GetAnimationSequenceData(InglTF, glTFAnimationPtr, InBuffers, glTFAnimationSequenceDatas))
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

            FName CurveName = FName(*(InNodeIndexToBoneNames[glTFAnimationSequenceData.NodeIndex]));
#if ENGINE_MINOR_VERSION <= 13
            FSmartNameMapping::UID CurveUID = FSmartNameMapping::MaxUID;
#else
            SmartName::UID_Type CurveUID = SmartName::MaxUID;
#endif
#if ENGINE_MINOR_VERSION <= 10
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
#elif ENGINE_MINOR_VERSION <= 12
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

            for (const FglTFAnimationSequenceKeyData& KeyData : glTFAnimationSequenceData.KeyDatas)
            {
                AnimSequence->AddKeyToSequence(KeyData.Time, CurveName, KeyData.Transform);
                AnimSequence->bNeedsRebake = true;

#if ENGINE_MINOR_VERSION <= 15
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

                SequenceLength = FMath::Max(SequenceLength, KeyData.Time);
            }
        }
        NumFrames = FMath::Max(NumFrames, glTFAnimationSequenceDatas.Datas.Num());

        FeedbackTaskWrapper.StatusUpdate(i + glTFAnimationSequenceDatasArray.Num(), glTFAnimationSequenceDatasArray.Num() * 2, LOCTEXT("AddKeyToSequence", "2/2 Add the key to sequence"));
    }

    AnimSequence->SequenceLength = SequenceLength;
    if (SequenceLength == 0)
    {
        NumFrames = 1;
    }
#if ENGINE_MINOR_VERSION <= 21
    AnimSequence->NumFrames = NumFrames;
#else
    AnimSequence->SetRawNumberOfFrame(NumFrames);
#endif

    if (AnimSequence->HasSourceRawData())
    {
        AnimSequence->BakeTrackCurvesToRawAnimation();
    }
    else
    {
        AnimSequence->PostProcessSequence();
    }
    AnimSequence->Modify(true);
    AnimSequence->MarkPackageDirty();
    AnimSequence->MarkRawDataAsModified();

    // Reregister skeletal mesh components so they reflect the updated animation
    for (TObjectIterator<USkeletalMeshComponent> Iter; Iter; ++Iter)
    {
        FComponentReregisterContext ReregisterContext(*Iter);
    }
    return AnimSequence;
}

#undef LOCTEXT_NAMESPACE
