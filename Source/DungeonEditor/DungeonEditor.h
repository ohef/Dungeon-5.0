// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"

// DECLARE_LOG_CATEGORY_EXTERN(DungeonEditor, All, All);

class FDungeonEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;
};

//////////////