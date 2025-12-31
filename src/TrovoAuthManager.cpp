#include "TrovoAuthManager.h"
#include "ConfigManager.h"
#include <obs-module.h>
#include <curl/curl.h>
#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>

static size_t trovo_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    ((std::string *)userp)->append((char *)contents, realsize);
    return realsize;
}

TrovoAuthManager::TrovoAuthManager(QObject *parent) : IPlatformService(parent)
{
    server = new QTcpServer(this);
    authTimeoutTimer = new QTimer(this);
    connect(server, &QTcpServer::newConnection, this, &TrovoAuthManager::onNewConnection);
    connect(authTimeoutTimer, &QTimer::timeout, this, &TrovoAuthManager::onAuthTimerTick);
    loadToken();
}

TrovoAuthManager::~TrovoAuthManager()
{
    if (server->isListening()) server->close();
    pendingTask.waitForFinished();
}

void TrovoAuthManager::loadToken()
{
    auto settings = ConfigManager::get().getSettings();
    accessToken = ConfigManager::get().getTrovoToken();
    userId = ConfigManager::get().getTrovoUserId();
}

bool TrovoAuthManager::isAuthenticated() const
{
    return !accessToken.isEmpty() && !userId.isEmpty();
}

void TrovoAuthManager::startAuthentication()
{
    if (isAuthenticating) return;
    
    if (server->isListening()) server->close();

    if (!server->listen(QHostAddress::LocalHost, 31000)) return;

    QUrl authUrl("https://open.trovo.live/page/login.html");
    QUrlQuery query;
    query.addQueryItem("client_id", CLIENT_ID);
    query.addQueryItem("response_type", "code");
    query.addQueryItem("scope", "channel_update_self user_details_my chat_send_self");
    query.addQueryItem("redirect_uri", AUTH_API_URL);
    authUrl.setQuery(query);
    QDesktopServices::openUrl(authUrl);
    isAuthenticating = true;

    authRemainingSeconds = 30;
    emit authenticationTimerTick(authRemainingSeconds);
    authTimeoutTimer->start(1000);
}

void TrovoAuthManager::onAuthTimerTick()
{
    authRemainingSeconds--;
    emit authenticationTimerTick(authRemainingSeconds);

    if (authRemainingSeconds <= 0) {
        authTimeoutTimer->stop();
        if (server->isListening()) server->close();
        isAuthenticating = false;
        emit authenticationFinished(false, "Timeout");
    }
}

void TrovoAuthManager::onNewConnection()
{
    QTcpSocket *clientSocket = server->nextPendingConnection();
    if (!clientSocket) return;

    authTimeoutTimer->stop();
    emit authenticationTimerTick(0);

    connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket]() {
        QString request = clientSocket->readAll();
        QStringList reqLines = request.split("\r\n");
        if (reqLines.isEmpty()) return;

        QString firstLine = reqLines.first();
        QString path = firstLine.split(" ")[1];
        QUrl url(path);
        QUrlQuery query(url.query());
        QString code = query.queryItemValue("code");

        if (!code.isEmpty()) {
            QString msg = obs_module_text("Auth.LoginAuthorized");
            clientSocket->write("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + msg.toUtf8());
            clientSocket->disconnectFromHost();
            server->close();
            isAuthenticating = false;
            exchangeCodeForToken(code);
        }
    });
}

void TrovoAuthManager::exchangeCodeForToken(const QString &code)
{
    QJsonObject json;
    json["code"] = code;
    auto future = performPOST(AUTH_API_URL, json, "");
    
    pendingTask = QtConcurrent::run([this, future]() {
        auto result = future.result();
        if (result.first == 200) {
            QJsonDocument doc = QJsonDocument::fromJson(result.second.toUtf8());
            this->accessToken = doc.object()["access_token"].toString();
            fetchUserInfo();
        }
    });
}

void TrovoAuthManager::fetchUserInfo()
{
    auto future = performGET("https://open-api.trovo.live/openplatform/validate", accessToken);
    pendingTask = QtConcurrent::run([this, future]() {
        auto result = future.result();
        if (result.first == 200) {
            QJsonDocument doc = QJsonDocument::fromJson(result.second.toUtf8());
            this->userId = doc.object()["uid"].toString();
            QString nickName = doc.object()["nick_name"].toString();
            
            ConfigManager::get().setTrovoToken(accessToken);
            ConfigManager::get().setTrovoUserId(userId);
            ConfigManager::get().setTrovoChannelLogin(nickName);
            ConfigManager::get().save(ConfigManager::get().getSettings());
            emit authenticationFinished(true, nickName);
        } else {
            emit authenticationFinished(false, obs_module_text("Auth.Error.GetUserIdFailed"));
        }
    });
}

