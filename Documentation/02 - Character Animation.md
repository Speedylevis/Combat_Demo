# Animation Setup in the Editor

**Create new animation class**

- In Rider, create a new Unreal class
	- Derived from `UAnimInstance`
	- Name it `MCAnimInstance`
	- The created class is `UMCAnimInstance`, which denotes it as a `UObject`

**Create an animation blueprint:**

- In the editor, `Content Browser > Animation > Animation Blueprint`
	- Set parent class to `MCAnimInstance`
	- Set skeleton to `SK_Mannequin`
	- Name it `ABP_MC`

---
<br>

# AnimGraph Setup

- Open the Animation Blueprint: `ABP_MC`
	- Right-click in the AnimGraph, choose State Machine
		- Name "Locomotion"
		- Drag state machine to output pose
	- Double-click to enter state machine

## Animation Code

Variables in the derived AnimInstance class can be marked as `BlueprintReadOnly` to be accessible to the AnimGraph. This allows defining the values of variables in the code, and having state transitions in the editor defined by the variables.


#### Acceleration and Velocity

The acceleration element of the movement component is the *intent* to move. Releasing the movement key press immediately drops acceleration to zero on the next frame. The size of the velocity vector `Velocity.Size2D()` reflects the magnitude of the character's current velocity in Unreal units (cm/s). With a `MaxWalkSpeed=500.0f`, this value can range from $[0,500]$. `CharacterMovementComponent` ramps toward `MaxWalkSpeed` using `MaxAcceleration` and back down using `BrakingDecelerationWalking` with `GroundFriction` applied. The velocity sweeps the range as the character stops/starts, which allows more detailed blending between movement speeds and animations. `Velocity.Size2D()` deliberately drops the z-component of the character's velocity, as it only needs to reflect movement in the XY-plane. Velocity while airborne is dominated by the z-component, and gravity should not affect the walk/run blend.

It is important to note that `GetCurrentAcceleration()` only reflects *input*. If an external force moves the character without input (e.g. knockback, a conveyor), `bShouldMove` will remain false. ==This will cause the character to slide in the idle pose.==

Velocity never truly reaches zero for multiple reasons, such as the friction based nature of its calculation or that moving objects can create slight velocity in the character. And at a small velocity, the character is covering distance that is almost unnoticeable but the character animation still plays. Setting `bShouldMove=false` when `GroundSpeed` falls below a small, non-zero value ensures that the animation transitions at near-zero speeds. The Lyra and third-person template projects use threshold values in the 3.0-10.0 range. For a max speed of 500.0, 3.0 is 0.6% so it's not clipping at anything the player would notice.

Because the velocity size is a magnitude it is never negative. A `CalculateDirection` function is available using the character movement component's velocity and rotation. It is not currently used, but will be important for animations that are based on where the character is facing. The original implementation used the following:

```C++
Direction = CalculateDirection(Velocity, MovementComponent->GetLastUpdateRotation());
```

A build warning indicated that `UAnimInstance::CalculateDirection` is deprecated, and to use `UKismetAnimationLibary::CalculateDirection` instead. This requires adding `AnimGraphRuntime` to the `PublicDependencyModuleNames` in the `Build.cs` file and regenerating the project files.

#### Jumping and Falling

The `UCharacterMovementComponent` has an enum `MovementMode` for if the character is walking, falling, flying, swimming, and has functionality for implementing custom movement modes. The function `IsFalling()` returns true/false if `MovementMode==MOVE_Falling`. This is defined by if the character is "falling under the effects of gravity", which can be both if it is in the jumping or falling state, meaning that this tells if the character is airborne. The character's `Velocity.Z` will be greater than zero if they are jumping and less than or equal to zero if they are falling. This allows creation of `bIsAirborne`, `bIsJumping`, and `bIsFalling`.

#### Native Functions

`UAnimInstance` exposes a set of C++ virtual functions that mirror the Blueprint events. They act as the C++ entry points that execute right before the Blueprint Event Graph counterparts. Labeled as `Native`, each one has a specific place in the frame and a specific set of rules about what is allowed:

