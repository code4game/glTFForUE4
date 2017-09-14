// Copyright 2017 Code 4 Game, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

class FglTFForUE4Module : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
