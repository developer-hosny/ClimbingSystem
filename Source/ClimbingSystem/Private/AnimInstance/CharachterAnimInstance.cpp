// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/CharachterAnimInstance.h"



void UCharachterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ClimbingSystemCharacter = Cast<AClimbingSystemCharacter>(TryGetPawnOwner());

	if (ClimbingSystemCharacter) {
		CustomMovementComponent = ClimbingSystemCharacter->GetCustomMovementComponent();
	}
}

void UCharachterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!ClimbingSystemCharacter || !CustomMovementComponent)return;

	GetGroundSpeed();
	GetShouldMove();
	GetAirSpeed();
	GetIsFalling();
	GetIsClimbing();
	GetClimbVelocity();
}

void UCharachterAnimInstance::GetGroundSpeed()
{
	GroundSpeed =  UKismetMathLibrary::VSizeXY(ClimbingSystemCharacter->GetVelocity());
}

void UCharachterAnimInstance::GetAirSpeed()
{
	AirSpeed = ClimbingSystemCharacter->GetVelocity().Z;
}

void UCharachterAnimInstance::GetShouldMove()
{
	bShouldMove =  CustomMovementComponent->GetCurrentAcceleration().Size()>0
		&&
		GroundSpeed > 5.f 
		&& 
		!bIsFalling;

}

void UCharachterAnimInstance::GetIsFalling()
{
	bIsFalling = CustomMovementComponent->IsFalling();
}

void UCharachterAnimInstance::GetIsClimbing()
{
	bIsClimbing = CustomMovementComponent->IsClimbing();
}


void UCharachterAnimInstance::GetClimbVelocity()
{
	ClimbVelocity = CustomMovementComponent->GetUnrotatedClimbVelocity();
}