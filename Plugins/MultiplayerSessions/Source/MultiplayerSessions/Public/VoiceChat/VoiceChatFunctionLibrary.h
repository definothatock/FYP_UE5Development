// Some are taken or rewritten from VR Expansion Plugin and Epic Online Services Integration Kit.
// Since Those are under MIT license, this Library will also be.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AudioDevice.h"
#include "Net/VoiceConfig.h" // for calling voip?
#include "VoipListenerSynthComponent.h"

#include "GameFramework/PlayerState.h"

#include "Online.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/VoiceInterface.h"

#include "UObject/UObjectIterator.h"

#include "VoiceChatFunctionLibrary.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(MP_VoiceChat, Warning, All);


/**
 *  This Library is built and altered for OSSv2, VOIP specifically.
 *  Using EOS is a better choice, but I have already run into too many rabbit hole, so I will leave this be.
 *   - Ad
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UVoiceChatFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Starts networked voice, allows push to talk in coordination with StopNetworkedVoice
	UFUNCTION(BlueprintCallable, Category = "Online|AdvancedVoice", meta = (WorldContext = "WorldContextObject"))
	static void StartNetworkedVoice(UObject* WorldContextObject, uint8 LocalPlayerNum = 0);

	// Stops networked voice, allows push to talk in coordination with StartNetworkedVoice
	UFUNCTION(BlueprintCallable, Category = "Online|AdvancedVoice", meta = (WorldContext = "WorldContextObject"))
	static void StopNetworkedVoice(UObject* WorldContextObject, uint8 LocalPlayerNum = 0);
	
	// Wrapper to safely UnRegisters all remote players as talkers, then call ClearVoicePackets().
	UFUNCTION(BlueprintCallable, Category = "Online|AdvancedVoice", meta = (WorldContext = "WorldContextObject"))
	static void RemoveAllRemoteTalkers(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Online|AdvancedVoice", meta = (WorldContext = "WorldContextObject"))
	static void RegisterRemoteTalker(UObject* WorldContextObject, APlayerState* TargetPlayerState);
	
	// Finds and safely destroys all VoipListenerSynthComponents in the Transient package.
	// Call this Event EndPlay or before OpenLevel.
	UFUNCTION(BlueprintCallable, Category = "Online|AdvancedVoice", meta = (WorldContext = "WorldContextObject"))
	static void DestroyTransientVoipComponents(const UObject* WorldContextObject);

	// This is not needed. StopNetworkedVoice() us enough: [403]LogOnlineVoice: OSS: StopLocalVoiceProcessing(0) returned 0x00000000
	UFUNCTION(BlueprintCallable, Category = "Online|AdvancedVoice",  meta = (WorldContext = "WorldContextObject"))
	static void UnregisterLocalTalkers(UObject* WorldContextObject);
	
	// Returns whether a remote player is currently talking
	UFUNCTION(BlueprintPure, Category = "Online|AdvancedVoice|VoiceInfo", meta = (WorldContext = "WorldContextObject"))
	static bool IsRemotePlayerTalking(UObject* WorldContextObject, APlayerState* TargetPlayerState);

	// Returns whether a player is muted for the specified local player
	UFUNCTION(BlueprintPure, Category = "Online|AdvancedVoice|VoiceInfo", meta = (WorldContext = "WorldContextObject"))
	static bool IsPlayerMuted(UObject* WorldContextObject, uint8 LocalUserNumChecking, APlayerState* TargetPlayerState);

	// Mutes the player associated with the PlayerState for the all the specified remote local player.
	// Should then also Call ClearVoicePackets().
	UFUNCTION(BlueprintCallable, Category = "Online|AdvancedVoice", meta = (WorldContext = "WorldContextObject"))
	static bool MuteRemoteTalker(UObject* WorldContextObject, uint8 LocalUserNum, APlayerState* TargetPlayerState, bool bIsSystemWide = false);

	// Reverse MuteRemoteTalker
	UFUNCTION(BlueprintCallable, Category = "Online|AdvancedVoice", meta = (WorldContext = "WorldContextObject"))
	static bool UnmuteRemoteTalker(UObject* WorldContextObject, uint8 LocalUserNum, APlayerState* TargetPlayerState, bool bIsSystemWide = false);

	// Seamless Travel need this: https://youtu.be/7phov5S1jwo?si=a1-HYyfP1IyG02Fc&t=378
	UFUNCTION(BlueprintCallable, Category = "Multiplayer|VoiceChat", meta = (WorldContext = "WorldContextObject"))
	static void ClearVoicePackets(UObject* WorldContextObject);



/*
 * ==================== Getter ====================
 */
	
	/**
	 * Returns the number of local talkers from VoiceInterface
	 * @return 0 if invalid, safe fail
	*/
	UFUNCTION(BlueprintPure, Category = "Online|AdvancedVoice|VoiceInfo", meta = (WorldContext = "WorldContextObject"))
	static int32 GetNumLocalTalkers(UObject* WorldContextObject);


/*
 * ==================== Other Helper ====================
 */
	
	UFUNCTION(BlueprintCallable, Category = "Online|AdvancedVoice|Helper")
	static inline void DumpVoipSynthState()
	{
		int32 Total = 0;
		for (TObjectIterator<UVoipListenerSynthComponent> It; It; ++It)
		{
			UVoipListenerSynthComponent* VoipSynth = *It;
			if (!IsValid(VoipSynth))
			{
				continue;
			}

			UE_LOG(MP_VoiceChat, Warning,
				   TEXT("DumpVoipSynthState: Synth=%s, World=%s, Outer=%s, Registered=%d, Playing=%d, GarbageElimination=%d"),
				   *VoipSynth->GetPathName(),
				   VoipSynth->GetWorld() ? *VoipSynth->GetWorld()->GetPathName() : TEXT("nullptr"),
				   VoipSynth->GetOuter() ? *VoipSynth->GetOuter()->GetPathName() : TEXT("nullptr"),
				   VoipSynth->IsRegistered(),
				   VoipSynth->IsPlaying(),
				   VoipSynth->IsGarbageEliminationEnabled());

			++Total;
		}
		UE_LOG(MP_VoiceChat, Warning, TEXT("DumpVoipSynthState: Total synths in memory: %d"), Total);
	}

	
};
