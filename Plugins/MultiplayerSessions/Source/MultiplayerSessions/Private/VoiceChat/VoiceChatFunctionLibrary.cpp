// Some are taken or rewritten from VR Expansion Plugin and Epic Online Services Integration Kit.
// Since Those are under MIT license, this Library will also be.

#include "VoiceChat/VoiceChatFunctionLibrary.h"

int MAX_PLAYERS = 32; // this should either set in .ini or my MP Menu.cpp, now just random big number for sanity check

DEFINE_LOG_CATEGORY(MP_VoiceChat);


/*
 * ==================== Internal Helper ====================
 *  Mostly static because used only in this cpp
 */


// Voice Interface safety check
static inline IOnlineVoicePtr GetVoiceInterface(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!IsValid(World))
	{
		UE_LOG(MP_VoiceChat, Warning, TEXT("GetVoiceInterface: Could not get World from WorldContextObject."));
		return nullptr;
	}

	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(World);
	if (!Subsystem)
	{
		UE_LOG(MP_VoiceChat, Warning, TEXT("GetVoiceInterface: Could not get Active Online Subsystem."));
		return nullptr;
	}
	
	// Note: Not all subsystems support voice. This is a common failure point.
	IOnlineVoicePtr VoiceInterface = Subsystem->GetVoiceInterface();
	if (!VoiceInterface.IsValid())
	{
		UE_LOG(MP_VoiceChat, Warning, TEXT("GetVoiceInterface: No Voice Interface available via subsystem '%s'"), *Subsystem->GetSubsystemName().ToString());
		return nullptr;
	}

	return VoiceInterface;
}


// Extract the UniqueNetId from a PlayerState safely
static TSharedPtr<const FUniqueNetId> GetUniqueIdFromPlayerState(const APlayerState* PlayerState)
{
	if (!IsValid(PlayerState))
	{
		UE_LOG(MP_VoiceChat, Warning, TEXT("GetUniqueIdFromPlayerState: Invalid PlayerState passed."));
		return nullptr;
	}

	// GetUniqueId() returns FUniqueNetIdRepl (a type of TSharedPtr).
	return PlayerState->GetUniqueId().GetUniqueNetId();
}


/*
 * ==================== Voice Chat Handle (VOIP) ====================
 * Mostly from the VR Plugin, but it was written for OSSv1, UE is phasing that out
 * rewritten for OSSv2
 */


void UVoiceChatFunctionLibrary::StartNetworkedVoice(UObject* WorldContextObject, uint8 LocalPlayerNum)
{
	IOnlineVoicePtr VoiceInterface = GetVoiceInterface(WorldContextObject);
	if (!VoiceInterface.IsValid()) { return; }
	
	if (LocalPlayerNum > MAX_PLAYERS)
	{
		UE_LOG(MP_VoiceChat, Warning, TEXT("StartNetworkedVoice: Invalid LocalPlayerNum %d, MAX_PLAYERS = %d"), LocalPlayerNum, MAX_PLAYERS);
		return;
	}

	VoiceInterface->StartNetworkedVoice(LocalPlayerNum);
}


void UVoiceChatFunctionLibrary::StopNetworkedVoice(UObject* WorldContextObject, uint8 LocalPlayerNum)
{
	IOnlineVoicePtr VoiceInterface = GetVoiceInterface(WorldContextObject);
	if (!VoiceInterface.IsValid()) { return; }

	VoiceInterface->StopNetworkedVoice(LocalPlayerNum);
}

// Largely skipped using other wrapper func in this file, need the context anyways
void UVoiceChatFunctionLibrary::RemoveAllRemoteTalkers(UObject* WorldContextObject)
{
	IOnlineVoicePtr VoiceInterface = GetVoiceInterface(WorldContextObject);
	if (!VoiceInterface.IsValid()) { return; }

	VoiceInterface->RemoveAllRemoteTalkers();
	UE_LOG(MP_VoiceChat, Warning, TEXT("RemoveAllRemoteTalkers: Successfully executed."));
	// UE_LOG(MP_VoiceChat, Warning, TEXT("RemoveAllRemoteTalkers: Successfully executed for subsystem '%s'."), *Subsystem->GetSubsystemName().ToString());
}

