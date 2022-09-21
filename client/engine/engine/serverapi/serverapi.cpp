#include "serverapi.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSslSocket>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>

#include "utils/ws_assert.h"
#include "utils/hardcodedsettings.h"
#include "engine/openvpnversioncontroller.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/ipvalidation.h"
#include "version/appversion.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"

#include "requests/loginrequest.h"
#include "requests/sessionrequest.h"
#include "requests/accessipsrequest.h"
#include "requests/serverlistrequest.h"
#include "requests/servercredentialsrequest.h"
#include "requests/deletesessionrequest.h"
#include "requests/serverconfigsrequest.h"
#include "requests/portmaprequest.h"
#include "requests/recordinstallrequest.h"
#include "requests/confirmemailrequest.h"
#include "requests/websessionrequest.h"

#ifdef Q_OS_LINUX
    #include "utils/linuxutils.h"
#endif

namespace
{
QUrlQuery MakeQuery(const QString &authHash, bool bAddOpenVpnVersion = false)
{
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrlQuery query;
    if (!authHash.isEmpty())
        query.addQueryItem("session_auth_hash", authHash);
    query.addQueryItem("time", strTimestamp);
    query.addQueryItem("client_auth_hash", md5Hash);
    if (bAddOpenVpnVersion)
        query.addQueryItem("ovpn_version",
                           OpenVpnVersionController::instance().getSelectedOpenVpnVersion());
    query.addQueryItem("platform", Utils::getPlatformNameSafe());
    query.addQueryItem("app_version", AppVersion::instance().semanticVersionString());

    return query;
}
} // namespace


