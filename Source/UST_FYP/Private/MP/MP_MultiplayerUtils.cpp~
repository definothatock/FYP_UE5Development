// Fill out your copyright notice in the Description page of Project Settings.


#include "MP/MP_MultiplayerUtils.h"

void UMP_MultiplayerUtils::PrintLocalNetRole(AActor* Actor)
{
	if (Actor->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Actor [%s] Has Authority."), *Actor->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Actor [%s] Does NOT have Authority."), *Actor->GetName());
	}

	// Log the LocalRole of the Actor (currently Warning tag for visibility in output log)
	switch (Actor->GetLocalRole())
	{
		case ROLE_None:
			UE_LOG(LogTemp, Warning, TEXT("Actor [%s] LocalRole: None - No Networking"), *Actor->GetName());
			break;

		case ROLE_SimulatedProxy:
			UE_LOG(LogTemp, Warning, TEXT("Actor [%s] LocalRole: SimulatedProxy - Network Client receiving updates from sever"), *Actor->GetName());
			break;
			
		case ROLE_AutonomousProxy:
			UE_LOG(LogTemp, Warning, TEXT("Actor [%s] LocalRole: AutonomousProxy - Network Client, controlled locally"), *Actor->GetName());
			break;

		case ROLE_Authority:
			UE_LOG(LogTemp, Warning, TEXT("Actor [%s] LocalRole: Authority - Server or single player"), *Actor->GetName());
			break;
			
		default:
			UE_LOG(LogTemp, Warning, TEXT("Actor [%s] LocalRole: Unknown role value"), *Actor->GetName());
			break;
	}


}

void UMP_MultiplayerUtils::PrintRemoteNetRole(AActor* Actor)
{
	// Log the RemoteRole of the Actor (currently Warning tag for visibility in output log)
	switch (Actor->GetRemoteRole())
	{
		case ROLE_None:
			UE_LOG(LogTemp, Warning, TEXT("Actor [%s] RemoteRole: None - No Networking"), *Actor->GetName());
			break;

		case ROLE_SimulatedProxy:
			UE_LOG(LogTemp, Warning, TEXT("Actor [%s] RemoteRole: SimulatedProxy - Network Client receiving updates from sever"), *Actor->GetName());
			break;
				
		case ROLE_AutonomousProxy:
			UE_LOG(LogTemp, Warning, TEXT("Actor [%s] RemoteRole: AutonomousProxy - Network Client, controlled locally"), *Actor->GetName());
			break;

		case ROLE_Authority:
			UE_LOG(LogTemp, Warning, TEXT("Actor [%s] RemoteRole: Authority - Server or single player"), *Actor->GetName());
			break;
				
		default:
			UE_LOG(LogTemp, Warning, TEXT("Actor [%s] RemoteRole: Unknown role value"), *Actor->GetName());
			break;
	}
}
