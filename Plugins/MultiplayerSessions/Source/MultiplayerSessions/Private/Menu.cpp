// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

// Log category for the subsystem already defined in subsys, redefine create ambi

//
// ==================== Menu Setup ====================
//

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath); // ?listen makes the server wait for players
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	// setup the menu to be the focus of the viewport
	UWorld* World = GetWorld();
	if (World)
	{	// get the first player controller in the world (always the local player)
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData; // struct that holds data for setting input mode to UI only
			InputModeData.SetWidgetToFocus(TakeWidget()); // set the widget to focus to this menu widget
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // do not lock the mouse to the viewport
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	// get our custom MultiplayerSessionsSubsystem, from the game instance
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{	// 
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

// init is called after the underlying slate widget constructed, early stage
bool UMenu::Initialize()
{
	if (!Super::Initialize()) // if super failed for whatever reason
	{
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("Menu: Super::Initialize() failed!"))
		return false;
	}

	/*
	// callbacks for the buttons
	if (HostButton)
	{ // adding dynamic delegate, bind the button click event to the callback function
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}
	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}
	*/

	return true;
}

void UMenu::NativeDestruct()
{
	UE_LOG(MP_SessionsSubsys, Warning, TEXT("Menu: NativeDestruct()"));
	MenuTearDown();
	Super::NativeDestruct();
}


//
// ==================== Callbacks for the delegates from the subsystem ====================
//

// Session was attempted to be created in the subsystem, delegate brought back here
void UMenu::OnCreateSession(bool bWasSuccessful)
{
	UE_LOG(MP_SessionsSubsys, Warning, TEXT("OnCreateSession: bWasSuccessful = %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));

	if (GetWorld()->GetNetMode() == NM_Client)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Purple,
			FString(TEXT("OnCreateSession: refusing ServerTravel because world is NM_Client!"))
		);
		UE_LOG(MP_SessionsSubsys, Error, TEXT("OnCreateSession: refusing ServerTravel because world is NM_Client!"));
		return;
	}

	if (bWasSuccessful)
	{
		UWorld* World = GetWorld();
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("OnCreateSession: Traveling to %s"), *PathToLobby);
		if (World)
		{
			World->ServerTravel(PathToLobby);
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString(TEXT("OnCreateSession: Failed to create session!"))
			);
		}
		HostButton->SetIsEnabled(true);
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	if (MultiplayerSessionsSubsystem == nullptr)
	{
		return;
	}

	//TODO: rearange this for loop into the if statement below, since if the search failed or no results, no need to loop
	for (auto Result : SessionResults)
	{
		FString SettingsValue;
		Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue); // fetch the match type from each session
		if (SettingsValue == MatchType)
		{
			// Forcing bUseLobbiesIfAvailable to be true, since there's a bug somewhere that flips it false after creating session.
			// Remove this later when the bug is resolved.
			Result.Session.SessionSettings.bUseLobbiesIfAvailable = true;


			UE_LOG(MP_SessionsSubsys, Warning, TEXT("OnFindSessions: Found session with matching MatchType: %s"), *SettingsValue);
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
		else
		{
			UE_LOG(MP_SessionsSubsys, Warning, TEXT("OnFindSessions: Skipping session, MatchType: %s"), *SettingsValue);
		}
	}
	
	if (!bWasSuccessful || SessionResults.Num() == 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1, 10.f, FColor::Red,
				FString(TEXT("OnFindSessions: Finding failed!"))
				);
		}
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("OnFindSessions: Finding failed! bWasSuccessful - %s, Matched sessions - %d"), bWasSuccessful ? TEXT("true") : TEXT("false"), SessionResults.Num())
		JoinButton->SetIsEnabled(true);
	}
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{	// the using online subsystem is determined in the .ini file
	// Anchor: I have no idea these update of API, where the fuck are your doc epic 
	
	/* 
	 * IOnlineSubsystem::Get() is now legacy? its even got erased out of the doc entirely 
	 * issue was:
	 * Global Access: Returns the default online subsystem globally, not tied to any specific world or context.
	 * Not Context-Aware: Can cause issues in multi-world scenarios (e.g., PIE, multiplayer testing)-
	 * -because it doesn't know which world or game instance you mean.
	 */
	// this might be the issue of why PIE and LAN was either not working or weird af
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	/*
	 * This might be better? I got the info from AI, cannot find it in the doc
	 * Context-Aware: requires a world context, returns the subsystem for specific world/game instance.
	 * Safer for Multiplayer: Ensure getting the right subsystem in multi instance scenarios (PIE, dedicated servers).
	 */
	// This made absolutely no difference afaik 3/10/25
	// IOnlineSubsystem* Subsystem = Online::GetSubsystem(GEngine->GetWorld());
	
	if (Subsystem)
	{	// GetSessionInterface() provides Session functions
		// IOnlineSessionPtr is alias for TSharedPtr<IOnlineSession, ESPMode::ThreadSafe>
		// Multithread related, parallel lines of code processing simultaneously
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

		if (SessionInterface.IsValid())
		{
			FString Address;
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			// GEngine->AddOnScreenDebugMessage(
			// 	-1,
			// 	15.f,
			// 	FColor::Green,
			// 	FString::Printf(TEXT("SubSys up") )
			// );

			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController)
			{
				PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
			}
		}
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
}


//
// ==================== Button Callbacks ====================
//


void UMenu::HostButtonClicked()
{
	HostButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::JoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(10000);
	}
}

// return controls to the player character, remove the menu from the viewport
void UMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
