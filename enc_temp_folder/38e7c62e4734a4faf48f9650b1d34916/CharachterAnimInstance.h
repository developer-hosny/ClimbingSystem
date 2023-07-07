// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ClimbingSystem/ClimbingSystemCharacter.h"
#include "Components/CustomMovementComponent.h"
#include "CharachterAnimInstance.generated.h"


class AClimbingSystemCharacter;
class UCustomMovementComponent;
/**
 * 
 */
UCLASS()
class CLIMBINGSYSTEM_API UCharachterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY()
	AClimbingSystemCharacter* ClimbingSystemCharacter;
	
	UPROPERTY()
	UCustomMovementComponent* CustomMovementComponent;
};
