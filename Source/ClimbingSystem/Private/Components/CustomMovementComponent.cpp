// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/CustomMovementComponent.h"

void UCustomMovementComponent::BeginPlay()
{
	OwingPlayerAnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();

	if (OwingPlayerAnimInstance) {
		OwingPlayerAnimInstance->OnMontageEnded.AddDynamic(this, &UCustomMovementComponent::OnClimbMontageEnded);

		OwingPlayerAnimInstance->OnMontageBlendingOut.AddDynamic(this, &UCustomMovementComponent::OnClimbMontageEnded);
	}

	OwningPlayerCharacter = Cast<AClimbingSystemCharacter>(CharacterOwner);
}

void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//CanClimbDownLeadge();

	 //TraceClimbSurfaces();
	 //TraceFromEyeHight(100.f);

}

void UCustomMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (IsClimbing())
	{
		bOrientRotationToMovement = false;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(48.f);

		OnEnterClimbStateDelegate.ExecuteIfBound();
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == ECustomMovementMode::MOVE_Climb)
	{
		bOrientRotationToMovement = true;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);

		const FRotator DirtayRotation = UpdatedComponent->GetComponentRotation();
		const FRotator CleanStandRotation = FRotator(0.f, DirtayRotation.Yaw, 0.f);
		UpdatedComponent->SetRelativeRotation(CleanStandRotation);
		
		StopMovementImmediately();
		
		OnExitClimbStateDelegate.ExecuteIfBound();
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UCustomMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	if (IsClimbing())
	{
		PhysicsClimb(deltaTime, Iterations);
	}

	Super::PhysCustom(deltaTime, Iterations);
}

float UCustomMovementComponent::GetMaxSpeed() const
{
	if (IsClimbing()) {
		return MaxClimbSpeed;
	}
	else
	{
		return Super::GetMaxSpeed();
	}
}

float UCustomMovementComponent::GetMaxAcceleration() const
{
	if (IsClimbing()) {
		return MaxClimbAceleration;
	}
	else
	{
		return Super::GetMaxAcceleration();
	}
}

FVector UCustomMovementComponent::ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const
{
	const bool bIsPlayingRMMontage = 
	IsFalling() && OwingPlayerAnimInstance && OwingPlayerAnimInstance->IsAnyMontagePlaying();

	if (bIsPlayingRMMontage) {
		return RootMotionVelocity;
	}
	else
	{
		return Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity, CurrentVelocity);
	}
}

void UCustomMovementComponent::PhysicsClimb(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	/*Process all the climbable surfaces info*/
	TraceClimbSurfaces();
	ProcessorClimbaleSurfaceInfo();

	//if (CheckHasReachedFloor()) {
	//	Debug::Print(TEXT("Floor Reached"), FColor::Green, 1);
	//}
	//else 
	//{
	//	Debug::Print(TEXT("Floor Not Reached"), FColor::Red, 1);
	//}

	/*Check if we should stop climbing*/
	if (CheckShouldStopClimbing() || CheckHasReachedFloor()) {
		StopClimbing();
	}

	RestorePreAdditiveRootMotionVelocity();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		// Define the max climb speed and acceleration
		CalcVelocity(deltaTime, 0.f, true, MaxBreakClimbDeceleration);
	}

	ApplyRootMotionToVelocity(deltaTime);

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);

	// Handle Climb rotation
	SafeMoveUpdatedComponent(Adjusted, GetClimbRotation(deltaTime), true, Hit);
	

	if (Hit.Time < 1.f)
	{

		// adjust and try again
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}

	/*Snap movement to climbable surfaces*/
	SnapMovementToClimbleSurfaces(deltaTime);

	if (CheckHasReachedLedge()) {
		PlayeClimbMontage(ClimbToTopAnimMontage);
	}
}

void UCustomMovementComponent::ProcessorClimbaleSurfaceInfo()
{
	CurrentClimbableSurfaceLocation = FVector::ZeroVector;
	CurrentClimbableSurfaceNormal = FVector::ZeroVector;

	if (ClimbableSurfacesTracedResults.IsEmpty())
		return;

	for (const FHitResult& TracedHitResult : ClimbableSurfacesTracedResults)
	{
		CurrentClimbableSurfaceLocation += TracedHitResult.ImpactPoint;
		CurrentClimbableSurfaceNormal += TracedHitResult.ImpactNormal;
	}

	CurrentClimbableSurfaceLocation /= ClimbableSurfacesTracedResults.Num();
	CurrentClimbableSurfaceNormal = CurrentClimbableSurfaceNormal.GetSafeNormal();

	//Debug::Print(TEXT("ClimbableSurfaceLocation : " + CurrentClimbableSurfaceLocation.ToCompactString()), FColor::Cyan, 1);
	//Debug::Print(TEXT("ClimbableSurfaceNormal : " + CurrentClimbableSurfaceNormal.ToCompactString()), FColor::Red, 2);
}
 
