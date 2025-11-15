#include "TwitchChatBot.h"
#include "ConfigManager.h"
#include <obs-module.h>
#include <windows.h>
#include <string>

TwitchChatBot &TwitchChatBot::get()
{
	static TwitchChatBot instance;
	return instance;
}

TwitchChatBot::TwitchChatBot(QObject *parent) : QObject(parent) {}

struct PowerShellResult {
	DWORD exitCode;
	std::string output;
};

PowerShellResult executePowerShellCommand(const std::string &psCommand)
{
	std::string fullCmd = "powershell -NoLogo -NoProfile -Command \"" + psCommand + "\"";

	HANDLE hRead, hWrite;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
		blog(LOG_ERROR, "[OBSGameDetector/Twitch] Falha ao criar pipe para captura de saída.");
		return { (DWORD)-1, "Falha ao criar pipe." };
	}

	STARTUPINFOA si = {0};
	PROCESS_INFORMATION pi = {0};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdOutput = hWrite;
	si.hStdError = hWrite;

	if (!CreateProcessA(NULL, (LPSTR)fullCmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
		DWORD err = GetLastError();
		blog(LOG_ERROR, "[OBSGameDetector/Twitch] Falha ao executar PowerShell: %lu", err);
		CloseHandle(hRead);
		CloseHandle(hWrite);
		return { err, "Falha ao criar processo do PowerShell." };
	}

	CloseHandle(hWrite);
	WaitForSingleObject(pi.hProcess, INFINITE);

	std::string output;
	DWORD bytesAvailable = 0;
	while (PeekNamedPipe(hRead, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable > 0) {
		std::string buffer(bytesAvailable, '\0');
		DWORD bytesRead = 0;
		if (ReadFile(hRead, &buffer[0], bytesAvailable, &bytesRead, NULL) && bytesRead > 0) {
			output.append(buffer, 0, bytesRead);
		} else {
			break;
		}
	}

	DWORD exitCode;
	GetExitCodeProcess(pi.hProcess, &exitCode);

	CloseHandle(hRead);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	return { exitCode, output };
}

std::string TwitchChatBot::getUserId(const std::string &token, const std::string &clientId)
{
	std::string psCommand =
		"try { "
		"    $response = Invoke-RestMethod -Method GET 'https://api.twitch.tv/helix/users' "
		"-Headers @{ 'Authorization'='Bearer " +
		token + "'; 'Client-Id'='" + clientId +
		"' }; "
		"    return $response.data[0].id "
		"} catch { "
		"    Write-Error $_; exit 1 "
		"}";

	PowerShellResult result = executePowerShellCommand(psCommand);

	if (result.exitCode == 0) {
		result.output.erase(result.output.find_last_not_of(" \n\r\t") + 1);
		return result.output;
	} else {
		blog(LOG_ERROR, "[OBSGameDetector/Twitch] Falha ao obter User ID. Código de saída: %lu. Erro: %s", result.exitCode, result.output.c_str());
		return "";
	}
}

void TwitchChatBot::sendMessage(const QString &message)
{
	// blog(LOG_INFO, "[OBSGameDetector/Twitch] Enviando mensagem: %s", message.toUtf8().constData());

	this->token = ConfigManager::get().getToken();
	this->clientId = ConfigManager::get().getClientId();

	std::string messageStr = message.toUtf8().constData();
	std::string clientIdStr = clientId.toUtf8().constData();
	std::string tokenStr = token.toUtf8().constData();

	if (tokenStr.empty() || clientIdStr.empty()) {
		blog(LOG_WARNING, "[OBSGameDetector/Twitch] Token ou Client ID não configurado.");
		return;
	}

	// Obter o ID do usuário (broadcaster/sender)
	std::string userIdStr = getUserId(tokenStr, clientIdStr);
	if (userIdStr.empty()) {
		blog(LOG_ERROR, "[OBSGameDetector/Twitch] Não foi possível obter o ID do usuário. Verifique o token e o Client ID.");
		return;
	}

	// blog(LOG_INFO, "[OBSGameDetector/Twitch] User ID obtido: %s", userIdStr.c_str());

	// Escapa aspas duplas na mensagem para evitar quebrar o JSON/PowerShell
	std::string escapedMessageStr = messageStr;
	size_t pos = 0;
	while ((pos = escapedMessageStr.find("\"", pos)) != std::string::npos) {
		escapedMessageStr.replace(pos, 1, "\\\"");
		pos += 2;
	}

	std::string psCommand =
		"try { "
		"    $body = @{ broadcaster_id = '" + userIdStr + "'; sender_id = '" + userIdStr + "'; message = '" + escapedMessageStr + "' }; "
		"    $response = Invoke-RestMethod -Method POST 'https://api.twitch.tv/helix/chat/messages' "
		"        -Headers @{ 'Authorization'='Bearer " + tokenStr + "'; 'Client-Id'='" + clientIdStr + "' } "
		"        -Body ($body | ConvertTo-Json) -ContentType 'application/json'; "
		"    return $response | ConvertTo-Json -Depth 3; "
		"} catch { Write-Error $_; exit 1 }";


	PowerShellResult result = executePowerShellCommand(psCommand);

	if (result.exitCode == 0) {
		// blog(LOG_INFO, "[OBSGameDetector/Twitch] Mensagem enviada com sucesso. Resposta: %s", result.output.c_str());
	} else {
		blog(LOG_ERROR, "[OBSGameDetector/Twitch] Falha ao enviar mensagem. Código de saída: %lu. Resposta: %s", result.exitCode, result.output.c_str());
	}
}