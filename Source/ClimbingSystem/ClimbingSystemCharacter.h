// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "MotionWarpingComponent.h"
#include "ClimbingSystemCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UCustomMovementComponent;
class UMotionWarpingComponent;

UCLASS(config = Game)
class AClimbingSystemCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AClimbingSystemCharacter(const FObjectInitializer &ObjectInitializer);

private:
#pragma region Components

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	 USpringArmComponent *CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	 UCameraComponent *FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movment, meta = (AllowPrivateAccess = "true"))
	UCustomMovementComponent *CustomMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movment, meta = (AllowPrivateAccess = "true"))
	UMotionWarpingComponent* MotionWarpingComponent;


#pragma endregion


#pragma region Input

	void OnPlayerEnterClimbState();
	void OnPlayerExitClimbState();

	void AddInputMappingContext(UInputMappingContext* Context, int32 InPriority);
	void RemoveInputMappingContext(UInputMappingContext* Context);

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	 UInputMappingContext *DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputMappingContext* ClimbMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	 UInputAction *JumpAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction *MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ClimbMoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ClimbAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ClimbHopAction;

#pragma endregion


#pragma region InputCallback

	void HandleGroundMovementInput(const FInputActionValue& Value);

	void HandleClimbMovementInput(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue &Value);

	void OnClimbActionStarted(const FInputActionValue &Value);

	void OnClimbHopActionStarted(const FInputActionValue& Value);

#pragma endregion


protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

	// To add mapping context
	virtual void BeginPlay();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE USpringArmComponent *GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE UCameraComponent *GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE UCustomMovementComponent *GetCustomMovementComponent() const { return CustomMovementComponent; }

	FORCEINLINE UMotionWarpingComponent* GetMotionWarpingComponent() const {return MotionWarpingComponent;}
};
