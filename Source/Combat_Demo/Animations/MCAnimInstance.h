// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MCAnimInstance.generated.h"

class UCharacterMovementComponent;


/**
 * 
 */
UCLASS()
class COMBAT_DEMO_API UMCAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> MovementComponent;
	
	// Horizontal speed, drives the walk/run blendspace
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsAirborne = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsJumping = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling = false;
};