bool UCustomMovementComponent::CheckShouldStopClimbing()
{
	if (ClimbableSurfacesTracedResults.IsEmpty())return true;
	const float DotResult = FVector::DotProduct(CurrentClimbableSurfaceNormal, FVector::UpVector);
	const float DgreeDiff = FMath::RadiansToDegrees(FMath::Acos(DotResult));
	
	//Debug::Print(TEXT("Degree Diff: ") + FString::SanitizeFloat(DgreeDiff), FColor::Cyan, 1);
	if (DgreeDiff <= 60.f) {
		return true;
	}
	return false;
}

bool UCustomMovementComponent::CheckHasReachedFloor()
{
	const FVector DownVector = -UpdatedComponent->GetUpVector();
	const FVector StartOffset = DownVector * 50.f;

	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + DownVector;

	TArray<FHitResult> PossibleFloorHits = DoCapsuleTraceMultiByObject(Start, End);

	if (PossibleFloorHits.IsEmpty()) return false;

	for(const FHitResult &PossibleFloorHit: PossibleFloorHits){
		bool bReachedFloor =  FVector::Parallel(-PossibleFloorHit.ImpactNormal, FVector::UpVector) && GetUnrotatedClimbVelocity().Z < -10.f;

		if (bReachedFloor) {
			return true;
		}
	}

	return false;
}

FQuat UCustomMovementComponent::GetClimbRotation(float deltaTime)
{
	const FQuat CurrentQuat = UpdatedComponent->GetComponentQuat();
	if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity()) {
		return CurrentQuat;
	}

	FQuat TargetQuat = FRotationMatrix::MakeFromX(-CurrentClimbableSurfaceNormal).ToQuat();

	return FMath::QInterpTo(CurrentQuat, TargetQuat, deltaTime, 5.f);
}

void UCustomMovementComponent::SnapMovementToClimbleSurfaces(float deltaTime)
{
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ProjectedCharacterToSurface = (CurrentClimbableSurfaceLocation - ComponentLocation).ProjectOnTo(ComponentForward);
	const FVector SnapVector = -CurrentClimbableSurfaceNormal * ProjectedCharacterToSurface.Length();
	UpdatedComponent->MoveComponent(SnapVector * deltaTime * MaxClimbSpeed, UpdatedComponent->GetComponentQuat(), true);

}

void UCustomMovementComponent::PlayeClimbMontage(UAnimMontage* MontageToPlay)
{
	if (!MontageToPlay) return;
	if (!OwingPlayerAnimInstance) return;
	if (OwingPlayerAnimInstance->IsAnyMontagePlaying()) return;

	OwingPlayerAnimInstance->Montage_Play(MontageToPlay);
}

void UCustomMovementComponent::OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == IdleToClimbAnimMontage || Montage == ClimbDownLedgeMontage) {
		StartClimbing();
		StopMovementImmediately();
	}

	if(Montage == ClimbToTopAnimMontage || Montage == VaultMontage){
		SetMovementMode(MOVE_Walking);
	}
}

void UCustomMovementComponent::SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPostion)
{
	if (!OwningPlayerCharacter) return;

	OwningPlayerCharacter->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocation(InWarpTargetName, InTargetPostion);

	StartClimbing();

}

void UCustomMovementComponent::HandleHopUp()
{
	FVector HopUpTargetPoint;
	if (CheckCanHopUp(HopUpTargetPoint))
	{
		SetMotionWarpTarget(TEXT("HopUpTargetPoint"), HopUpTargetPoint);

		PlayeClimbMontage(HopUpMontage);
	}
	
}


bool UCustomMovementComponent::CheckCanHopUp(FVector& OutHopUpTragetPosition)
{
	FHitResult HopUpHit =	TraceFromEyeHight(100.f, -20.f);
	FHitResult SaftyLedgHit = TraceFromEyeHight(100.f, 150.f);

	if (HopUpHit.bBlockingHit && SaftyLedgHit.bBlockingHit) {
		
		OutHopUpTragetPosition = HopUpHit.ImpactPoint;

		return true;
	}
	return false;
}

