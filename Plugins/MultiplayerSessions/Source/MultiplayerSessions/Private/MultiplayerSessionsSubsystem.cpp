// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "Logging/LogMacros.h" // this is for DEFINE_LOG_CATEGORY, custom log category
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"


DEFINE_LOG_CATEGORY(MP_SessionsSubsys); // define the log category declared in the header file

//
/*	Overall Logic of the Multiplayer Session Subsystem:
 *
*/
//

// Constructor: init (construct in construct) all delegates
// These delegates can be constructed using a static func that exist on The delegate class (from engine class online session interface)
// CreateUObject(): prepare a cross-class delegate that is bound to a UObject, to a specific callback function
// 
// in human language:
// to where? which function?
// “Build an instance of FOnCreateSessionCompleteDelegate that is permanently bound to this object’s OnCreateSessionComplete method.”
// equivalent to “subscribe my OnCreateSessionComplete function so that, whenever the Online Session system says ‘session creation finished’, call me back here.”
//
// the calling side of the delegate is in the CreateSession() function, when we add the delegate to the interface delegate list
UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{}

//
// ==================== Session Controls ====================
//

// Create a session. If a session already exists, destroy it first.
void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	UE_LOG(MP_SessionsSubsys, Warning, TEXT("CreateSession: Attempting to create session"));
	if (!IsValidSessionInterface())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1, 10.f, FColor::Red,
				FString(TEXT("CreateSession: SessionInterface NOT valid!"))
				);
		}
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("CreateSession: SessionInterface NOT valid!"))
		return;
	}

	// Search for the existing session with the global var NAME_GameSession (defined in OnlineSessionNames.h). If found, Destroy.
	// NAME_GameSession is the engine default name of the active session, there should only be one active session at a time
	// Tutorial used auto for whatever reason, I think if not using auto, it is FNamedOnlineSession*
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	UE_LOG(MP_SessionsSubsys, Warning, TEXT("CreateSession: ExistingSession is %s"), ExistingSession == nullptr ? TEXT("null") : TEXT("not null"));
	if (ExistingSession != nullptr)
	{
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("CreateSession: Found existing session at creation. Destroying..."))
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;
		
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("CreateSession: About to call DestroySession"));
		DestroySession();

		// IMPORTANT: wait for OnDestroySessionComplete to call CreateSession again
		return;
	}
	UE_LOG(MP_SessionsSubsys, Warning, TEXT("CreateSession: Creating session now"));

	// First, add the delegate to the interface delegate list (through AddOnCreateSessionCompleteDelegate_Handle).
	// Then, store the delegate in a custom FDelegateHandle (CreateSessionCompleteDelegateHandle) so we can later remove it from our delegate list.
	// Side note: Once the Session is created, the delegate (CreateSessionCompleteDelegate) will call the linked callback func (OnCreateSessionComplete). (link was done in the constructor)
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	// The online session overall settings 
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings()); // MakeShareable warps the raw pointer into the shared pointer
	LastSessionSettings->bIsLANMatch = Online::GetSubsystem(GetWorld())->GetSubsystemName() == "NULL" ? true : false; // if using the NULL subsystem (local), then it is a LAN match
	LastSessionSettings->NumPublicConnections = NumPublicConnections; // iirc I set this in the .ini file (right?)
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true; // related to steam service, it searches the session going on the same region as the user (I think so?)
	LastSessionSettings->bShouldAdvertise = true; // allow steam to broadcast the session for others to find
	LastSessionSettings->bUsesPresence = true; // allow user to find steam session that is in the same region
	LastSessionSettings->bUseLobbiesIfAvailable = true; // this is for steam, use steam lobby if available
	// set key-value pair associated with the session, it checks the match type when finding sessions
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->BuildUniqueId = 1;


	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	UE_LOG(MP_SessionsSubsys, Warning, TEXT("CreateSession: LocalPlayer is %s"), LocalPlayer == nullptr ? TEXT("null") : TEXT("not null"));
	if (LocalPlayer) 
	{
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("CreateSession: Attempting to create session, PlayerId = %s, SessionName = %d"), *LocalPlayer->GetPreferredUniqueNetId()->ToString(), NAME_GameSession);
	}
		
	// Attempt to create the session: first local player will be the host (controller), the session name, and the session settings
	// If it fails, clear the delegate we added above (since it won't be called)
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		// Broadcast our own custom delegate with false, since the session creation did not even start
		MultiplayerOnCreateSessionComplete.Broadcast(false);
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("CreateSession: Failed to create session from SessionInterface, Broadcasting FALSE to Menu.cpp"));
	}
	// after here, callback func OnCreateSessionComplete() will be called from the delegate.
}



