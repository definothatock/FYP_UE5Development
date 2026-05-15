// Fill out your copyright notice in the Description page of Project Settings.
// Study work by Chiu Chi Keung 20983923

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MP_LANMenu.generated.h"

// Forward declarations, just using as a pointer, no need to include the full header
class UEditableTextBox;
class UButton;


UCLASS()
class UST_FYP_API UMP_LANMenu : public UUserWidget
{
	GENERATED_BODY()

public:

	/*
	 * Binding on early stage of widget life cycle
	*/

	virtual void NativeOnInitialized() override; // Override the native initialization function of UUserWidget

private:

	/*
	 * UPROPERTY is recognition tag for reflection system, generating metadata for dynamically used in BP and other ue systems
	 * 
	 * TObjectPtr is a smart pointer (Template-class/wrapper) for UObject types, designed for ue Garbage Collection.
	 * Raw pointers can still be used, but it will not be in the reflection system, worse case GC destroy the Object and leave a dangling pointer.
	 * Soft pointer is more of a reference than a pointer, it stores the path of the object, and load it when needed (can be null if the object is not loaded).
	*/

	UPROPERTY(meta = (BindWidget)) // BindWidget will bind this variable to a widget in the UMG editor with the same name
	TObjectPtr<UEditableTextBox> TextBox_IPAddress;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Host; // UButton has built-in dynamic multicast delegate (similar to Signal in Godot) "OnClicked", all listeners (callbacks) will be called when the button is clicked

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Join;

	UPROPERTY(EditDefaultsOnly) // EditDefaultsOnly: can only be edited in Unreal Editor Defaults panel (not at runtime or on instances of the object)
	TSoftObjectPtr<UWorld> HostingLevel;

	/* 
	 * Callback function for OnClicked events
	*/

	UFUNCTION() // UFUNCTION is reconition tag for reflection system, generating metadata for dynamically used in BP and other ue systems (in this case binding with button events)
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();
};
