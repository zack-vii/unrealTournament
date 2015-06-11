// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTServerBeaconLobbyClient.h"
#include "Net/UnrealNetwork.h"
#include "UTLobbyGameState.h"

AUTServerBeaconLobbyClient::AUTServerBeaconLobbyClient(const class FObjectInitializer& PCIP) :
Super(PCIP)
{
}

void AUTServerBeaconLobbyClient::InitLobbyBeacon(FURL LobbyURL, uint32 LobbyInstanceID, FGuid InstanceGUID, FString AccessKey)
{
	InitClient(LobbyURL);
	GameInstanceID = LobbyInstanceID;
	GameInstanceGUID = InstanceGUID;
	HubKey = AccessKey;
	bDedicatedInstance = !AccessKey.IsEmpty();
}

void AUTServerBeaconLobbyClient::OnConnected()
{
	UE_LOG(UT,Verbose,TEXT("Instance %i [%s] has Connected to the hub (%i)"), GameInstanceID, *GameInstanceGUID.ToString(),bDedicatedInstance);

	if (bDedicatedInstance)
	{
		UE_LOG(UT,Verbose,TEXT("Becoming a dedicated instances"));
		AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
		Lobby_IsDedicatedInstance(GameInstanceGUID, HubKey, GameState ? GameState->ServerName : TEXT("My Instance"));
	}

	AUTGameMode* UTGameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (UTGameMode)
	{
		Lobby_PrimeMapList(UTGameMode->GetMapPrefix());
	}


}

void AUTServerBeaconLobbyClient::OnFailure()
{
	UE_LOG(UT,Log,TEXT("Instance %i [%s] could not connect to the hub"), GameInstanceID, *GameInstanceGUID.ToString());
}


void AUTServerBeaconLobbyClient::UpdateMatch(FString Update)
{
	UE_LOG(UT,Verbose,TEXT("UpdateMatch: Instance %i [%s] Update = %s"), GameInstanceID, *GameInstanceGUID.ToString(), *Update);
	Lobby_UpdateMatch(GameInstanceID, Update);
}

void AUTServerBeaconLobbyClient::UpdatePlayer(FUniqueNetIdRepl PlayerID, const FString& PlayerName, int32 PlayerScore)
{
	UE_LOG(UT,Verbose,TEXT("UpdatePlayer: Instance %i [%s] Player = %s [%s] Score = %i"), GameInstanceID, *GameInstanceGUID.ToString(), *PlayerName, *PlayerID.ToString(),  PlayerScore);
	Lobby_UpdatePlayer(GameInstanceID, PlayerID, PlayerName, PlayerScore);
}

void AUTServerBeaconLobbyClient::EndGame(FString FinalUpdate)
{
	UE_LOG(UT,Verbose,TEXT("EndGame: Instance %i [%s] Update = %s"), GameInstanceID, *GameInstanceGUID.ToString(), *FinalUpdate);
	Lobby_EndGame(GameInstanceID, FinalUpdate);
}

void AUTServerBeaconLobbyClient::Empty()
{
	UE_LOG(UT,Verbose,TEXT("Empty: Instance %i [%s] %i"), GameInstanceID, *GameInstanceGUID.ToString(), bInstancePendingKill);

	if (!bInstancePendingKill)
	{
		bInstancePendingKill = true;
		Lobby_InstanceEmpty(GameInstanceID);
	}
}


bool AUTServerBeaconLobbyClient::Lobby_NotifyInstanceIsReady_Validate(uint32 InstanceID, FGuid InstanceGUID) { return true; }
void AUTServerBeaconLobbyClient::Lobby_NotifyInstanceIsReady_Implementation(uint32 InstanceID, FGuid InstanceGUID)
{
	UE_LOG(UT,Verbose,TEXT("[HUB] NotifyInstanceIsReady: Instance %i [%s]"), InstanceID, *InstanceGUID.ToString());
	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		LobbyGameState->GameInstance_Ready(InstanceID, InstanceGUID);
		Instance_ReceiveHubID(LobbyGameState->HubGuid);
	}


}

