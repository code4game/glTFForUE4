#pragma once

#include "glTF/glTFImportOptionsWindow.h"

namespace libgltf
{
    struct SGlTF;
}

class SglTFImportOptionsWindowEd : public SglTFImportOptionsWindow
{
    typedef SglTFImportOptionsWindow Super;

public:
    static TSharedPtr<struct FglTFImportOptions> Open(const FString& InFilePathInOS, const FString& InFilePathInEngine, const libgltf::SGlTF& InGlTF, bool& OutCancel);

public:
    SglTFImportOptionsWindowEd();

public:
    virtual void Construct(const FArguments& InArgs) override;
};
