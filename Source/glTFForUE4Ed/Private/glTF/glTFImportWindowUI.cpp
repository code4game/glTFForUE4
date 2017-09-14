#include "glTFForUE4EdPrivatePCH.h"
#include "glTF/glTFImportWindowUI.h"

UglTFImportWindowUI::UglTFImportWindowUI(const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
    , bImportAnimations(false)
    , AnimationName(TEXT(""))
{
    //
}
