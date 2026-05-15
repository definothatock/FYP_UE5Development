// Fill out your copyright notice in the Description page of Project Settings.
// Study work by Chiu Chi Keung 20983923

#include "MP/LAN/MP_LANMenu.h"

// forward included in .h
#include "Components/Button.h"
#include "Components/EditableTextBox.h"

#include "Kismet/GameplayStatics.h"


void UMP_LANMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Set input mode to UI only
	FInputModeUIOnly InputModeData;
	GetOwningPlayer()->SetInputMode(InputModeData); // prevent player from controlling the character when in menu
	GetOwningPlayer()->SetShowMouseCursor(true); // show mouse cursor

	// Link the button click events to the callback functions (from this specific user widget Object)
	Button_Host->OnClicked.AddDynamic(this, &UMP_LANMenu::HostButtonClicked);
	Button_Join->OnClicked.AddDynamic(this, &UMP_LANMenu::JoinButtonClicked);

	
}

// Create a host session (listening server)
void UMP_LANMenu::HostButtonClicked()
{
	// revert input mode to game only
	FInputModeGameOnly InputModeData;
	GetOwningPlayer()->SetInputMode(InputModeData);
	GetOwningPlayer()->SetShowMouseCursor(false);

	// Open the hosting level as a listening server
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, HostingLevel, true, TEXT("listen"));

}

// Get the IP address from the text box and join the session
void UMP_LANMenu::JoinButtonClicked()
{
	// revert input mode to game only
	FInputModeGameOnly InputModeData;
	GetOwningPlayer()->SetInputMode(InputModeData);
	GetOwningPlayer()->SetShowMouseCursor(false);

	// Get the address from the text box, and open the level as a client
	// OpenLevel 2nd arg is FName, need to dereference FString to TCHAR, the implicitly convert to FName // yeah wtf
	const FString Address = TextBox_IPAddress->GetText().ToString();
	UGameplayStatics::OpenLevel(this, *Address, true, TEXT("listen"));
}