void UCustomMovementComponent::HandleHopDown()
{
	FVector HopDownTargetPoint;
	if (CheckCanHopDown(HopDownTargetPoint))
	{
		SetMotionWarpTarget(TEXT("HopDownTargetPoint"), HopDownTargetPoint);

		PlayeClimbMontage(HopDownMontage);
	}
}

bool UCustomMovementComponent::CheckCanHopDown(FVector& OutHopDownTragetPosition)
{
	FHitResult HopDownHit = TraceFromEyeHight(100.f, -300.f, true, true);

	if (HopDownHit.bBlockingHit) {

		OutHopDownTragetPosition = HopDownHit.ImpactPoint;

		return true;
	}
	return false;
}

FVector UCustomMovementComponent::GetUnrotatedClimbVelocity() const
{
	return UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), Velocity);
}

#pragma region ClimbTraces
TArray<FHitResult> UCustomMovementComponent::DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebugShape, bool bDrawPersistantShapes)
{
	TArray<FHitResult> OutCapsuleTraceHitResults;

	EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;

	if (bShowDebugShape)
	{
		DebugTraceType = EDrawDebugTrace::ForOneFrame;

		if (bDrawPersistantShapes)
		{
			DebugTraceType = EDrawDebugTrace::Persistent;
		}
	}

	UKismetSystemLibrary::CapsuleTraceMultiForObjects(
		this, Start, End,
		ClimbCapsuleTraceRadius,
		ClimbCapsuleTraceHalfHeight,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		DebugTraceType,
		OutCapsuleTraceHitResults,
		false);

	return OutCapsuleTraceHitResults;
}

FHitResult UCustomMovementComponent::DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebugShape, bool bDrawPersistantShapes)
{
	FHitResult OutHit;

	EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;

	if (bShowDebugShape)
	{
		DebugTraceType = EDrawDebugTrace::ForOneFrame;

		if (bDrawPersistantShapes)
		{
			DebugTraceType = EDrawDebugTrace::Persistent;
		}
	}

	UKismetSystemLibrary::LineTraceSingleForObjects(
		this, Start, End,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		DebugTraceType,
		OutHit,
		false);

	return OutHit;
}
#pragma endregion

#pragma region ClimbCore
// Trace for climbable surfaces, return true if there are  indeed valid surfaces, false otherewise
bool UCustomMovementComponent::TraceClimbSurfaces()
{

	FVector StartOffset = UpdatedComponent->GetForwardVector() * 30.f;


	//Debug::Print(TEXT("StartOffset : " + StartOffset.ToCompactString()), FColor::Red, 3);


	FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	FVector End = Start + UpdatedComponent->GetForwardVector();

	ClimbableSurfacesTracedResults = DoCapsuleTraceMultiByObject(Start, End);

	return !ClimbableSurfacesTracedResults.IsEmpty();
}


bool UCustomMovementComponent::CheckHasReachedLedge()
{
	FHitResult LedgeHitResult =  TraceFromEyeHight(100.f, 50.f);
	if (!LedgeHitResult.bBlockingHit) {
		const FVector WalkableSurfaceTraceStart = LedgeHitResult.TraceEnd;
		const FVector DownVector = -UpdatedComponent->GetUpVector();
		const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;

		FHitResult WalkableSurfaceHitResult = 
		DoLineTraceSingleByObject(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd);

		if (WalkableSurfaceHitResult.bBlockingHit) {
			return true;
		}
	}
	return false;

}

void UCustomMovementComponent::TryStartVaulting()
{
	FVector VaultStartPositon;
	FVector VaultLandPostion;

	if (CanStartVaulting(VaultStartPositon, VaultLandPostion)) {
		// Start Vaulting
		SetMotionWarpTarget(TEXT("VaultStartPoint"), VaultStartPositon);
		SetMotionWarpTarget(TEXT("VaultLandPoint"), VaultLandPostion);
		PlayeClimbMontage(VaultMontage);
	}
}

