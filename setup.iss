; Script do Inno Setup para o plugin OBS Game Detector
; Desenvolvido por FabioZumbi12

[Setup]
; --- Informações Básicas do Aplicativo ---
AppName=Game Detector OBS Plugin
AppVersion=1.0.0
AppPublisher=FabioZumbi12
AppPublisherURL=https://github.com/FabioZumbi12/game-detector
AppSupportURL=https://github.com/FabioZumbi12/game-detector/issues
AppUpdatesURL=https://github.com/FabioZumbi12/game-detector/releases

; --- Opções do Instalador ---
; DefaultDirName será sobrescrito pela seção [Code] para o caminho do OBS Studio.
DefaultDirName={autopf}\obs-studio
; O nome do executável final do setup.
OutputDir=setup
OutputBaseFilename=GameDetector-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
; Privilégios de administrador são necessários para escrever na pasta 'Arquivos de Programas'.
PrivilegesRequired=admin
DirExistsWarning=no
UninstallFilesDir={app}\GameDetector-Uninstaller
UninstallDisplayName=Game Detector
SetupIconFile=img\game.ico
UninstallDisplayIcon={uninstallexe}

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
english.RemoveConfig=Do you also want to remove the additional configuration files?
brazilianportuguese.RemoveConfig=Deseja também remover os arquivos de configuração adicionais?
english.OBSNotFound=OBS Studio installation not found automatically.#13#10You will be able to select the folder manually.
brazilianportuguese.OBSNotFound=A instalação do OBS Studio não foi encontrada automaticamente.#13#10Você poderá selecionar a pasta manualmente.
english.LaunchOBS=Launch OBS Studio now
brazilianportuguese.LaunchOBS=Iniciar OBS Studio agora

[Files]
; Copia a DLL do plugin para a pasta de plugins do OBS.
Source: "setup\obs-plugins\64bit\game-detector.dll"; DestDir: "{app}\obs-plugins\64bit\"; Flags: ignoreversion

; Copia a pasta 'data' do plugin para a pasta de dados do OBS.
Source: "setup\data\obs-plugins\game-detector\*"; DestDir: "{app}\data\obs-plugins\game-detector\"; Flags: ignoreversion recursesubdirs createallsubdirs

[Run]
; Opcional: Oferece a opção de abrir o OBS Studio após a instalação.
Filename: "{app}\bin\64bit\obs64.exe"; Description: "{cm:LaunchOBS}"; Flags: nowait postinstall shellexec

[UninstallDelete]
; Garante que todos os arquivos e pastas sejam removidos na desinstalação.
Type: files; Name: "{app}\obs-plugins\64bit\game-detector.dll"
Type: filesandordirs; Name: "{app}\data\obs-plugins\game-detector"

[Code]
var
  ObsPath: string;

function InitializeSetup(): Boolean;
begin
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\OBS Studio', '', ObsPath) then
  begin
  end
  else
  begin
    MsgBox(CustomMessage('OBSNotFound'), mbInformation, MB_OK);
  end;

  Result := True;
end;

procedure InitializeWizard();
begin
  if ObsPath <> '' then
    WizardForm.DirEdit.Text := ObsPath;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  TargetDir: string;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    TargetDir := ExpandConstant('{userappdata}\obs-studio\plugin_config\game-detector');

    if DirExists(TargetDir) then
    begin
      if MsgBox(
        CustomMessage('RemoveConfig') + #13#10 + TargetDir,
        mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
      begin
        DelTree(TargetDir, True, True, True);
      end;
    end;
  end;
end;