namespace server_api {

ServerAPI::ServerAPI(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager) : QObject(parent),
    connectStateController_(connectStateController),
    networkAccessManager_(networkAccessManager),
    isProxyEnabled_(false),
    hostMode_(HOST_MODE_HOSTNAME),
    bIsRequestsEnabled_(false),
    curUserRole_(0),
    bIgnoreSslErrors_(false),
    myIpReply_(nullptr)
{

    if (QSslSocket::supportsSsl())
    {
        qCDebug(LOG_SERVER_API) << "SSL version:" << QSslSocket::sslLibraryVersionString();
    }
    else
    {
        qCDebug(LOG_SERVER_API) << "Fatal: SSL not supported";
    }
}

ServerAPI::~ServerAPI()
{
}

uint ServerAPI::getAvailableUserRole()
{
    uint userRole = curUserRole_;
    curUserRole_++;
    return userRole;
}

void ServerAPI::setProxySettings(const types::ProxySettings &proxySettings)
{
    proxySettings_ = proxySettings;
}

void ServerAPI::disableProxy()
{
    isProxyEnabled_ = false;
}

void ServerAPI::enableProxy()
{
    isProxyEnabled_ = true;
}

void ServerAPI::setRequestsEnabled(bool bEnable)
{
    qCDebug(LOG_SERVER_API) << "setRequestsEnabled:" << bEnable;
    bIsRequestsEnabled_ = bEnable;
}

bool ServerAPI::isRequestsEnabled() const
{
    return bIsRequestsEnabled_;
}

void ServerAPI::setHostname(const QString &hostname)
{
    qCDebug(LOG_SERVER_API) << "setHostname:" << hostname;
    hostMode_ = HOST_MODE_HOSTNAME;
    hostname_ = hostname;
}

QString ServerAPI::getHostname() const
{
    // if this is IP, return without change
    if (IpValidation::instance().isIp(hostname_))
    {
        return hostname_;
    }
    // otherwise return hostname without "api." appendix
    else
    {
        QString modifiedHostname = hostname_;
        if (modifiedHostname.startsWith("api.", Qt::CaseInsensitive))
        {
            modifiedHostname = modifiedHostname.remove(0, 4);
        }
        return modifiedHostname;
    }
}

// works with direct IP
BaseRequest *ServerAPI::accessIps(const QString &hostIp)
{
    AcessIpsRequest *request = new AcessIpsRequest(this, hostIp);
    executeRequest(request, false);
    return request;
}

BaseRequest *ServerAPI::login(const QString &username, const QString &password, const QString &code2fa)
{
    LoginRequest *request = new LoginRequest(this, hostname_, username, password, code2fa);
    executeRequest(request, false);
    return request;
}

BaseRequest *ServerAPI::session(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    SessionRequest *request = new SessionRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::serverLocations(const QString &language, bool isNeedCheckRequestsEnabled,
                                const QString &revision, bool isPro, PROTOCOL protocol, const QStringList &alcList)
{
    ServerListRequest *request = new ServerListRequest(this, hostname_, language, revision, isPro, protocol, alcList, connectStateController_);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::serverCredentials(const QString &authHash, PROTOCOL protocol, bool isNeedCheckRequestsEnabled)
{
    ServerCredentialsRequest *request = new ServerCredentialsRequest(this, hostname_, authHash, protocol);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::deleteSession(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    DeleteSessionRequest *request = new DeleteSessionRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::serverConfigs(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    ServerConfigsRequest *request = new ServerConfigsRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::portMap(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    PortMapRequest *request = new PortMapRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::recordInstall(bool isNeedCheckRequestsEnabled)
{
    RecordInstallRequest *request = new RecordInstallRequest(this, hostname_);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::confirmEmail(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    ConfirmEmailRequest *request = new ConfirmEmailRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::webSession(const QString authHash, WEB_SESSION_PURPOSE purpose, bool isNeedCheckRequestsEnabled)
{
    WebSessionRequest *request = new WebSessionRequest(this, hostname_, authHash, purpose);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

void ServerAPI::myIP(bool isDisconnected, uint userRole, bool isNeedCheckRequestsEnabled)
{
    // if previous myIp request not finished, mark that it should be ignored in handlers
    SAFE_DELETE_LATER(myIpReply_);

    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit myIPAnswer("N/A", false, isDisconnected, userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/MyIp?time=" + strTimestamp + "&client_auth_hash="
             + md5Hash + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), GET_MY_IP_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    myIpReply_ = networkAccessManager_->get(request);
    myIpReply_->setProperty("userRole", userRole);
    myIpReply_->setProperty("isDisconnected", isDisconnected);
    connect(myIpReply_, &NetworkReply::finished, this, &ServerAPI::handleMyIP);
}

void ServerAPI::checkUpdate(UPDATE_CHANNEL updateChannel, uint userRole, bool isNeedCheckRequestsEnabled)
{
    // This check will only be useful in the case that we expand our supported linux OSes and the platform flag is not added for that OS
    if (Utils::getPlatformName().isEmpty()) {
        qCDebug(LOG_SERVER_API) << "Check update failed: platform name is empty";
        emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
        return;
    }

    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        qCDebug(LOG_SERVER_API) << "Check update failed: API not ready";
        emit checkUpdateAnswer(types::CheckUpdate(), true, userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/CheckUpdate");
    QUrlQuery query;
    query.addQueryItem("time", strTimestamp);
    query.addQueryItem("client_auth_hash", md5Hash);
    query.addQueryItem("platform", Utils::getPlatformNameSafe());
    query.addQueryItem("app_version", AppVersion::instance().semanticVersionString());
    query.addQueryItem("version", AppVersion::instance().version());
    query.addQueryItem("build", AppVersion::instance().build());

    if (updateChannel == UPDATE_CHANNEL_BETA)
        query.addQueryItem("beta", "1");
    else if (updateChannel == UPDATE_CHANNEL_GUINEA_PIG)
        query.addQueryItem("beta", "2");
    else if (updateChannel == UPDATE_CHANNEL_INTERNAL)
        query.addQueryItem("beta", "3");

    // add OS version and build
    QString osVersion, osBuild;
    Utils::getOSVersionAndBuild(osVersion, osBuild);

    if (!osVersion.isEmpty())
        query.addQueryItem("os_version", osVersion);

    if (!osBuild.isEmpty())
        query.addQueryItem("os_build", osBuild);

    url.setQuery(query);

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleCheckUpdate);
}

void ServerAPI::debugLog(const QString &username, const QString &strLog, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit debugLogAnswer(SERVER_RETURN_API_NOT_READY, userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/Report/applog");

    QUrlQuery postData;
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("client_auth_hash", md5Hash);
    QByteArray ba = strLog.toUtf8();
    postData.addQueryItem("logfile", ba.toBase64());
    if (!username.isEmpty())
        postData.addQueryItem("username", username);
    postData.addQueryItem("platform", Utils::getPlatformNameSafe());
    postData.addQueryItem("app_version", AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: application/x-www-form-urlencoded");
    NetworkReply *reply = networkAccessManager_->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleDebugLog);
}

void ServerAPI::speedRating(const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        qCDebug(LOG_SERVER_API) << "SpeedRating request failed: API not ready";
        return;
    }

    QUrl url("https://" + hostname_ + "/SpeedRating");
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QString str = "time=" + strTimestamp + "&client_auth_hash=" + md5Hash + "&session_auth_hash="
                  + authHash + "&hostname=" + speedRatingHostname + "&rating="
                  + QString::number(rating) + "&ip=" + ip
                  + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString();

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: text/html; charset=utf-8");
    NetworkReply *reply = networkAccessManager_->post(request, str.toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleSpeedRating);
}

void ServerAPI::staticIps(const QString &authHash, const QString &deviceId, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit staticIpsAnswer(SERVER_RETURN_API_NOT_READY, apiinfo::StaticIps(), userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

#ifdef Q_OS_WIN
    QString strOs = "win";
#elif defined Q_OS_MAC
    QString strOs = "mac";
#elif defined Q_OS_LINUX
    QString strOs = "linux";
#endif

    qDebug() << "device_id for StaticIps request:" << deviceId;
    QUrl url("https://" + hostname_ + "/StaticIps?time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + authHash
             + "&device_id=" + deviceId + "&os=" + strOs
             + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleStaticIps);
}

void ServerAPI::pingTest(quint64 cmdId, uint timeout, bool bWriteLog)
{
    const QString kCheckIpHostname = HardcodedSettings::instance().serverTunnelTestUrl();

    if (bWriteLog)
        qCDebug(LOG_SERVER_API) << "Do ping test to:" << kCheckIpHostname << " with timeout: " << timeout;

    QString strUrl = QString("https://") + kCheckIpHostname;
    QUrl url(strUrl);

    NetworkRequest request(url.toString(), timeout, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("cmdId", cmdId);
    reply->setProperty("isWriteLog", bWriteLog);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handlePingTest);
    pingTestReplies_[cmdId] = reply;
}

void ServerAPI::cancelPingTest(quint64 cmdId)
{
    auto it = pingTestReplies_.find(cmdId);
    if (it != pingTestReplies_.end()) {
        it.value()->deleteLater();
        pingTestReplies_.erase(it);
    }
}

void ServerAPI::notifications(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit notificationsAnswer(SERVER_RETURN_API_NOT_READY, QVector<types::Notification>(), userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/Notifications?time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + authHash
             + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleNotifications);
}

void ServerAPI::wgConfigsInit(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled, const QString &clientPublicKey, bool deleteOldestKey)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit wgConfigsInitAnswer(SERVER_RETURN_API_NOT_READY, userRole, false, 0, QString(), QString());
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/WgConfigs/init");
    QUrlQuery postData;
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("client_auth_hash", md5Hash);
    postData.addQueryItem("session_auth_hash", authHash);
    // Must encode the public key in case it has '+' characters in its base64 encoding.  Otherwise the
    // server API will store the incorrect key and the wireguard handshake will fail due to a key mismatch.
    postData.addQueryItem("wg_pubkey", QUrl::toPercentEncoding(clientPublicKey));
    postData.addQueryItem("platform", Utils::getPlatformNameSafe());
    postData.addQueryItem("app_version", AppVersion::instance().semanticVersionString());
    if (deleteOldestKey)
        postData.addQueryItem("force_init", "1");

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: text/html; charset=utf-8");
    NetworkReply *reply = networkAccessManager_->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleWgConfigsInit);
}

void ServerAPI::wgConfigsConnect(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled,
                                 const QString &clientPublicKey, const QString &serverName, const QString &deviceId)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit wgConfigsConnectAnswer(SERVER_RETURN_API_NOT_READY, userRole, false, 0, QString(), QString());
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/WgConfigs/connect");

    QUrlQuery postData;
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("client_auth_hash", md5Hash);
    postData.addQueryItem("session_auth_hash", authHash);
    // Must encode the public key in case it has '+' characters in its base64 encoding.  Otherwise the
    // server API will store the incorrect key and the wireguard handshake will fail due to a key mismatch.
    postData.addQueryItem("wg_pubkey", QUrl::toPercentEncoding(clientPublicKey));
    postData.addQueryItem("hostname", serverName);
    postData.addQueryItem("platform", Utils::getPlatformNameSafe());
    postData.addQueryItem("app_version", AppVersion::instance().semanticVersionString());

    if (!deviceId.isEmpty()) {
        qDebug() << "Setting device_id for WgConfigs connect request:" << deviceId;
        postData.addQueryItem("device_id", deviceId);
    }

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: text/html; charset=utf-8");
    NetworkReply *reply = networkAccessManager_->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleWgConfigsConnect);
}

void ServerAPI::getRobertFilters(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit getRobertFiltersAnswer(SERVER_RETURN_API_NOT_READY, QVector<types::RobertFilter>(), userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/Robert/filters?time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + authHash
             + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleGetRobertFilters);
}

void ServerAPI::setRobertFilter(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled, const types::RobertFilter &filter)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit setRobertFilterAnswer(SERVER_RETURN_API_NOT_READY, userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/Robert/filter?time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + authHash
             + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString());

    QString json = QString("{\"filter\":\"%1\", \"status\":%2}").arg(filter.id).arg(filter.status);

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: text/html; charset=utf-8");
    NetworkReply *reply = networkAccessManager_->put(request, json.toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleSetRobertFilter);
}

void ServerAPI::setIgnoreSslErrors(bool bIgnore)
{
    bIgnoreSslErrors_ = bIgnore;
}

void ServerAPI::handleMyIP()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    WS_ASSERT(reply == myIpReply_);
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    myIpReply_ = nullptr;
    const uint userRole = reply->property("userRole").toUInt();
    const bool isDisconnected = reply->property("isDisconnected").toBool();

    if (!reply->isSuccess()) {
        emit myIPAnswer("N/A", false, isDisconnected, userRole);
        return;
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json";
            emit myIPAnswer("N/A", false, isDisconnected, userRole);
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json(data field not found)";
            emit myIPAnswer("N/A", false, isDisconnected, userRole);
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("user_ip")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json(user_ip field not found)";
            emit myIPAnswer("N/A", false, isDisconnected, userRole);
        }
        emit myIPAnswer(jsonData["user_ip"].toString(), true, isDisconnected, userRole);
    }
}

void ServerAPI::handleCheckUpdate()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "Check update failed(" << reply->errorString() << ")";
        emit checkUpdateAnswer(types::CheckUpdate(), true, userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();

        if (jsonObject.contains("errorCode")) {
            // Putting this debug line here to help us quickly troubleshoot errors returned from the
            // server API, which hopefully should be few and far between.
            qCDebugMultiline(LOG_SERVER_API) << arr;
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("update_needed_flag")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        int updateNeeded = jsonData["update_needed_flag"].toInt();
        if (updateNeeded != 1) {
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        if (!jsonData.contains("latest_version")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }
        if (!jsonData.contains("update_url")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        QString err;
        bool outSuccess;
        types::CheckUpdate cu = types::CheckUpdate::createFromApiJson(jsonData, outSuccess, err);
        if (!outSuccess) {
            qCDebug(LOG_SERVER_API) << err;
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        emit checkUpdateAnswer(cu, false, userRole);
    }
}

void ServerAPI::handleDebugLog()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "DebugLog request failed(" << reply->errorString() << ")";
        emit debugLogAnswer(SERVER_RETURN_NETWORK_ERROR, userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
            emit debugLogAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
            emit debugLogAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("success")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
            emit debugLogAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        int is_success = jsonData["success"].toInt();
        emit debugLogAnswer(is_success == 1 ? SERVER_RETURN_SUCCESS : SERVER_RETURN_INCORRECT_JSON, userRole);
    }
}

void ServerAPI::handleSpeedRating()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);

    if (!reply->isSuccess())
        qCDebug(LOG_SERVER_API) << "SpeedRating request failed(" << reply->errorString() << ")";
    else {
        qCDebug(LOG_SERVER_API) << "SpeedRating request successfully executed";
        QByteArray arr = reply->readAll();
        qCDebugMultiline(LOG_SERVER_API) << arr;
    }
}

void ServerAPI::handlePingTest()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const quint64 cmdId = reply->property("cmdId").toULongLong();
    const bool isWriteLog = reply->property("isWriteLog").toBool();

    pingTestReplies_.remove(cmdId);

    if (!reply->isSuccess()) {
        if (isWriteLog)
            qCDebug(LOG_SERVER_API) << "PingTest request failed(" << reply->errorString() << ")";
        emit pingTestAnswer(SERVER_RETURN_NETWORK_ERROR, "");
    } else {
        QByteArray arr = reply->readAll();
        emit pingTestAnswer(SERVER_RETURN_SUCCESS, arr);
    }
}

void ServerAPI::handleNotifications()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "Notifications request failed(" << reply->errorString() << ")";
        emit notificationsAnswer(SERVER_RETURN_NETWORK_ERROR, QVector<types::Notification>(), userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications";
            emit notificationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::Notification>(), userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications";
            emit notificationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::Notification>(), userRole);
            return;
        }

        QJsonObject jsonData =  jsonObject["data"].toObject();
        const QJsonArray jsonNotifications = jsonData["notifications"].toArray();

        QVector<types::Notification> notifications;

        for (const QJsonValue &value : jsonNotifications) {
            QJsonObject obj = value.toObject();
            types::Notification n;
            if (!n.initFromJson(obj)) {
                qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications (not all required fields)";
                emit notificationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::Notification>(), userRole);
                return;
            }
            notifications.push_back(n);
        }
        qCDebug(LOG_SERVER_API) << "Notifications request successfully executed";
        emit notificationsAnswer(SERVER_RETURN_SUCCESS, notifications, userRole);
    }
}

