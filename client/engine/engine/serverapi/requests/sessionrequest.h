#pragma once

#include "baserequest.h"
#include "types/sessionstatus.h"

namespace server_api {

class SessionRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit SessionRequest(QObject *parent, const QString &hostname, const QString &authHash);

    QUrl url() const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    types::SessionStatus sessionStatus() const;

private:
    QString authHash_;

    // output values
    types::SessionStatus sessionStatus_;
};

} // namespace server_api {

