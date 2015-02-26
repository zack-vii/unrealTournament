// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTWeaponAttachment.h"

#include "UTWeapAttachment_BioRifle.generated.h"

UCLASS(Blueprintable, NotPlaceable, Abstract)
class AUTWeapAttachment_BioRifle : public AUTWeaponAttachment
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent)
	void SetChargingEffect(bool bNowActive);

	virtual void PlayFiringEffects() override
	{
		// odd FlashCount is charging, even is firing
		// FIXME: rework this, no need for so much dependence on weapon sequence
		if (UTOwner->FireMode != 1 || (UTOwner->FlashCount % 2) == 0)
		{
			SetChargingEffect(false);
			Super::PlayFiringEffects();
		}
		else
		{
			StopFiringEffects(true);
			SetChargingEffect(true);
		}
	}
};