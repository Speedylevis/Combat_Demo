# Character Constructor

The constructor `AMCCharacter::AMCCharacter()` includes by default `PrimaryActorTick.bCanEverTick = true`. The `Tick()` function is currently not used, so this can be commented out as it will unnecessarily reduce performance while active.

## Components and Attachments

The `ACharacter` parent class creates the standard character subobjects in its constructor and provides handwritten accessors for returning a cached pointer to those subobjects:

| Accessor                 | Type                          | Notes                                               |
| ------------------------ | ----------------------------- | --------------------------------------------------- |
| `GetCapsuleComponent()`  | `UCapsuleComponent`           | The root component                                  |
| `GetMesh()`              | `USkeletalMeshComponent`      | Attached to the capsule                             |
| `GetCharacterMovement()` | `UCharacterMovementComponent` | Not a scene component, no transform                 |
| `GetRootComponent()`     | `USceneComponent`             | From `AActor`, returns the capsule in this instance |

There's no `GetComponent<T>()` in Unreal, so to find accessors requires looking at what the base class actually exposes. For components where there isn't an accessor, there is a runtime lookup: `FindComponentByClass<T>()`. It searches the actor's component array, which can be expensive if performed frequently. Don't call it every frame, instead cache the result.

`CreateDefaultSubobject<T>(TEXT("Name"))` builds a component as part of the class default object. It is constructor-only, and calling it at runtime will assert. The `"Name"` must be unique within the class and shows in the editor's component tree.

`SetupAttachment` is the constructor-time counterpart to the runtime `AttachToComponent`. It only records an intent, as the actual attachment happens later when components register. This also means that components can be attached in any order in the constructor. Always name the parent to avoid relying on implicit rooting.

The second argument to `SetupAttachment` is a socket. A **socket** is a named attachment point that a component can define, with its own transform, distinct from the component's own transform. `USpringArmComponent` defines exactly one, `SocketName` (the string is `"SpringEndpoint"`), positioned at the far end of the arm. Attach without a socket name and the camera lands at the spring arm's origin. The **origin** is the pivot on a component (in this case the character).

## Capsule and Skeletal Mesh Components

The `UCapsuleComponent` defines the bounds of the actor. It determines where the actor is in the world and how it collides with walls and floors. `USkeletalMeshComponent` references the instance of the skeletal mesh asset.

A capsule has its origin at the center of the capsule, whereas a mesh of a character typically has its origin at the root bone. If this root bone is the feet, the mesh's feet will be at the center of the capsule and it will only fill the top half of the capsule. The half-height of the capsule is this distance from the center of the capsule to its top (or bottom, on a symmetrical capsule). Offsetting the Z-value (in Unreal's Z-up coordinate system) of the relative location of the mesh by the half-height of the capsule puts its roughly in the bounds of the capsule. This value may need to be tweaked to get it to fit exactly as desired, which can be tested in the editor, but assigning the offset in the code ensures that changes to the capsule are automatically reflected in the mesh's position.

Unreal Engine considers the +X as the forward direction, whereas outside modeling software (e.g. Blender) might export with +Y as the forward direction. This causes the skeletal mesh's forward (imported from an outside software) to be offset by 90 degrees from the capsule's forward (from Unreal Engine). Offsetting the yaw of the local rotation of the mesh by 90 degrees aligns the forward of the mesh and the capsule. In Unreal, the yaw relates to rotation around the Z-up axis.

## Spring Arm Component

The `USpringArmComponent` maintains its children components at a fixed distance (`TargetArmLength`) from the its parent component. In this case, it is used to hold the camera behind the character. If the spring arm collides with something (like a wall), it will retract and prevent the camera from clipping.

`USpringArmComponent` has a `SocketOffset` for the child's attachment point at the end of the spring arm and a `TargetOffset` for the parent's attachment point at the origin of the spring arm. These offsets should be used instead of adjusting the relative locations of the spring arm and its children to ensure that the spring arm's line trace can properly address collisions, rotation, and lag.

## Camera Component

The current camera is a simple attachment to a character's spring arm. 

The value `bUsePawnControlRotation` is used to update the rotation of a component to match the pawn it is attached to. This is set to true on the spring arm so that the arm adopts control rotation directly, decoupling the camera from the body). It is set to false on the camera so that it inherits the arm's endpoint transform. Setting it true bypasses the arm and breaks camera lag.

## Character Movement Component

The `UCharacterMovementComponent` handles movement logic across various movement modes (e.g. walking, jumping, swimming, flying, custom).

