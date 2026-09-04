// Fill out your copyright notice in the Description page of Project Settings.


#include "MCCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"


// Sets default values
AMCCharacter::AMCCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// PrimaryActorTick.bCanEverTick = true;
	
	// Derive the offset instead of hardcoding the height offset so it survives a capsule resize
	const float HalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.0f, 0.0f, -HalfHeight),
		FRotator(0.0f, -90.0f, 0.0f));
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	Move->JumpZVelocity = 500.0f;
	Move->MaxWalkSpeed = WalkSpeed;
	JumpMaxHoldTime = 0.5f;
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 50.0f);
	CameraBoom->bUsePawnControlRotation = true;
	
	CharacterCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CharacterCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	CharacterCamera->bUsePawnControlRotation = false;
}

// Called to bind functionality to input
void AMCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	
	EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	
	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this , &AMCCharacter::Move);
	EIC->BindAction(LookAction, ETriggerEvent::Triggered, this , &AMCCharacter::Look);
	
	EIC->BindAction(SprintAction, ETriggerEvent::Started, this , &AMCCharacter::StartSprint);
	EIC->BindAction(SprintAction, ETriggerEvent::Completed, this , &AMCCharacter::StopSprint);
	EIC->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AMCCharacter::StopSprint);
}

void AMCCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	
	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AMCCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller) { return; }
	
	const FVector2D Input = Value.Get<FVector2D>();
	
	// Zeroing pitch and roll is the C++ equivalent of the split-struct-pin in blueprints
	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FRotationMatrix RotMatrix(YawRotation);
	
	AddMovementInput(RotMatrix.GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(RotMatrix.GetUnitAxis(EAxis::Y), Input.X);
}

void AMCCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	
	AddControllerYawInput(Input.X * LookSensitivity);
	AddControllerPitchInput(Input.Y * LookSensitivity);
}

void AMCCharacter::StartSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AMCCharacter::StopSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}