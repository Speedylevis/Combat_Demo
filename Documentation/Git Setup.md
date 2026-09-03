**On GitHub**

- Create new repo
- Name
- Desription
- Do not create README
- Do not create .gitignore

**In project folder**

- `git init`
- `git lfs install`
- `git config user.name <name>`
- `git config user.email <email>`
- `ssh -T git@github.com`
	- Requires SSH keygen stored for the computer
	- Should output: `Hi <username>!...`

**Create `.gitignore` file**

```git
Binaries/
DerivedDataCache/
Intermediate/
Saved/
Build/
.vs/
.idea/
*.sln
*.slnx
*.suo
*.xcodeproj/
*.xcworkspace/
*.vsconfig
```

Commit the following: `Config/`, `Content/`, `Source/`, `Plugins/`, `.uproject`.

**Create `.gitattributes` file**

```git
*.uasset filter=lfs diff=lfs merge=lfs -text lockable
*.umap   filter=lfs diff=lfs merge=lfs -text lockable
*.fbx    filter=lfs diff=lfs merge=lfs -text
*.png    filter=lfs diff=lfs merge=lfs -text
*.tga    filter=lfs diff=lfs merge=lfs -text
*.wav    filter=lfs diff=lfs merge=lfs -text
*.mp4    filter=lfs diff=lfs merge=lfs -text
*.ttf    filter=lfs diff=lfs merge=lfs -text
*.udk    filter=lfs diff=lfs merge=lfs -text
```

The `lockable` flag marks files as read-only on checkout until someone takes a lock, which is what stops two people editing the same blueprint.

**Check setup before pushing to GitHub**

- `git add -A`
	- Shouldn't show anything yet
- `git status`
	- Should not show anything under `Binaries/`, `Intermediate/`, `Saved/`, `DerivedCache/`, `.vs/`, or a `.sln` or `.slnx`
	- Should show the following
```bash
On branch main

No commits yet

Changes to be committed:
  (use "git rm --cached <file>..." to unstage)
        new file:   .gitattributes
        new file:   .gitignore
        new file:   Combat_Demo.uproject
        new file:   Config/DefaultEditor.ini
        new file:   Config/DefaultEngine.ini
        new file:   Config/DefaultGame.ini
        new file:   Config/DefaultInput.ini
        new file:   Content/Levels/MainLevel.umap
        new file:   Source/Combat_Demo.Target.cs
        new file:   Source/Combat_Demo/Combat_Demo.Build.cs
        new file:   Source/Combat_Demo/Combat_Demo.cpp
        new file:   Source/Combat_Demo/Combat_Demo.h
        new file:   Source/Combat_DemoEditor.Target.cs
```

The `.umap` and any `.uasset` files should be listed under the LFS section. A more direct check on a single file is: `git check-attr filter -- Content/Levels/MainLevel.umap`. This should show `filter: lfs`. If it shows `filter: unspecified`, the `.gitattributes` pattern has failed.

**First commit**

```bash
git commit -m "Initial Commit: UE 5.8.2 C++ project - Combat_Demo"
git branch -M main
git remote add origin git@github.com:<user>/<repo>.git
git push -u origin main
```

**Collaborator Access**

- Repo -> Settings -> Collaborators
- `git lfs install`
- `git clone <repo-url>`
- If the project has C++, collaborators need Visual Studio with the "Game development with C++" workload and the correct MSVC toolchain for 5.8
	- On the first open, the editor will prompt to rebuild the game modules
- First launch will compile shaders for a long time
- Right-click the `.uproject`
	- Generate VS project files (recreates the `.sln`)
- Double-click `.uproject` and accept the prompt to rebuild the missing modules
- Before editing a `.umap` or `.uasset`:
	- `git lfs lock Content/Path/BP_Thing.uasset`
	- push changes
	- `git lfs unlock Content/Path/BP_Thing.uasset`
- Every time a plugin is added or the engine version is changed
	- The `.uproject` changes and all collaborators need to regenerate project files

---