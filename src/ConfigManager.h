#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <obs.h>
#include <QObject>
#include <QString>

class ConfigManager : public QObject {
	Q_OBJECT

private:
	static constexpr const char *TOKEN_KEY = "twitch_access_token";
	static constexpr const char *USERNAME_KEY = "twitch_username";
	static constexpr const char *CLIENT_ID_KEY = "twitch_client_id";
	static constexpr const char *COMMAND_KEY = "twitch_command_message";
	static constexpr const char *MANUAL_GAMES_KEY = "manual_games_list";
	static constexpr const char *COMMAND_NO_GAME_KEY = "twitch_command_no_game";
	static constexpr const char *EXECUTE_AUTOMATICALLY_KEY = "execute_automatically";

	obs_data_t *settings = nullptr;

	explicit ConfigManager(QObject *parent = nullptr);

public:
	static ConfigManager &get();

	void load();
	void setSettings(obs_data_t *settings_data);

	void save(const QString &token, const QString &command);
	void save(obs_data_t *settings);
	void saveManualGames(obs_data_array_t *gamesArray);

	obs_data_t *getSettings() const;
	QString getToken() const;
	QString getClientId() const;
	QString getCommand() const;
	obs_data_array_t *getManualGames() const;
	QString getNoGameCommand() const;
	bool getExecuteAutomatically() const;
};

#endif // CONFIGMANAGER_H