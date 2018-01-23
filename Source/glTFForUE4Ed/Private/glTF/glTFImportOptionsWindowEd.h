// Copyright 2017 - 2018 Code 4 Game, Org. All Rights Reserved.

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
