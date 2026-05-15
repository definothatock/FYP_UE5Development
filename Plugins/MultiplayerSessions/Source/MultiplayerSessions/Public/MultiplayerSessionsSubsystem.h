// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h" // this header includes the delegate headers (FOnCreateSessionCompleteDelegate, etc)

#include "MultiplayerSessionsSubsystem.generated.h" // any other includes must be before the generated.h



//
// Declaring custom delegates *type* for the Menu class to bind callbacks to
// DYNAMIC_MULTICAST: delegate can be serialized and used in blueprints ; multiple classes can bind their callbacks to this delegate
// In BP they are called Event Dispatchers
// Some cannot be dynamic, For example, <FOnlineSessionSearchResult> is not a UCLASS or USTRUCT, so cannot be used in dynamic delegate
//
// The provided delegates from the #include (FOnCreateSessionCompleteDelegate) did the same thing in their .h
//
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);

// Log category for this subsystem
DECLARE_LOG_CATEGORY_EXTERN(MP_SessionsSubsys, Warning, All);

/*
 * This implementation follows the old OSSv1 method. Its works as of 5.6 (made in Sep 2025).
 * UE is pushing devs to use UE::Online::IOnlineServices, but this requires me to rewrite the entire system.
 * For example, Async calls do not use delegates anymore, new system uses Async/Await & Futures (no idea what they are)
 * I do not think that is necessary, nor it is wise to do so (no tutorials). So, I will keep this as is.
 *  - Ad, Dec 2025
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMultiplayerSessionsSubsystem();

	//
	// These are the actual top-most function to manage sessions. They handle session functionality.
	// The Menu class will call these. These functions are like the 'button' that make thing works in one go.
	//
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	void FindSessions(int32 MaxSearchResults); // MaxSearchResults is kinda for the Steam dev game tag 480, so 10000 is more than enough
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();
	bool IsValidSessionInterface();

	//
	// Our own custom delegates for the Menu class to bind callbacks to.
	// Unlike the internal delegates, these for binding to other classes, so they need to be public.
	// Binding between OnCreateSession() (from Menu.cpp) and MultiplayerOnCreateSessionComplete (here) is done in Menu.cpp
	//
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionsComplete;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

protected:
	
	//
	// Callback funcs (internal) that will be linked to the OnlineSession Interface delegate list.
	// Should not allow uses outside the system
	// these funcs need specific signatures (return type and parameters) to be bound to the delegate (unreal macros requirements)
	// the delegates will feed in the retrieved data into those callbacks
	//
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:

	// 
	// Session interface and related settings, no need to be exposed.
	// All of these are promoted to member variables, because the callbacks need them,-
	// -and some returned data need to be accessed in the callback funcs (like LastSessionSearch)
	//
	// the rest does not have alias TSharedPtr, so need to use the full name
	// shared pointer is sth related to multithreading and memory management (honestly no idea wth this is)
	//
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings; 
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch; 

	//
	// These are the delegates and their handles.
	// Delegates: binding the 
	// Delegate are like Signal in Godots. (or boardcast in more general term)
	// internal callbacks (right above this section) will be linked to these delegate.
	//
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	bool bCreateSessionOnDestroy{ false };
	int32 LastNumPublicConnections;
	FString LastMatchType;
};
