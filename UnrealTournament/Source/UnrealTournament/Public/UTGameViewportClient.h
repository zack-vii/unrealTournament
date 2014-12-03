// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "UTGameViewportClient.generated.h"

UCLASS()
class UUTGameViewportClient : public UGameViewportClient
{
	GENERATED_UCLASS_BODY()


	virtual void PeekTravelFailureMessages(UWorld* World, enum ETravelFailure::Type FailureType, const FString& ErrorString) override;
	virtual void PeekNetworkFailureMessages(UWorld *World, UNetDriver *NetDriver, enum ENetworkFailure::Type FailureType, const FString& ErrorString) override;

protected:

	TSharedPtr<class SUWDialog> ReconnectDialog;
	TSharedPtr<class SUWRedirectDialog> RedirectDialog;

	// Holds the IP/Port of the last connect so we can try to reconnect
	FURL LastAttemptedURL;

	virtual void NetworkFailureDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);
	virtual void ConnectPasswordResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);
	virtual void RedirectResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);
};

