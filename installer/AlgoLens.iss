; ============================================================================
;  AlgoLens — Inno Setup 6 installer script
;
;  HOW TO COMPILE:
;    1. Assemble the "stage" folder next to this script (see STAGING below).
;    2. Open this file in the Inno Setup Compiler and press Compile,
;       or run:  iscc AlgoLens.iss
;    3. The installer is written to  installer\output\
;
;  STAGING (installer\stage\ must contain a ready-to-run AlgoLens):
;    - AlgoLens.exe, AlgoLensRenderer.exe
;    - All CEF runtime files (libcef.dll, *.pak, icudtl.dat, snapshot_blob.bin,
;      v8_context_snapshot.bin, chrome_elf.dll, locales\, etc.) + SDL3.dll
;    - app\                 (the Vue production build — copy algolens\dist\* here)
;    - algolens-server\     (with node_modules — runs under bun)
;    - algolens-terminal\   (with node_modules incl. node-pty built for node.exe)
;    - bun.exe, node.exe    (bundled runtimes)
;    - python\              (optional embeddable Python for code execution)
; ============================================================================

#define MyAppName "AlgoLens"
#define MyAppVersion "0.2.0"
#define MyAppPublisher "Siddhesh Dupare"
#define MyAppExeName "AlgoLens.exe"
#define StageDir "stage"

[Setup]
; AppId MUST stay the same across versions so updates install in place.
AppId={{A1B2C3D4-E5F6-47A8-9B0C-1D2E3F4A5B6C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion} Beta
AppPublisher={#MyAppPublisher}
WizardStyle=modern
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#MyAppExeName}
OutputDir=output
OutputBaseFilename=AlgoLens-Setup-{#MyAppVersion}-beta
Compression=lzma2/max
SolidCompression=yes
MinVersion=10.0
; Per-user install: no admin prompt, and in-place updates work cleanly.
PrivilegesRequired=lowest
; Inno 6.3+: x64compatible. If you have an older Inno 6, change both to: x64
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Close a running AlgoLens before updating (needed for the in-app updater).
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{userdesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