void TrovoAuthManager::updateCategory(const QString &gameName)
{
    int actionMode = ConfigManager::get().getActionMode();

    if (actionMode == 0) { // Chat Command
        QString cmd;
        if (gameName == "Just Chatting") {
            cmd = ConfigManager::get().getNoGameCommand();
        } else {
            cmd = ConfigManager::get().getCommand();
            cmd.replace("{game}", gameName);
        }
        if (!cmd.isEmpty()) sendChatMessage(cmd);
        emit categoryUpdateFinished(true, gameName, "Command sent");
        return;
    }

    if (!isAuthenticated()) {
        emit categoryUpdateFinished(false, gameName, obs_module_text("Trovo.Error.NotAuthenticated"));
        return;
    }
    searchAndSetCategory(gameName);
}

void TrovoAuthManager::searchAndSetCategory(const QString &gameName)
{
    QString searchTerm = gameName;
    if (gameName == "Just Chatting") {
        searchTerm = "Chit Chat";
    }

    QJsonObject body;
    body["query"] = searchTerm;
    body["limit"] = 1;
    auto future = performPOST("https://open-api.trovo.live/openplatform/searchcategory", body, accessToken);

    pendingTask = QtConcurrent::run([this, future, gameName]() {
        auto result = future.result();
        QString categoryId;
        if (result.first == 200) {
            QJsonDocument doc = QJsonDocument::fromJson(result.second.toUtf8());
            QJsonArray list = doc.object()["category_info"].toArray();
            if (!list.isEmpty()) categoryId = list.first().toObject()["id"].toString();
        }

        if (categoryId.isEmpty()) {
            emit categoryUpdateFinished(false, gameName, obs_module_text("Trovo.Error.GameNotFound"));
            return;
        }

        QJsonObject updateBody;
        updateBody["channel_id"] = this->userId;
        updateBody["category_id"] = categoryId;
        auto updateFuture = performPOST("https://open-api.trovo.live/openplatform/channels/update", updateBody, accessToken);
        
        if (updateFuture.result().first == 200) {
            emit categoryUpdateFinished(true, gameName, "");
        } else {
            emit categoryUpdateFinished(false, gameName, obs_module_text("Trovo.Error.UpdateFailed"));
        }
    });
}

void TrovoAuthManager::sendChatMessage(const QString &message)
{
    if (!isAuthenticated()) return;
    QJsonObject body;
    body["content"] = message;
    body["channel_id"] = userId;
    performPOST("https://open-api.trovo.live/openplatform/chat/send", body, accessToken);
}

QFuture<std::pair<long, QString>> TrovoAuthManager::performPOST(const QString &url, const QJsonObject &body, const QString &token)
{
    return QtConcurrent::run([this, url, body, token]() -> std::pair<long, QString> {
        CURL *curl = curl_easy_init();
        if (!curl) return {0, ""};
        long http_code = 0;
        std::string response;
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, ("Client-ID: " + CLIENT_ID.toStdString()).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!token.isEmpty()) {
            std::string auth = "Authorization: OAuth " + token.toStdString();
            headers = curl_slist_append(headers, auth.c_str());
        }
        QJsonDocument doc(body);
        std::string json = doc.toJson(QJsonDocument::Compact).toStdString();
        curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, trovo_curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return {http_code, QString::fromStdString(response)};
    });
}

QFuture<std::pair<long, QString>> TrovoAuthManager::performGET(const QString &url, const QString &token)
{
    return QtConcurrent::run([this, url, token]() -> std::pair<long, QString> {
        CURL *curl = curl_easy_init();
        if (!curl) return {0, ""};
        long http_code = 0;
        std::string response;
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, ("Client-ID: " + CLIENT_ID.toStdString()).c_str());
        if (!token.isEmpty()) {
            std::string auth = "Authorization: OAuth " + token.toStdString();
            headers = curl_slist_append(headers, auth.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, trovo_curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return {http_code, QString::fromStdString(response)};
    });
}
