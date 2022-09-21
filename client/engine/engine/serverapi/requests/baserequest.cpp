#include "baserequest.h"

#include <QCryptographicHash>

#include "version/appversion.h"
#include "utils/utils.h"
#include "utils/hardcodedsettings.h"


namespace server_api {

BaseRequest::BaseRequest(QObject *parent, RequestType requestType, const QString &hostname) : QObject(parent),
    requestType_(requestType), hostname_(hostname)
{
}

BaseRequest::BaseRequest(QObject *parent, RequestType requestType, const QString &hostname, bool isUseFailover, int timeout): QObject(parent),
    requestType_(requestType),
    hostname_(hostname),
    bUseFailover_(isUseFailover),
    timeout_(timeout)
{
}

QString BaseRequest::contentTypeHeader() const
{
    return QString();
}

QByteArray BaseRequest::postData() const
{
    return QByteArray();
}

void BaseRequest::addPlatformQueryItems(QUrlQuery &urlQuery) const
{
    urlQuery.addQueryItem("platform", Utils::getPlatformNameSafe());
    urlQuery.addQueryItem("app_version", AppVersion::instance().semanticVersionString());
}

void BaseRequest::addAuthQueryItems(QUrlQuery &urlQuery, const QString &authHash) const
{
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();
    if (!authHash.isEmpty())
        urlQuery.addQueryItem("session_auth_hash", authHash);
    urlQuery.addQueryItem("time", strTimestamp);
    urlQuery.addQueryItem("client_auth_hash", md5Hash);
}

RequestType BaseRequest::requestType() const
{
    return requestType_;
}

int BaseRequest::timeout() const
{
    return timeout_;
}

void BaseRequest::setRetCode(SERVER_API_RET_CODE retCode)
{
    retCode_ = retCode;
}

SERVER_API_RET_CODE BaseRequest::retCode() const
{
    return retCode_;
}

} // namespace server_api {