See the [Movement](#Movement) section for how accumulated input determines acceleration and velocity of the character.

The `ACharacter` parent class comes with a `Jump` function that applies velocity along the Z-up axis based on `JumpZVelocity`. A `StopJumping` function stops applying the z-velocity and causes the character to fall. Having a positive `JumpMaxHoldTime` value allows the height of the character's jump to be determined by how long the player holds the associated jump key. The world gravity directly affects the height and speed of the character's jump and fall.

---
<br>

# Rotation Flags

UE uses a [Z-up, left-hand coordinate system](https://dev.epicgames.com/documentation/unreal-engine/coordinate-system-and-spaces-in-unreal-engine?lang=en-US) (LHS), and the rotation elements are different than in other systems:

- The Z-axis determines how far up and down along a vertical line an actor is located. 
    - Positive values are upwards.
    - Rotation element: **Yaw**
- The Y-axis determines how far to the left or right an actor is located.
    - Positive values are to the right.
    - Rotation element: **Pitch**
- The X-axis determines how far forward or backward an actor is located.
    - Positive values are forward.
    - Rotation element: **Roll**

It is important to note that an `FRotator` orders `(Pitch, Yaw, Roll)`, while in the detail panel in the editor it is ordered as `(X=Roll, Y=Pitch, Z=Yaw)`.

There are three separate rotations active:

- **Control Rotation**
	- Lives on the `AController`, not the pawn
	- Where the player is looking
	- `AddControllerYawInput` and `PitchInput` feed it
- **Actor Rotation**
	- The character's body in the world
	- Which way the mesh faces
- **Component Rotation**
	- The spring arm and camera

`bUseControllerRotationYaw/Pitch/Roll` (from `APawn`) controls whether the *actor* copies the control rotation. Each frame the controller calls `FaceRotation` on the pawn, and for each axis where the flag is true, the actor's rotation is forced to match the control rotation on that axis.

With yaw set to `true`, the character's body always faces the camera direction (e.g. for strafing and FPS). With yaw set to `false`, the body is free and `bOrientRotationToMovement` on the `UCharacterMovementComponent` takes over. The character rotates to face its velocity at `RotateRate.Yaw` degrees per second. This is why `bUseControllerRotationYaw` and `bOrientRotationToMovement` conflict and shouldn't be set to true at the same time. Pitch and roll are false as they introduce unrealistic character movement based on the camera rotation. ==Remember that yaw refers to the Z-up axis, so rotating around it turns left/right.==

---
<br>

# Pawn Control and Subsystems

`NotifyControllerChanged` is an overridden function from `APawn` that triggers an event on a controller change for the current pawn (in this case, the `MCCharacter`).

`Cast<>` returns `nullptr` on failures, so the if-statement will read false if a casting fails due to no controller or AI controller.

A **subsystem** is UE5's managed-singleton pattern. The engine automatically instantiates exactly one per owning object and destroys it with that owner. This allows for automatic lifetime management without a global state.

A mapping context defines a set of actions that result from a set of inputs. It makes it easier to change what a large set of inputs do by just swapping the mapping context.
`UEnhancedInputLocalPlayerSubsystem` owns the stack of active mapping contexts for one local player. At input time, it walks the stack by priority and resolves which key press maps to which `UInputAction`.

`AddMappingContext` pushes the context onto the subsystem's stack. The subsystem is a stack so that different context can be assigned a different priority. Gameplay bindings are assigned the lowest priority `0`, and then something like a menu context will be assigned priority `1` and take over when the game is paused.

---
<br>

# Input Actions and Bindings

`SetupPlayerInputComponent` is an overridden function from `APawn`. The `UInputComponent` parameter is provided based on the Default Input Component Class for the project (Project Settings > Input). If this isn't set to `UEnhancedInputComponent` (which it should be by default for UE5), `CastChecked<>` will assert.

The `UEnhancedInputComponent` takes an input action and binds it to a function which is called on the assigned event.

The input mapping context and input actions are data assets, and the best approach is to assign them in the editor. Hardcoding a path in C++ means a rename breaks it silently at runtime instead of at compile time.

#### Triggered Events

The assigned event determines how often the bound function is called:

- `ETriggerEvent::Started`: fires once on the frame the action begins
- `ETriggerEvent::Triggered`: fires every frame the trigger condition holds
- `ETriggerEvent::Completed`: fires once when it ends
- `ETriggerEvent::Ongoing/Canceled`: important for held/timed actions

#### Input Actions

`UInputAction`s are created in the editor, where the action determines a value based on how it is set up. The action mapping context determines which keys trigger which input actions.

The value type of the action determines what `FInputActionValue` it stores. For example, a `bool` action type will be either zero or one if the key is pressed or not. An `Axis2D` value will be a `FVector2D`, where value a key press represents can be defined by the action.

Modifiers can be applied to manipulate the action's value. A negate modifier flips the sign of the value, or a swizzle modifier can swap the values of a multi-dimensional value type. Modifiers can be placed on the input action itself, or on the mapping context where the key is bound to the action. These effects can be applied as modifiers through the editor, or through the C++ code.

#### Action Functions

Some functions, like `Jump` and `StopJumping` are readily available to be bound to input actions. If a function doesn't exist, or existing functions don't have the desired functionality, custom functions can be tied to action instead.

`BindAction` is templated over several handler signatures (e.g. `void()`, `void(const FInputActionValue&)`, `void(const FInputActionInstance&)`). It passes the current action value when the signature asks for one for the bound functions to use.

---
<br>

# Movement

`AddMovementInput()` accumulates `WorldDirection * ScaleValue` into a pending input vector on the pawn:

1. The movement component consumes the vector on its next tick
	- `ConsumeMovementInputVector()`
2. Value becomes an acceleration direction scaled by `MaxAcceleration` and clamped to 1.0
	- W and D being pressed at the same time (1,1) produces a magnitude of 1.41, which would make diagonal movement faster unless clamped
	- `Acceleration = GetMaxAcceleration() * InputAcceleration.GetClampedToMaxSize(1.0f)`
3. Velocity integrates towards that direction and is capped by `MaxWalkSpeed`
4. `bOrientRotationToMovement` reads the resulting velocity and rotates the actor toward it at `RotationRate.Yaw`

The input actions provide the value as follows:

- W: (0, 1)
- S:  (0, -1)
- D: (1, 0)
- A: (-1, 0)

W/S provides the `Input.Y` value and D/A provides `Input.X`. These are used as the `ScaleValue`, where if it is negative it moves in the opposite direction.

Control rotation is world-space, always, and `bUsePawnControlRotation` on the camera boom ensures that "where the camera points" and "control rotation" are the same yaw.

`FRotator`'s constructor is `(Pitch,Yaw,Roll)`. So a pitch value looks up/down, a yaw value turns left/right, and a roll value tilts side to side. In this case, only yaw matters because it is reflects the direction around the positive-z axis to rotate the character.

`FRotationMatrix` converts the rotator into a 4x4 matrix, where the upper 3x3 is a rotation basis:

$$
	\begin{pmatrix}
	\cos\theta & \sin\theta & 0 \\
	-\sin\theta & \cos\theta & 0 \\
	0 & 0 & 1
	\end{pmatrix}
$$
- The Forward vector is row 0
	- `EAxis::X` is $(\cos\theta, \sin\theta, 0)$
- The Right vector is row 1
	- `EAxis::Y` is $(-\sin\theta, \cos\theta, 0)$
- The Up vector is row 2
	- `EAxis::Z` is is $(0, 0, 1)$

The yaw element is consumed by sines and cosines and spread across the whole matrix. Row 2 is exactly world up, because a yaw-only rotation spins about Z and leaves Z fixed. Rows 0 and 1 are the world axes swung around by $\theta$.

Examples using values:

| Yaw          | Forward (X)       | Right (Y)          |
| ------------ | ----------------- | ------------------ |
| $0\degree$   | $(1,0,0)$         | $(0,1,0)$          |
| $90\degree$  | $(0,1,0)$         | $(-1,0,0)$         |
| $180\degree$ | $(-1,0,0)$        | $(0,-1,0)$         |
| $45\degree$  | $(0.707,0.707,0)$ | $(-0.707,0.707,0)$ |

Consider the camera pointing along +Y (yaw=90), and the player holds W so that `Input=(0,1)`:

```C++
AddMovementInput(Forward, Input.Y); // (0,1,0)  * 1  = (0,1,0)
AddMovementInput(Right, Input.X);   // (-1,0,0) * 0  = (0,0,0)
```

The accumulated input is `(0,1,0)`, which is world +Y. With the camera being pointed in this direction, the movement is straight away from the camera along +Y. Turn the camera 180 degrees and the same keypress produces `(0,-1,0)`. The character still moves straight away from the camera on the same keypress, but now along a different direction in world space -Y.

W+D at yaw 0 gives `(1,0,0)+(0,1,0)=(1,1,0)`. The length of 1.41 is clamped to a length of 1 before use, so it would be `(0.707,0.707,0)`.

On a gamepad, the amount the analog stick is tilted can change the movement speed. Fully tilting the stick moves at full speed while partially tilting the stick can implement walking. In this case, `Acceleration.Size() / GetMaxAcceleration()` recovers the $[0,1]$ magnitude. `MaxSpeed * AnalgoInputModifier` then determines the speed based on the analog stick.

---
<br>

# Look Around

Mouse input is a *delta*, not a position (it reflects how far the mouse moved since the last frame). Important to consider the difference in a gamepad, where a thumbstick reports a *held position* and continuously rotates. The gamepad implementation would require `* DeltaSeconds` to smoothly rotate the camera over time.

`Input.X` is horizontal movement, `Input.Y` is vertical. `AddControllerYawInput` turns left/right around the Z-up axis. `AddControllerPitchInput` tilts up/down around the Y-right axis. They accumulate into the controller's rotation input, which `APlayerController::UpdateRotation` applies to control rotation each frame. Pitch is clamped by the `ViewPitchMin`/`ViewPitchMax` based on `PlayerCameraManager` to prevent rolling the camera over the top/bottom.

Raw vertical axis sign and `AddControllerPitchInput`'s sign convention don't line up for the standard (non-inverted) look. In Unreal, positive pitch looks up, which is opposite to what a strict left-hand rule about +Y would give. The negation on Y creates the desired non-inverted vertical look, and should only be applied in one case. Here it is handled by the IMC, not in the code.

A `LookSensitivity` value adjusts the rate that the camera changes.

---