void ServerAPI::handleWgConfigsInit()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "WgConfigs init curl request failed(" << reply->errorString() << ")";
        emit wgConfigsInitAnswer(SERVER_RETURN_NETWORK_ERROR, userRole, false, 0, QString(), QString());
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed to parse JSON for WgConfigs init response";
            emit wgConfigsInitAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonObject = doc.object();

        if (jsonObject.contains("errorCode")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            int errorCode = jsonObject["errorCode"].toInt();
            qCDebug(LOG_SERVER_API) << "WgConfigs init failed:" << jsonObject["errorMessage"].toString() << "(" << errorCode << ")";
            emit wgConfigsInitAnswer(SERVER_RETURN_SUCCESS, userRole, true, errorCode, QString(), QString());
            return;
        }

        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs init JSON is missing the 'data' element";
            emit wgConfigsInitAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonData = jsonObject["data"].toObject();
        if (!jsonData.contains("success") || jsonData["success"].toInt(0) == 0) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs init JSON contains a missing or invalid 'success' field";
            emit wgConfigsInitAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonConfig = jsonData["config"].toObject();
        if (jsonConfig.contains("PresharedKey") && jsonConfig.contains("AllowedIPs")) {
            QString presharedKey = jsonConfig["PresharedKey"].toString();
            QString allowedIps   = jsonConfig["AllowedIPs"].toString();
            qCDebug(LOG_SERVER_API) << "WgConfigs/init json:" << doc.toJson(QJsonDocument::Compact);
            qCDebug(LOG_SERVER_API) << "WgConfigs init request successfully executed";
            emit wgConfigsInitAnswer(SERVER_RETURN_SUCCESS, userRole, false, 0, presharedKey, allowedIps);
        } else {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs init 'config' entry is missing required elements";
            emit wgConfigsInitAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
        }
    }
}

