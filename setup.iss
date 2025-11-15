; Script do Inno Setup para o plugin OBS Game Detector
; Desenvolvido por FabioZumbi12

[Setup]
; --- Informações Básicas do Aplicativo ---
AppName=OBS Game Detector
AppVersion=1.0.0
AppPublisher=FabioZumbi12
AppPublisherURL=https://github.com/FabioZumbi12/OBSGameDetector
AppSupportURL=https://github.com/FabioZumbi12/OBSGameDetector/issues
AppUpdatesURL=https://github.com/FabioZumbi12/OBSGameDetector/releases

; --- Opções do Instalador ---
; DefaultDirName será sobrescrito pela seção [Code] para o caminho do OBS Studio.
DefaultDirName={autopf}\obs-studio
; O nome do executável final do setup.
OutputDir=setup
OutputBaseFilename=OBSGameDetector-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
; Privilégios de administrador são necessários para escrever na pasta 'Arquivos de Programas'.
PrivilegesRequired=admin
DirExistsWarning=no
UninstallFilesDir={app}\OBSGameDetector-Uninstaller
UninstallDisplayName=OBS Game Detector

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Copia a DLL do plugin para a pasta de plugins do OBS.
Source: "setup\obs-plugins\64bit\obs-game-detector.dll"; DestDir: "{app}\obs-plugins\64bit\"; Flags: ignoreversion

; Copia a pasta 'data' do plugin para a pasta de dados do OBS.
Source: "setup\data\obs-plugins\obs-game-detector\*"; DestDir: "{app}\data\obs-plugins\obs-game-detector\"; Flags: ignoreversion recursesubdirs createallsubdirs

[Run]
; Opcional: Oferece a opção de abrir o OBS Studio após a instalação.
Filename: "{app}\bin\64bit\obs64.exe"; Description: "Iniciar OBS Studio agora"; Flags: nowait postinstall shellexec

[UninstallDelete]
; Garante que todos os arquivos e pastas sejam removidos na desinstalação.
Type: files; Name: "{app}\obs-plugins\64bit\obs-game-detector.dll"
Type: filesandordirs; Name: "{app}\data\obs-plugins\obs-game-detector"

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
    MsgBox(
      'A instalação do OBS Studio não foi encontrada automaticamente.'#13#10 +
      'Você poderá selecionar a pasta manualmente.',
      mbInformation, MB_OK
    );
  end;

  Result := True;
end;

procedure InitializeWizard();
begin
  if ObsPath <> '' then
    WizardForm.DirEdit.Text := ObsPath;  // AGORA É SEGURO
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  TargetDir: string;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    TargetDir := ExpandConstant('{userappdata}\obs-studio\plugin_config\obs-game-detector');

    if DirExists(TargetDir) then
    begin
      if MsgBox(
        'Deseja também remover os arquivos de configuração adicionais?' #13#10 +
        TargetDir,
        mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
      begin
        DelTree(TargetDir, True, True, True);
      end;
    end;
  end;
end;