// Copyright 2016 - 2020 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTF/glTFImporterOptionsWindow.h"

namespace libgltf
{
    struct SGlTF;
}

class SglTFImporterOptionsWindowEd : public SglTFImporterOptionsWindow
{
    typedef SglTFImporterOptionsWindow Super;

public:
    static TSharedPtr<struct FglTFImporterOptions> Open(const FString& InFilePathInOS, const FString& InFilePathInEngine, const libgltf::SGlTF& InGlTF, bool& OutCancel);

public:
    SglTFImporterOptionsWindowEd();

public:
    virtual void Construct(const FArguments& InArgs) override;
};