void ServerAPI::handleWgConfigsConnect()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "WgConfigs connect curl request failed(" << reply->errorString() << ")";
        emit wgConfigsConnectAnswer(SERVER_RETURN_NETWORK_ERROR, userRole, false, 0, QString(), QString());
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed to parse JSON for WgConfigs connect response";
            emit wgConfigsConnectAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonObject = doc.object();

        if (jsonObject.contains("errorCode")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            int errorCode = jsonObject["errorCode"].toInt();
            qCDebug(LOG_SERVER_API) << "WgConfigs connect failed:" << jsonObject["errorMessage"].toString() << "(" << errorCode << ")";
            emit wgConfigsConnectAnswer(SERVER_RETURN_SUCCESS, userRole, true, errorCode, QString(), QString());
            return;
        }

        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs connect JSON is missing the 'data' element";
            emit wgConfigsConnectAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonData = jsonObject["data"].toObject();
        if (!jsonData.contains("success") || jsonData["success"].toInt(0) == 0) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs connect JSON contains a missing or invalid 'success' field";
            emit wgConfigsConnectAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonConfig = jsonData["config"].toObject();
        if (jsonConfig.contains("Address") && jsonConfig.contains("DNS")) {
            QString ipAddress  = jsonConfig["Address"].toString();
            QString dnsAddress = jsonConfig["DNS"].toString();
            qCDebug(LOG_SERVER_API) << "WgConfigs connect request successfully executed";
            emit wgConfigsConnectAnswer(SERVER_RETURN_SUCCESS, userRole, false, 0, ipAddress, dnsAddress);

        } else {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs connect 'config' entry is missing required elements";
            emit wgConfigsConnectAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
        }
    }
}

