#pragma once

#include "IPlatformService.h"
#include <QTcpServer>
#include <QFuture>
#include <QJsonObject>
#include <QTimer>

class TrovoAuthManager : public IPlatformService {
    Q_OBJECT

public:
    explicit TrovoAuthManager(QObject *parent = nullptr);
    ~TrovoAuthManager();

    void startAuthentication(); // Chamado pela UI se necessário
    bool isAuthenticated() const override;
    void updateCategory(const QString &gameName) override;
    void sendChatMessage(const QString &message) override;
    
    void loadToken();

signals:
    void authenticationFinished(bool success, QString message);
    void authenticationTimerTick(int remainingSeconds);

private:
    QTcpServer *server;
    QString accessToken;
    QString userId;
    bool isAuthenticating = false;

    QTimer *authTimeoutTimer = nullptr;
    int authRemainingSeconds = 0;

    QFuture<void> pendingTask;

    const QString AUTH_API_URL = "https://trovo-obs.areaz12server.net.br"; // SEU BACKEND
    const QString CLIENT_ID = "SEU_CLIENT_ID_DA_TROVO";

    void onNewConnection();
    void onAuthTimerTick();
    void exchangeCodeForToken(const QString &code);
    void fetchUserInfo();
    void searchAndSetCategory(const QString &gameName);

    QFuture<std::pair<long, QString>> performPOST(const QString &url, const QJsonObject &body, const QString &token);
    QFuture<std::pair<long, QString>> performGET(const QString &url, const QString &token);
};