| Function                          | Blueprint Equivalent                   | When                                   | Thread |
| --------------------------------- | -------------------------------------- | -------------------------------------- | ------ |
| `NativeInitializeAnimation`       | Blueprint Initialize Animation         | Once, when the AnimInstance is created | Game   |
| `NativeBeginPlay`                 | Blueprint Begin Play                   | Once, after the owner has begun play   | Game   |
| `NativeUpdateAnimation`           | Blueprint Update Animation             | Every frame                            | Game   |
| `NativeThreadSafeUpdateAnimation` | Blueprint Thread Safe Update Animation | Every frame, after the above           | Worker |
| `NativePostEvaluateAnimation`     | -                                      | After the pose is evaluated            | Game   |

The AnimInstance is a `UObject` created by the engine when the skeletal mesh component sets up its anim class. At constructor time, it has no owner wired up, so `GetOwningActor()` returns nothing useful. `NativeInitializeAnimation` is the first point where the owner exists, so the cast-and-cache of the character and its movement component exist here instead of in a constructor. `NativeInitializeAnimation` can possibly fire before the owner is fully assembled in some spawn orders. If this occurs, use `NativeBeginPlay` (which fires later) with a belt-and-braces approach:

```C++
if (!MovementComponent)
{
    if (const ACharacter* Character = Cast<ACharacter>(GetOwningActor()))
    {
        MovementComponent = Character->GetCharacterMovement();
    }
    if (!MovementComponent) { return; }
}
```

`NativeUpdateAnimation` runs on the game thread, inline with everything else. `NativeThreadSafeUpdateAnimation` runs on a worker thread, in parallel with other work the engine is doing. With a crowd of characters on screen, animation update is one of the most parallelizable costs in the frame. Source code comments suggest the bulk of the work being done in `NativeThreadSafeUpdateAnimation`. However, it requires not touching anything that isn't thread-safe:

- Not Allowed
	- Line traces or any world query
	- Spawning or destroying actors
	- Calling arbitrary gameplay functions on other objects
	- Modifying anything outside the AnimInstance
	- Most of the `UWorld` API
	- Anything touching the render or physics scene mid-update
- Allowed
	- Reading cached values off pointers stored in initialize
	- Pure math
	- Writing to user-created member variables

Everything in this update is a cached read plus arithmetic. If later needing the game thread (e.g. foot-placement line trace), it is recommended to do the work in the character and store the result in a `UPROPERTY`. The AnimInstance can read that cached value in the thread-safe update.

The AnimInstance's job is to **translate game state into animation parameters**. It should not be the thing that computes game state. Character owns the state, AnimInstance reads it. The formalization of this is called the [Property Access System](https://dev.epicgames.com/documentation/unreal-engine/property-access-in-unreal-engine?lang=en-US). It allows declaring bindings to other objects' properties that the engine resolves safely off-thread. The Lyra project uses this heavily, but it is overkill for now.

## Animation State Machines

To start, only four states are needed. The animations used are migrated from the third-person template project:

- `Idle`
	- Animation: `MM_Idle`
	- Entry for the state machine
- `Run`
	- Animation: `MF_Unarmed_Walk_Fwd`
- `Jump`
	- Animation: `MM_Jump`
- `Fall`
	- Animation: `MM_Fall_Loop`

The `Idle` and `Run` states transition between each other based on if the character is moving, which is determined by the WASD keys. Both states should be able to transition into the `Jump` state, which at the top of the jump transitions into the `Fall` state. `Idle` and `Run` can also transition to `Fall` if the floor underneath the player disappears (e.g. if running off a ledge).

#### State Alias

**State aliases** allows creating a source for multiple states to transition out of. They are *source-only*, so nothing transitions into a state alias. The following aliases are created for the existing states:

- State alias: `Grounded`
	- State: `Idle`
	- State: `Run`
- State alias: `Airborne`
	- State: `Jump`
	- State: `Fall`

| From               | To   | Rule                           |
| ------------------ | ---- | ------------------------------ |
| Idle               | Run  | `bShouldMove`                  |
| Run                | Idle | `!bShouldMove`                 |
| `Grounded` (alias) | Jump | `bIsJumping`                   |
| `Grounded` (alias) | Fall | `bIsAirborne && !bIsJumping`   |
| Jump               | Fall | `!bIsJumping`                  |
| `Airborne` (alias) | Idle | `!bIsAirborne && !bShouldMove` |
| `Airborne` (alias) | Run  | `!bIsAirborne && bShouldMove`  |

