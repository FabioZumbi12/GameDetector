#ifndef TWITCHCHATBOT_H
#define TWITCHCHATBOT_H

#include <QObject>
#include <QString>

class QNetworkAccessManager;

class TwitchChatBot : public QObject {
	Q_OBJECT

private:
	QString token;
	QString broadcasterId; // ID do usuário do canal
	QString clientId;

	explicit TwitchChatBot(QObject *parent = nullptr);

	std::string getUserId(const std::string &token, const std::string &clientId);

public:
	static TwitchChatBot &get();

	// Envia a mensagem via API Helix
	void sendMessage(const QString &message);
};

#endif // TWITCHCHATBOT_H