bool UCustomMovementComponent::CanStartVaulting(FVector& OutVaultStartPosition, FVector& OutVaultLandPosition)
{
	if (IsFalling()) return false;

	OutVaultStartPosition = FVector::ZeroVector;
	OutVaultLandPosition = FVector::ZeroVector;

	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector UpVector = UpdatedComponent->GetUpVector();
	const FVector DownVector = -UpdatedComponent->GetUpVector();
	
	for (int32 i = 0; i < 5; i++)
	{
	
		const FVector Start = ComponentLocation + UpVector * 100.f + ComponentForward * 100.f * (i + 1);
		const FVector End = Start + DownVector * 80.f * (i + 1);

		FHitResult VaultTraceHit = DoLineTraceSingleByObject(Start, End);
		if (i == 0 && VaultTraceHit.bBlockingHit) {
			OutVaultStartPosition = VaultTraceHit.ImpactPoint;
		}

		if (i == 1 && VaultTraceHit.bBlockingHit) {
			OutVaultLandPosition = VaultTraceHit.ImpactPoint;
		}

	};

	if (OutVaultStartPosition != FVector::ZeroVector && OutVaultLandPosition!= FVector::ZeroVector) {
		return true;
	}

	return false;
}

void UCustomMovementComponent::RequestHopping()
{
	const FVector UnrotatedLastInputVector =
	UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), GetLastInputVector());

	Debug::Print(UnrotatedLastInputVector.GetSafeNormal().ToString(), FColor::Cyan, 1);

	const float DotProduct = FVector::DotProduct(UnrotatedLastInputVector.GetSafeNormal(), FVector::UpVector);

	Debug::Print(TEXT("Dot Result: ") + FString::SanitizeFloat(DotProduct));

	if(DotProduct >= 0.9f){
		Debug::Print(TEXT("Hop Up"));
		HandleHopUp();
	}
	else if (DotProduct <= -0.9f) 
	{
		Debug::Print(TEXT("Hop Down"));
		HandleHopDown();
	}
	else
	{
		Debug::Print(TEXT("Invalid input"));
	}
}

bool UCustomMovementComponent::IsClimbing() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::MOVE_Climb;
}

void UCustomMovementComponent::ToggleClimbing(bool bEnableClimb)
{
	if (bEnableClimb)
	{
		if (CanStartClimbing())
		{
			PlayeClimbMontage(IdleToClimbAnimMontage);
		}
		else if (CanClimbDownLeadge()) {
			PlayeClimbMontage(ClimbDownLedgeMontage);
		}
		else {
			TryStartVaulting();
		}
		
	}

	if (!bEnableClimb)
	{
		// ToDo: Stop climbing
		StopClimbing();
		// Debug::Print(TEXT("Stopped climbing"));
	}
}

bool UCustomMovementComponent::CanStartClimbing()
{
	if (IsFalling())
		return false;

	if (!TraceClimbSurfaces())
		return false;

	if (!TraceFromEyeHight(100.f).bBlockingHit)
		return false;

	return true;
}

bool UCustomMovementComponent::CanClimbDownLeadge()
{
	if (IsFalling()) return false;

	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector DownVector = -UpdatedComponent->GetUpVector();

	const FVector WalkableSurfaceTraceStart = ComponentLocation + ComponentForward * ClimbDownWalkableSurfaceTraceOffset;

	const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;

	FHitResult WalkableSurfaceHit =  DoLineTraceSingleByObject(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd);

	const FVector LedgeTraceStart = WalkableSurfaceHit.TraceStart + ComponentForward * ClimbDownLedgeTraceOffset;

	const FVector LedgeTraceEnd = LedgeTraceStart + DownVector * 200.f;

	FHitResult LedgeTraceHit = DoLineTraceSingleByObject(LedgeTraceStart, LedgeTraceEnd);

	if (WalkableSurfaceHit.bBlockingHit && !LedgeTraceHit.bBlockingHit) {
		return true;
	}

	return false;
}

void UCustomMovementComponent::StartClimbing()
{
	SetMovementMode(MOVE_Custom, ECustomMovementMode::MOVE_Climb);
}

void UCustomMovementComponent::StopClimbing()
{
	SetMovementMode(MOVE_Falling);
}

FHitResult UCustomMovementComponent::TraceFromEyeHight(float TraceDistance, float TraceStartOffset,
	bool bShowDebugShape, bool bDrawPersistantShapes)
{
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + TraceStartOffset);
	const FVector Start = ComponentLocation + EyeHeightOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;

	return DoLineTraceSingleByObject(Start, End, bShowDebugShape, bDrawPersistantShapes);
}
#pragma endregion
