// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include <Kismet/KismetMathLibrary.h>
#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "ClimbingSystem/DebugHelper.h"
#include "Components/CapsuleComponent.h"
#include "MotionWarpingComponent.h"
#include "CustomMovementComponent.generated.h"

DECLARE_DELEGATE(FOnEnterClimbState)
DECLARE_DELEGATE(FOnExitClimbState)

class UAnimMontage;
class UAnimInstance;
class AClimbingSystemCharacter;

UENUM(BlueprintType)
namespace ECustomMovementMode
{
	enum Type
	{
		MOVE_Climb UMETA(DisplayName = "Climb Mode")
	};
}
/**
 *
 */
UCLASS()
class CLIMBINGSYSTEM_API UCustomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	FOnEnterClimbState OnEnterClimbStateDelegate;
	FOnExitClimbState OnExitClimbStateDelegate;

protected:
#pragma region OverridenFunctions

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	virtual float GetMaxSpeed() const override;

	virtual float GetMaxAcceleration() const override;

	virtual  FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const override;
#pragma endregion

private:
#pragma region ClimbTraces

	TArray<FHitResult> DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebugShape = false, bool bDrawPersistantShapes = false);

	FHitResult DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebugShape = false, bool bDrawPersistantShapes = false);

#pragma endregion

#pragma region ClimbCore
	bool TraceClimbSurfaces();

	FHitResult TraceFromEyeHight(float TraceDistance, float TraceStartOffset = 0.f, bool bShowDebugShape = false, bool bDrawPersistantShapes = false);

	bool CanStartClimbing();

	bool CanClimbDownLeadge();

	void StartClimbing();

	void StopClimbing();

	void PhysicsClimb(float deltaTime, int32 Iterations);

	void ProcessorClimbaleSurfaceInfo();
	
	bool CheckShouldStopClimbing();

	bool CheckHasReachedFloor();

	FQuat GetClimbRotation(float deltaTime);

	void SnapMovementToClimbleSurfaces(float deltaTime);

	void PlayeClimbMontage(UAnimMontage* MontageToPlay);

	UFUNCTION()
	void OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPostion);

	void HandleHopUp();

	bool CheckCanHopUp(FVector& OutHopUpTragetPosition);

	void HandleHopDown();

	bool CheckCanHopDown(FVector& OutHopDownTragetPosition);

#pragma endregion

#pragma region ClimbCoreVariables

	TArray<FHitResult> ClimbableSurfacesTracedResults;

	FVector CurrentClimbableSurfaceLocation;

	FVector CurrentClimbableSurfaceNormal;

	UPROPERTY()
	UAnimInstance* OwingPlayerAnimInstance;

	UPROPERTY()
	AClimbingSystemCharacter* OwningPlayerCharacter;

#pragma endregion

#pragma region ClimbBPVariables

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
		TArray<TEnumAsByte<EObjectTypeQuery>> ClimbableSurfaceTraceTypes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
		float ClimbCapsuleTraceRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
		float ClimbCapsuleTraceHalfHeight = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
		float MaxBreakClimbDeceleration = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
		float MaxClimbSpeed = 100.f;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
		float MaxClimbAceleration = 100.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
		float ClimbDownWalkableSurfaceTraceOffset = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
		float ClimbDownLedgeTraceOffset = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* IdleToClimbAnimMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ClimbToTopAnimMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ClimbDownLedgeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* VaultMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopUpMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movment: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopDownMontage;

#pragma endregion

public:
	void ToggleClimbing(bool bEnableClimb = false);
	bool CheckHasReachedLedge();
	void TryStartVaulting();
	bool CanStartVaulting(FVector& OutVaultStartPosition, FVector& OutVaultLandPosition);
	void RequestHopping();
	bool IsClimbing() const;
	FORCEINLINE FVector GetClimableSurfaceNormal() const { return CurrentClimbableSurfaceNormal; }
	FVector GetUnrotatedClimbVelocity()	const;
};
