// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

// Log category for the subsystem already defined in subsys, redefine create ambi

/**
 * This Menu (UUserWidget) is very basic. If we are going to build a much integrated menu screen, this class should be replaced.
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString LobbyPath = FString(TEXT("/Game/ThirdPersonCPP/Maps/Lobby")));

protected:

    virtual bool Initialize() override;
    virtual void NativeDestruct() override;

    //
    // Callback funcs that will be linked to the delegates of the MultiplayerSessionsSubsystem
    //

    UFUNCTION()
    void OnCreateSession(bool bWasSuccessful);
    void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
    void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
    UFUNCTION()
    void OnDestroySession(bool bWasSuccessful);
    UFUNCTION()
    void OnStartSession(bool bWasSuccessful);
    
    // Callbacks for the buttons (Now callable from Blueprint)
    UFUNCTION(BlueprintCallable)
    void HostButtonClicked();

    UFUNCTION(BlueprintCallable)
    void JoinButtonClicked();
    

private:

    UPROPERTY(meta = (BindWidget))
    class UButton* HostButton;
    UPROPERTY(meta = (BindWidget))
    UButton* JoinButton;

    void MenuTearDown();

    class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

    int32 NumPublicConnections{4};
    FString MatchType{TEXT("FreeForAll")};
    FString PathToLobby{TEXT("")};
};