void UVoiceChatFunctionLibrary::RegisterRemoteTalker(UObject* WorldContextObject, APlayerState* TargetPlayerState)
{
	IOnlineVoicePtr VoiceInterface = GetVoiceInterface(WorldContextObject);
	if (!VoiceInterface.IsValid()) { return; }

	TSharedPtr<const FUniqueNetId> UniqueId = GetUniqueIdFromPlayerState(TargetPlayerState);
	if (!UniqueId.IsValid())
	{ UE_LOG(MP_VoiceChat, Warning, TEXT("RegisterRemoteTalker: Invalid UniqueId.")); }
	
	VoiceInterface->RegisterRemoteTalker(*UniqueId);
	UE_LOG(MP_VoiceChat, Warning, TEXT("RegisterRemoteTalker: Register Remote Talker for %s."), *UniqueId->ToString());
}


void UVoiceChatFunctionLibrary::DestroyTransientVoipComponents(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		UE_LOG(MP_VoiceChat, Warning, TEXT("DestroyTransientVoipComponents: WorldContextObject invalid."));
		return; 
	}
	
	UE_LOG(MP_VoiceChat, Warning, TEXT("DestroyTransientVoipComponents: Starting DestroyTransientVoip..."));
	
	UWorld* CurrentWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!CurrentWorld)
	{
		UE_LOG(MP_VoiceChat, Warning, TEXT("DestroyTransientVoipComponents: Could not get World from WorldContextObject."));
		return;
	}

	UE_LOG(MP_VoiceChat, Warning, TEXT("DestroyTransientVoipComponents: World is: %s"), *CurrentWorld->GetName());

	int32 DestroyedCount = 0;
	// TObjectIterator finds ALL objects of this class in memory (including Editor previews, other PIE windows)
	// So we must check GetWorld() to ensure we only kill the ones in the current game instance.
	for (TObjectIterator<UVoipListenerSynthComponent> It; It; ++It)
	{
		UVoipListenerSynthComponent* VoipSynth = *It;
		
		// 1. Check if valid
		// 2. Check if it belongs to the World we are currently in (and about to leave) / The world is already gone from the GameThread perspective 
		// 3. Check if it isn't already pending kill
		if (IsValid(VoipSynth) || (VoipSynth->GetWorld() == CurrentWorld || VoipSynth->GetWorld() == nullptr) || !VoipSynth->IsUnreachable())
		{
			UE_LOG(MP_VoiceChat, Log, TEXT("Force cleaning up VoipSynth: %s"), *VoipSynth->GetName());

			// Stop audio rendering for this component
			if (VoipSynth->IsPlaying())
			{
				VoipSynth->Stop();
			}

			// Detach from the Audio Device to prevent the "Unregistered from world None" crash
			if (VoipSynth->IsRegistered())
			{
				VoipSynth->UnregisterComponent();
			}

			// Mark it for destruction
			VoipSynth->DestroyComponent();

			++DestroyedCount;
		}
	}

	UE_LOG(MP_VoiceChat, Warning, TEXT("DestroyTransientVoipComponents: Destroyed %d in total."), DestroyedCount);
    
	// Force GC to recognize these are gone, though usually UnregisterComponent is enough.
	// GEngine->ForceGarbageCollection(true); 
		
}

void UVoiceChatFunctionLibrary::UnregisterLocalTalkers(UObject* WorldContextObject)
{
	IOnlineVoicePtr VoiceInterface = GetVoiceInterface(WorldContextObject);

	if (!VoiceInterface.IsValid()) { return; }

	VoiceInterface->UnregisterLocalTalkers();

	UE_LOG(MP_VoiceChat, Warning, TEXT("UnregisterLocalTalkers: Successfully executed."));
}