void ServerAPI::handleGetRobertFilters()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "Get ROBERT filters request failed(" << reply->errorString() << ")";
        emit getRobertFiltersAnswer(SERVER_RETURN_NETWORK_ERROR, QVector<types::RobertFilter>(), userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for get ROBERT filters";
            emit getRobertFiltersAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::RobertFilter>(), userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for get ROBERT filters";
            emit getRobertFiltersAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::RobertFilter>(), userRole);
            return;
        }

        QJsonObject jsonData =  jsonObject["data"].toObject();
        const QJsonArray jsonFilters = jsonData["filters"].toArray();

        QVector<types::RobertFilter> filters;

        for (const QJsonValue &value : jsonFilters) {
            QJsonObject obj = value.toObject();

            types::RobertFilter f;
            if (!f.initFromJson(obj)) {
                qCDebug(LOG_SERVER_API) << "Failed parse JSON for get ROBERT filters (not all required fields)";
                emit getRobertFiltersAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::RobertFilter>(), userRole);
                return;
            }

            filters.push_back(f);
        }
        qCDebug(LOG_SERVER_API) << "Get ROBERT request successfully executed";
        emit getRobertFiltersAnswer(SERVER_RETURN_SUCCESS, filters, userRole);
    }
}

void ServerAPI::handleSetRobertFilter()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "Set ROBERT filter request failed(" << reply->errorString() << ")";
        emit setRobertFilterAnswer(SERVER_RETURN_NETWORK_ERROR, userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for set ROBERT filters";
            emit setRobertFilterAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for set ROBERT filters";
            emit setRobertFilterAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        QJsonObject jsonData =  jsonObject["data"].toObject();
        qCDebug(LOG_SERVER_API) << "Set ROBERT request successfully executed ( result =" << jsonData["success"] << ")";
        emit setRobertFilterAnswer((jsonData["success"] == 1) ? SERVER_RETURN_SUCCESS : SERVER_RETURN_INCORRECT_JSON, userRole);
    }
}

