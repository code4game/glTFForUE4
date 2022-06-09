// Copyright(c) 2016 - 2022 Code 4 Game, Org. All Rights Reserved.

#pragma once

#include "glTFForUE4PublicPCH.h"

#if GLTFFORUE_ENGINE_VERSION < 415
#include <Core.h>
#include <CoreUObject.h>
#include <Engine.h>
#else
#include <CoreMinimal.h>
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogglTFForUE4, Log, All);