bool AUTServerBeaconLobbyClient::Lobby_UpdateMatch_Validate(uint32 InstanceID, const FString& Update) { return true; }
void AUTServerBeaconLobbyClient::Lobby_UpdateMatch_Implementation(uint32 InstanceID, const FString& Update)
{
	UE_LOG(UT,Verbose,TEXT("[HUB] UpdateMatch: Instance %i Update = %s"), InstanceID, *Update);
	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		LobbyGameState->GameInstance_MatchUpdate(InstanceID, Update);
	}
}


bool AUTServerBeaconLobbyClient::Lobby_UpdatePlayer_Validate(uint32 InstanceID, FUniqueNetIdRepl PlayerID, const FString& PlayerName, int32 PlayerScore) { return true; }
void AUTServerBeaconLobbyClient::Lobby_UpdatePlayer_Implementation(uint32 InstanceID, FUniqueNetIdRepl PlayerID, const FString& PlayerName, int32 PlayerScore)
{
	UE_LOG(UT,Verbose,TEXT("[HUB] UpdatePlayer: Instance %i PlayerName = %s [%s] Score = %i"), InstanceID, *PlayerName, *PlayerID.ToString(), PlayerScore);
	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		LobbyGameState->GameInstance_PlayerUpdate(InstanceID, PlayerID, PlayerName, PlayerScore);
	}
}



bool AUTServerBeaconLobbyClient::Lobby_EndGame_Validate(uint32 InstanceID, const FString& FinalDescription) { return true; }
void AUTServerBeaconLobbyClient::Lobby_EndGame_Implementation(uint32 InstanceID, const FString& FinalDescription)
{
	UE_LOG(UT,Verbose,TEXT("[HUB] EndGame: Instance %i FinalUpdate = %s"), InstanceID, *FinalDescription);
	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		LobbyGameState->GameInstance_EndGame(InstanceID,FinalDescription);
	}
}

bool AUTServerBeaconLobbyClient::Lobby_InstanceEmpty_Validate(uint32 InstanceID) { return true; }
void AUTServerBeaconLobbyClient::Lobby_InstanceEmpty_Implementation(uint32 InstanceID)
{
	UE_LOG(UT,Verbose,TEXT("[HUB] Empty: Instance %i"), InstanceID);
	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		LobbyGameState->GameInstance_Empty(InstanceID);
	}

}


bool AUTServerBeaconLobbyClient::Lobby_UpdateBadge_Validate(uint32 InstanceID, const FString& Update) { return true; }
void AUTServerBeaconLobbyClient::Lobby_UpdateBadge_Implementation(uint32 InstanceID, const FString& Update)
{
	UE_LOG(UT,Verbose,TEXT("[HUB] UpdateBadge: Instance %i Update = %s"), InstanceID, *Update);
	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		LobbyGameState->GameInstance_MatchBadgeUpdate(InstanceID, Update);
	}
}

bool AUTServerBeaconLobbyClient::Lobby_RequestNextMap_Validate(uint32 InstanceID, const FString& CurrentMap) { return true; }
void AUTServerBeaconLobbyClient::Lobby_RequestNextMap_Implementation(uint32 InstanceID, const FString& CurrentMap)
{
	UE_LOG(UT,Verbose,TEXT("[HUB] RequestNextMap: Instance %i CurrentMap = %s"), InstanceID, *CurrentMap);
	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		LobbyGameState->GameInstance_RequestNextMap(this, InstanceID, CurrentMap);
	}
}


void AUTServerBeaconLobbyClient::InstanceNextMap_Implementation(const FString& NextMap)
{
	UE_LOG(UT,Verbose,TEXT("NextMap: Instance %i [%s] NextMap = %s"), GameInstanceID, *GameInstanceGUID.ToString(), *NextMap);
	AUTGameMode* CurrentGameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (CurrentGameMode)
	{
		CurrentGameMode->InstanceNextMap(NextMap);	
	}
}