void ServerAPI::handleNetworkRequestFinished()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    BaseRequest *request = static_cast<BaseRequest*>(reply->property("request").value<void*>());

    if (!reply->isSuccess())
    {
        if (reply->error() ==  NetworkReply::NetworkError::SslError && !bIgnoreSslErrors_)
            request->setRetCode(SERVER_RETURN_SSL_ERROR);
        else
            request->setRetCode(SERVER_RETURN_NETWORK_ERROR);

        qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed(" << reply->errorString() << ")";
        emit request->finished();
    }
    else {
        request->handle(reply->readAll());
        emit request->finished();
    }
}

types::ProxySettings ServerAPI::currentProxySettings() const
{
    if (isProxyEnabled_) {
        return proxySettings_;
    } else {
        return types::ProxySettings();
    }
}

void ServerAPI::executeRequest(BaseRequest *request, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        QTimer::singleShot(0, this, [request] () {
            qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed: API not ready";
            request->setRetCode(SERVER_RETURN_API_NOT_READY);
            emit request->finished();
        });
        return;
    }

    NetworkRequest networkRequest(request->url().toString(), request->timeout(), true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply;
    switch (request->requestType()) {
        case RequestType::kGet:
            reply = networkAccessManager_->get(networkRequest);
            break;
        case RequestType::kPost:
            reply = networkAccessManager_->post(networkRequest, request->postData());
            break;
        case RequestType::kDelete:
            reply = networkAccessManager_->deleteResource(networkRequest);
            break;
        case RequestType::kPut:
            reply = networkAccessManager_->put(networkRequest, request->postData());
            break;
        default:
            WS_ASSERT(false);
    }

    reply->setProperty("request",  QVariant::fromValue(static_cast<void*>(request)));
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleNetworkRequestFinished);
}
void ServerAPI::handleStaticIps()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "StaticIps request failed(" << reply->errorString() << ")";
        emit staticIpsAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::StaticIps(), userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
            emit staticIpsAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::StaticIps(), userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
            emit staticIpsAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::StaticIps(), userRole);
            return;
        }

        QJsonObject jsonData =  jsonObject["data"].toObject();

        apiinfo::StaticIps staticIps;
        if (!staticIps.initFromJson(jsonData)) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
            emit staticIpsAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::StaticIps(), userRole);
            return;
        }

        qCDebug(LOG_SERVER_API) << "StaticIps request successfully executed";
        emit staticIpsAnswer(SERVER_RETURN_SUCCESS, staticIps, userRole);
    }
}

} // namespace server_api
