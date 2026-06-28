; Inno Setup — instalador do DANTE CLI (Windows 10 1809+/11, x64).
; Empacota a saída do windeployqt (deploy\DanteCLI) num setup.exe.
; Build (CI): ISCC instalado via choco; saída em installer\Output\.

[Setup]
AppName=DANTE CLI
AppVersion=0.0.2
AppPublisher=Dante Testa
DefaultDirName={autopf}\DANTE CLI
DefaultGroupName=DANTE CLI
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=DanteCLI-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\deploy\DanteCLI\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs

[Icons]
Name: "{group}\DANTE CLI"; Filename: "{app}\DanteCLI.exe"
Name: "{autodesktop}\DANTE CLI"; Filename: "{app}\DanteCLI.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Criar atalho na área de trabalho"; GroupDescription: "Atalhos:"

[Run]
Filename: "{app}\DanteCLI.exe"; Description: "Abrir o DANTE CLI"; Flags: nowait postinstall skipifsilent
