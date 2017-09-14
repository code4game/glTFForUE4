#pragma once

class FglTFImporter
{
public:
    static const FglTFImporter& Get();

private:
    FglTFImporter();
    virtual ~FglTFImporter();

public:
};