void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!IsValidSessionInterface())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1, 10.f, FColor::Red,
				FString(TEXT("FindSessions: SessionInterface NOT valid!"))
				);
		}
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("FindSessions: SessionInterface NOT valid!"))
		return;
	}

	// Same in the CreateSession(), the process of managing delegates
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	// Some Session Search settings
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = Online::GetSubsystem(GetWorld())->GetSubsystemName() == "NULL" ? true : false; // same logic as in CreateSession()
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals); // There are different keys.

	// slight different logic from CreateSession(), we get the first local player for specifying who is searching (filtering or personalized search?)
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}



void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
	
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}



void UMultiplayerSessionsSubsystem::DestroySession()
{
	UE_LOG(MP_SessionsSubsys, Warning, TEXT("DestroySession: got into DestroySession"));
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1, 15.f, FColor::Red,
				FString(TEXT("DestroySession: Failed! Attempted to destroy non-existing SessionInterface!"))
				);
		}
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("DestroySession: Failed! Attempted to destroy non-existing SessionInterface!"))
		return;
	}

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	// attempt to destroy the session, and if it fails, 
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("DestroySession: Failed to destroy session from SessionInterface!"))
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}



void UMultiplayerSessionsSubsystem::StartSession()
{
}



bool UMultiplayerSessionsSubsystem::IsValidSessionInterface()
{
	if (!SessionInterface) // if the interface is not valid, try to get it from the online subsystem
	{
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("IsValidSessionInterface: SessionInterface is NULL, attempting to get it from OnlineSubsystem"));
		IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
		if (Subsystem)
		{
			SessionInterface = Subsystem->GetSessionInterface();
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1, 15.f, FColor::Green,
					FString::Printf(TEXT("IsValidSessionInterface: Found Online Subsystem - %s (NULL for LAN)"), *Subsystem->GetSubsystemName().ToString())
					);
			}
			UE_LOG(MP_SessionsSubsys, Warning, TEXT("IsValidSessionInterface: Found Online Subsystem - %s (NULL for LAN)"), *Subsystem->GetSubsystemName().ToString())
		}
		else
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1, 15.f, FColor::Red,
					FString(TEXT("IsValidSessionInterface: No Online Subsystem found!"))
					);
			}
			UE_LOG(MP_SessionsSubsys, Warning, TEXT("IsValidSessionInterface: No Online Subsystem found!"))
			return false;
		}
	}
	return SessionInterface.IsValid();
}

//
// ==================== Callback Functions ====================
//

// Callback func: Once the CreateSession() is Done, the delegate will redirect the thread to here
// 
void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1, 10.f, FColor::Green,
				FString::Printf(TEXT("OnCreateSessionComplete: Session created - %s ; bWasSuccessful - %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"))
				);
		}
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("OnCreateSessionComplete: Session created - %s ; bWasSuccessful - %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"))
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1, 10.f, FColor::Red,
				FString::Printf(TEXT("OnCreateSessionComplete: Failed to create session - %s ; bWasSuccessful - %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"))
				);
		}
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("OnCreateSessionComplete: Failed to create session - %s ; bWasSuccessful - %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"))
	}
	
	if (SessionInterface)
	{	// clear the delegate from the interface delegate list, since we are done with it
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}



void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		// Debug: print all the found sessions
		for (auto Result : LastSessionSearch->SearchResults)
		{
			FString Id = Result.GetSessionIdStr();
			FString User = Result.Session.OwningUserName;
			FString MatchType;
			Result.Session.SessionSettings.Get(FName("MatchType"), MatchType);

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1, 10.f, FColor::Cyan,
					FString::Printf(TEXT("OnFindSessionsComplete: Found session: %s by %s, MatchType = %s"), *Id, *User, *MatchType)
					);
			}
			UE_LOG(MP_SessionsSubsys, Warning, TEXT("OnFindSessionsComplete: Found session: %s by %s, MatchType = %s"), *Id, *User, *MatchType)
		}
	}


	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	// If no search results, or the search itself failed, just return an empty array
	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		UE_LOG(MP_SessionsSubsys, Warning, TEXT("OnFindSessionsComplete: No sessions found, Broadcasting empty array with false"))
		return;
	}

	MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}



void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}



void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType);
	}
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}



void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}
