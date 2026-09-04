// Fill out your copyright notice in the Description page of Project Settings.


#include "MCAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


void UMCAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	if (const ACharacter* Character = Cast<ACharacter>(GetOwningActor()))
	{
		MovementComponent = Character->GetCharacterMovement();
	}
}

void UMCAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
	if (!MovementComponent) { return; }
	
	const FVector Velocity = MovementComponent->Velocity;
	
	GroundSpeed = Velocity.Size2D();
	bShouldMove = GroundSpeed > 3.0f && !MovementComponent->GetCurrentAcceleration().IsNearlyZero();
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, MovementComponent->GetLastUpdateRotation());
	
	bIsAirborne = MovementComponent->IsFalling();
	bIsJumping = bIsAirborne && Velocity.Z >  0.0f;
	bIsFalling = bIsAirborne && Velocity.Z <= 0.0f;
}