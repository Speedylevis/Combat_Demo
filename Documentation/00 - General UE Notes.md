# Basic Project Creation

New Unreal Project
- Blank template
- C++

Scene Setup
- New level
	- Create "Level" folder in the content drawer
	- Edit -> Project Settings -> Project -> Maps & Modes
		- Update the Editor Startup Map, Game Default Map
- Add -> Shapes -> Plane
	- Origin (0,0,0)
	- Scale (20,20,20)
- Add -> Visual Effects -> Sky Atmosphere
- Add -> Lights -> Directional Lights

---
<br>

# Creating Files

Custom folders and files are created in the module folder for the project:

```
- <Project_Name>/
  - Source/
    - <Project_Name>.Target.cs
    - <Project_Name>Editor.Target.cs
    - <Project_Name> // Module folder
      - <Project_Name>.cpp
      - <Project_Name>.h
      - <Project_Name>.Build.cs
      - Characters/
        - MCCharacter.cpp
        - MCCharacter.h
	  - GameModes
	    - GMBMain.cpp
	    - GMBMain.h
```

Create C++ classes in Rider by right-clicking the solution explorer and selecting `Add > "Unreal class"`. This allows creating a C++ class with a parent Unreal class. It will create a `.h` and `.cpp` file with templated code and automatically names classes and modules. Create C++ classes from the editor using `Tools > New C++ Class`.

Blueprint classes can be created based on C++ classes, but not the other way around. Because it is more difficult to reparent an existing Blueprint class to a new C++ class, starting with a C++ class is often a better approach. A C++ class `GMBMain` was created derived from `GameModeBase`, but no code was implemented yet. It was purely to allow creating the Blueprint based on this class, and the default pawn was assigned in the editor.

Unreal uses a set of prefixes to identify data:

- `A`: is used for actors
- `U`: is used for other `UObject`s
- `F`: is used for plain structs
- `I`: is used for interfaces
- `E`: is used for enums
- `T`: is used for templates

## Reloading Data

If files exist on disk, but not in the Solution view, the project files need to be regenerated. This can be done by right-clicking the `.uproject` file in Windows Explorer and selecting `Generate Visual Studio project files`. Rider should typically handle this itself, but may be required when pulling Git files or manually moving things around. Regenerating the project files allows the IDE to update based on the code and engine.

Changes to the code such as adding/deleting/renaming data requires rebuilding the project. This requires closing the editor and the project can be built from the IDE.

Quick adjustments and small changes may allow for Live Coding from within the editor using `Ctrl+Alt+F11`. This typically refers to editing an existing function body. I've found this to have somewhat limited use cases, typically just iteration of function logic. But this should be used where possible to avoid having to close/reopen the editor.

---
<br>

# Unreal Engine C++ Classes

Deriving a class from (e.g. from `ACharacter`) provides basic functions for override such as `BeginPlay` and `Tick`. It will name the DLL export as `<Module_Name>_API` and update the class name to denote it as an actor (e.g. `MCCharacter.cpp` will create class `AMCCharacter`).

The `<filename>.generated.h` include is required to be the last `#include` in the list for reflection to work. And `GENERATED_BODY()` should be the first line of the class.

The `.h` and `.cpp` files will access variables and functions from various files in the engine's source code. When `#include` is provided to a file, the compiler has to include all the data from the attached file. Every include in a header propagates to every file that includes that header, and in a large project this can greatly slow builds. Instead, the approach is to `#include` in the `.cpp` files and use forward declarations in the headers. The pointer or reference in the header file only needs the compiler to know the name exists, not the full class layout.

```C++
// In MCCharacter.h
#include "CoreMinimal.h"  
#include "GameFramework/Character.h"  
#include "MCCharacter.generated.h"  
  
class USpringArmComponent;  
class UCameraComponent;  
class UInputMappingContext;  
class UInputAction;  
struct FInputActionValue;

// In MCCharacter.cpp
#include "MCCharacter.h"  
  
#include "EnhancedInputComponent.h"  
#include "EnhancedInputSubsystems.h"  
#include "Camera/CameraComponent.h"  
#include "Components/CapsuleComponent.h"  
#include "GameFramework/CharacterMovementComponent.h"  
#include "GameFramework/SpringArmComponent.h"
```

#### Visibility

The visibility of a variable or function controls who can access that data:

- `public` means anyone
- `protected` means classes and its subclasses
- `private` means the class alone, or friend classes

Deriving from a class allows a child class to override any virtual functions from the parent class listed as public or protected. If doing so, typically match the visibility of the parent class for a given function (unless there is a reason not to do so). If the data does not need to be accessed outside the class, mark it private. If the class is expected to be derived from later, data can be marked as protected to allow those derived classes access.

---
<br>

# UE Reflection

The [Unreal Engine Reflection System](https://dev.epicgames.com/documentation/unreal-engine/reflection-system-in-unreal-engine) uses macros to provide engine and editor functionality to custom C++ code. `UCLASS()` tags classes derived from `UObject` to use UE's garbage collection, serialization, initialization, etc.

#### UPROPERTY

`UPROPERTY` allows creating C++ variables with metadata and specifiers. It provides editor exposure, Blueprint access, serialization, replication, and garbage collection tracking. Any `UObject`-derived pointer stored as a member must be a `UPROPERTY`. Otherwise the garbage collector won't know the reference exists and might collect the object before it is done.

Skip `UPROPERTY` in the following cases:

- Local variables and function parameters
- Plain data that doesn't need to be exposed
- Types the reflection system can't handle
	- `TSharedPtr`, `TUniquePtr`, `std::`, containers, raw structs that aren't `USTRUCT`

#### TObjectPtr

`TObjectPtr<T>` is UE5's replacement for raw pointers in `UPROPERTY` members. `TObjectPtr` for `UPROPERTY` members, raw pointers everywhere else.

- In the editor
	- It adds access tracking and lazy load resolution
		- Supports incremental cooking
- In packaged builds
	- It compiles down to a plain pointer with no overhead

#### UFUNCTION

`UFUNCTION` is only needed when something must find the function through reflection:

- `BlueprintCallable`/`BlueprintPure`
	- Callable from a BP Graph
- `BlueprintImplementableEvent`/`BlueprintNativeEvent`
	- Overridable in BP
- Dynamic Delegate Binding
	- `AddDynamic` looks up the target by name at runtime and requires `UFUNCTION`
- `Exec`
	- Console commands

#### USTRUCT

`USTRUCT` is for plain-data types that need to be in the reflection system (e.g. visible in the details panel, usable as a BP struct, replicated, serialized). This can create named groups the details panel which can help when tuning values.

If data is not needed by the editor or a Blueprint, a plain C++ struct is lighter and fine to use. The same logic applies to `UENUM`.

---
<br>

# Migrating and Importing Data

Data from existing projects can be migrated to other projects. The following allows using the basic UE5 mannequin and animations from the third-person template project:

- Create a new project
	- Third-person template
	- Blueprint
	- Combat variant
- From the Content Browser
	- Content > Characters > Mannequins
	- Migrate folder to target project

---