bool AUTServerBeaconLobbyClient::Lobby_IsDedicatedInstance_Validate(FGuid InstanceGUID, const FString& InHubKey, const FString& ServerName) { return true; }
void AUTServerBeaconLobbyClient::Lobby_IsDedicatedInstance_Implementation(FGuid InstanceGUID, const FString& InHubKey, const FString& ServerName)
{
	UE_LOG(UT, Verbose, TEXT("Dedicated Instance (%s) requesting authorization with key %s"), *ServerName, *InHubKey);
	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		LobbyGameState->AuthorizeDedicatedInstance(this, InstanceGUID, InHubKey, ServerName);
	}
}

void AUTServerBeaconLobbyClient::AuthorizeDedicatedInstance_Implementation(FGuid HubGuid)
{
	UE_LOG(UT, Verbose, TEXT("This server has been authorized as a dedicated instance!"));
	AUTGameMode* CurrentGameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (CurrentGameMode)
	{
		CurrentGameMode->BecomeDedicatedInstance(HubGuid);
	}
}

bool AUTServerBeaconLobbyClient::Lobby_PrimeMapList_Validate(const FString& MapPrefix) { return true; }
void AUTServerBeaconLobbyClient::Lobby_PrimeMapList_Implementation(const FString& MapPrefix)
{
	UE_LOG(UT,Verbose, TEXT("Lobby_PrimeMapList - Searching for maps [%s]"),*MapPrefix);
	AUTLobbyGameState* LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		// Parse the prefixes 
		TArray<FString> Prefixes;
		MapPrefix.ParseIntoArray(Prefixes,TEXT(","), true);
		
		// Grab the allowed maps
		
		AllowedMaps.Empty();
		LobbyGameState->GetAvailableMaps(Prefixes, AllowedMaps);

		UE_LOG(UT,Verbose, TEXT("Lobby_PrimeMapList - GameState returned %i maps for %i prefixes"),AllowedMaps.Num(), Prefixes.Num());

		if (AllowedMaps.Num() > 0)
		{
			Instance_ReceiveMap(AllowedMaps[0]->PackageName, AllowedMaps[0]->Title, AllowedMaps[0]->Screenshot,0);
		}
	}
}

void AUTServerBeaconLobbyClient::Instance_ReceiveMap_Implementation(const FString& MapPackageName, const FString& MapTitle, const FString& MapScreenshotReference, int32 Index)
{
	AUTGameState* UTGameState = GetWorld()->GetGameState<AUTGameState>();
	if (UTGameState)
	{
		UTGameState->CreateMapVoteInfo(MapPackageName, MapTitle, MapScreenshotReference);
	}

	Lobby_SendNextMap(Index);
}

bool AUTServerBeaconLobbyClient::Lobby_SendNextMap_Validate(int32 LastIndex) { return true; }
void AUTServerBeaconLobbyClient::Lobby_SendNextMap_Implementation(int32 LastIndex)
{
	LastIndex++;
	if (AllowedMaps.Num() > LastIndex)
	{
		Instance_ReceiveMap(AllowedMaps[LastIndex]->PackageName, AllowedMaps[LastIndex]->Title, AllowedMaps[LastIndex]->Screenshot, LastIndex);
	}
}

void AUTServerBeaconLobbyClient::Instance_ReceiveHubID_Implementation(FGuid HubGuid)
{
	UE_LOG(UT,Verbose,TEXT("Instance has received Hub GUID: %s"), *HubGuid.ToString());
	AUTGameState* UTGameState = GetWorld()->GetGameState<AUTGameState>();
	if (UTGameState)
	{
		UTGameState->HubGuid = HubGuid;
	}
}