bool UVoiceChatFunctionLibrary::IsRemotePlayerTalking(UObject* WorldContextObject, APlayerState* TargetPlayerState)
{
	IOnlineVoicePtr VoiceInterface = GetVoiceInterface(WorldContextObject);
	TSharedPtr<const FUniqueNetId> UniqueId = GetUniqueIdFromPlayerState(TargetPlayerState);

	if (VoiceInterface.IsValid() && UniqueId.IsValid())
	{ return VoiceInterface->IsRemotePlayerTalking(*UniqueId); }
	
	return false;
}

bool UVoiceChatFunctionLibrary::IsPlayerMuted(UObject* WorldContextObject, uint8 LocalUserNumChecking,
	APlayerState* TargetPlayerState)
{
	IOnlineVoicePtr VoiceInterface = GetVoiceInterface(WorldContextObject);
	TSharedPtr<const FUniqueNetId> UniqueId = GetUniqueIdFromPlayerState(TargetPlayerState);

	if (VoiceInterface.IsValid() && UniqueId.IsValid())
	{ return VoiceInterface->IsMuted(LocalUserNumChecking, *UniqueId); }
	
	return false;
}

bool UVoiceChatFunctionLibrary::MuteRemoteTalker(UObject* WorldContextObject, uint8 LocalUserNum,
	APlayerState* TargetPlayerState, bool bIsSystemWide)
{
	IOnlineVoicePtr VoiceInterface = GetVoiceInterface(WorldContextObject);
	TSharedPtr<const FUniqueNetId> UniqueId = GetUniqueIdFromPlayerState(TargetPlayerState);

	if (!UniqueId.IsValid())
	{
		UE_LOG(MP_VoiceChat, Warning, TEXT("MuteRemoteTalker: Invalid unique net id!"));
		return false;
	}

	if (VoiceInterface.IsValid())
	{
		bool MuteStatus = VoiceInterface->MuteRemoteTalker(LocalUserNum, *UniqueId, bIsSystemWide);
		VoiceInterface->ClearVoicePackets();
		return MuteStatus;
	}
	
	return false;
}

bool UVoiceChatFunctionLibrary::UnmuteRemoteTalker(UObject* WorldContextObject, uint8 LocalUserNum,
	APlayerState* TargetPlayerState, bool bIsSystemWide)
{
	IOnlineVoicePtr VoiceInterface = GetVoiceInterface(WorldContextObject);
	TSharedPtr<const FUniqueNetId> UniqueId = GetUniqueIdFromPlayerState(TargetPlayerState);

	if (!UniqueId.IsValid())
	{
		UE_LOG(MP_VoiceChat, Warning, TEXT("MuteRemoteTalker: Invalid unique net id!"));
		return false;
	}
	
	if (VoiceInterface.IsValid())
	{ return VoiceInterface->UnmuteRemoteTalker(LocalUserNum, *UniqueId, bIsSystemWide); }
	
	return false;
}

void UVoiceChatFunctionLibrary::ClearVoicePackets(UObject* WorldContextObject)
{
	IOnlineVoicePtr VoiceInterface = GetVoiceInterface(WorldContextObject);
	if (!VoiceInterface.IsValid()) { return; }

	VoiceInterface->ClearVoicePackets();
	UE_LOG(MP_VoiceChat, Warning, TEXT("ClearVoicePackets: Cleared queued voice packets."));
}

int32 UVoiceChatFunctionLibrary::GetNumLocalTalkers(UObject* WorldContextObject)
{
	IOnlineVoicePtr VoiceInterface = GetVoiceInterface(WorldContextObject);

	if (VoiceInterface.IsValid())
	{ return VoiceInterface->GetNumLocalTalkers(); }
    
	// Return 0 if interface is invalid, safe fail
	UE_LOG(MP_VoiceChat, Warning, TEXT("GetNumLocalTalkers: Voice Interface invalid, returning 0."));
	return 0;
}