Instead of the direct transitions `Idle->Jump` and `Run->Jump`, `bIsJumping=true` will trigger a transition `Grounded->Jump` if the current state is either `Idle` or `Run`. This allows for less overall nodes and transitions, creating a cleaner AnimGraph. It also makes it easier to later add new states. If creating a `Crouch` state, instead of creating transitions to the `Jump` and `Fall` states it can just be added to the `Grounded` state alias.

There is a **Global Alias** checkbox in the alias's details panel that auto-includes all states (including future ones).

---
<br>

# Blend Spaces

A blend space is a lookup table over a continuous input space. Animation samples are placed at coordinates on a grid, input float values are provided at runtime, and it returns a pose blended from the nearest samples weighted by distance.

The blend space migrated from the third-person template project `BS_Idle_Walk_Run` is 2D, with the horizontal axis being the character's direction ranging from $[-180.0,180.0]$ and the vertical axis being the character's speed ranging from $[0.0,600.0]$. It uses 27 animations to blend idle, walk, and jog poses for the UE5 mannequin based on its current speed and direction. At speed 0.0 and direction 0.0, the character is idle and facing forward. At speed 450 and direction -90, it provides a blend of walk-left and jog-left.

The blend space is designed for a strafing setup, where the character faces the camera instead of facing the direction it moves. For now, the current implementation will be kept so that `Direction` always equals zero, and most of the animations will be ignored. Later when locking the character's direction, the animations can be used to strafe.

Settings that help make a blend space appear as a crossfade between animations:

- **Sync Groups**
	- Samples in the same group align normalized phase
	- e.g. the walk's left foot lands with the jog's right foot
- **Rate Scale**
	- If enabled per-sample, it time-warps clips so the feet match the blend weight
	- Without it, blending a 1.0-rate walk with a 1.0-rate jog (different stride length) gives two out-of-phase cycles and visible foot slide
- **Smoothing Time**
	- On the Speed axis, which damps jitter when speed oscillates near a sample

## Speed Transitions

The blend space is set up so that Idle animations use Speed=0.0, Walk animations use Speed=300.0, and Jog animations use Speed=600.0. If the `MaxWalkSpeed` is less than the max speed in the blend space, the character will never use all the animations.

**In the Code**:

The `MCCharacter` file was updated to include values for `WalkSpeed=300.0` and `SprintSpeed=600.0`. They were created `EditDefaultsOnly` to allow tuning in the editor against the blend space.

A new `IA_Sprint` action was created to change speeds and movement animations. When the action is triggered, it calls a `StartSprint()` function to update `MaxWalkSpeed=SprintSpeed`. When the action is completed or canceled, `StopSprint()` returns `MaxWalkSpeed=WalkSpeed`.

**In the Editor**:

- Create `IA_Sprint` with a `bool` value type
- Update `IMC_Default` to include a mapping to `IA_Sprint` on `LeftShift`
- Update `BP_MC` to tie the `SprintAction` pointer to `IA_Sprint`
- Open the AnimGraph of `ABP_MC`
	- Update the run state to use `BS_Idle_Walk_Run` as the Output Animation Pose
	- Use the `Direction` and `GroundSpeed` values from the `Locomotion` grouping (as assigned in the code) as the inputs to the blend space

The blend space is supposed to handle idle animations at a speed of 0.0, which should allow removal of the `Idle` animation state. However, there are various reasons to want an `Idle` animation state, such as having a place to implement idle-specific content. For now, `Idle` will remain. Come back later if idle ends up not needing its own state.

Accelerating from 300 to 600 speed reads fine as it ramps up to the new value. However, decelerating has a bit of a snap between the animations because the character movement component abruptly clamps from 600 to 300. The solution would be to interpolate the value of `MaxWalkSpeed` instead of directly assigning it, but this requires using `Tick()` and setting `bCanEverTick=true`. To avoid the performance hit, this will be left as-is for now. If `Tick()` gets implemented later, remember to add this in.

---