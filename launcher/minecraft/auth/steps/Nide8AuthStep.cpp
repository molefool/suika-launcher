// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#include "Nide8AuthStep.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUuid>

#include "Application.h"
#include "BuildConfig.h"
#include "minecraft/auth/Nide8AuthConstants.h"
#include "net/RawHeaderProxy.h"

namespace {
QString normalizedRoot(QString root)
{
    if (!root.endsWith('/')) {
        root += '/';
    }
    return root;
}

QList<Net::HeaderPair> jsonHeaders()
{
    return {
        { "Content-Type", "application/json" },
        { "Accept", "application/json" },
    };
}

QJsonObject objectFromJson(const QByteArray& data, QString& errorMessage)
{
    QJsonParseError jsonError;
    const auto doc = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !doc.isObject()) {
        errorMessage = QObject::tr("Unified Pass response could not be parsed as JSON: %1").arg(jsonError.errorString());
        return {};
    }
    return doc.object();
}
}  // namespace

Nide8AuthStep::Nide8AuthStep(AccountData* data, bool refresh) : AuthStep(data), m_refresh(refresh) {}

QString Nide8AuthStep::describe()
{
    return m_refresh ? tr("Refreshing Unified Pass account.") : tr("Logging in with Unified Pass.");
}

QUrl Nide8AuthStep::defaultServiceRoot() const
{
    const auto serverId = m_data->nide8ServerId.trimmed().isEmpty() ? QString(Nide8Auth::DefaultServerId) : m_data->nide8ServerId.trimmed();
    return QUrl(QString("https://auth.mc-user.com:233/%1/").arg(serverId));
}

QUrl Nide8AuthStep::apiEndpoint(const QString& path) const
{
    const auto root = normalizedRoot(m_data->nide8ApiRoot.isEmpty() ? defaultServiceRoot().toString() : m_data->nide8ApiRoot);
    return QUrl(root).resolved(QUrl(path));
}

QString Nide8AuthStep::clientToken() const
{
    const auto saved = m_data->yggdrasilToken.extra.value("clientToken").toString();
    if (!saved.isEmpty()) {
        return saved;
    }
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void Nide8AuthStep::perform()
{
    if (m_data->nide8ServerId.trimmed().isEmpty()) {
        m_data->nide8ServerId = QString(Nide8Auth::DefaultServerId);
    }
    requestMetadata();
}

void Nide8AuthStep::requestMetadata()
{
    auto [request, response] = Net::Download::makeByteArray(defaultServiceRoot());
    m_download = request;
    m_task.reset(new NetJob("Nide8Metadata", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_download);
    connect(m_task.get(), &Task::finished, this, [this, response] { metadataFinished(response); });
    m_task->start();
}

void Nide8AuthStep::metadataFinished(QByteArray* response)
{
    if (m_download->error() != QNetworkReply::NoError) {
        emit finished(AccountTaskState::STATE_OFFLINE, m_download->errorString());
        return;
    }
    if (!parseMetadata(*response)) {
        return;
    }

    if (m_refresh && !m_data->yggdrasilToken.token.isEmpty()) {
        requestValidate();
    } else {
        requestAuthenticate();
    }
}

bool Nide8AuthStep::parseMetadata(const QByteArray& response)
{
    QString errorMessage;
    const auto root = objectFromJson(response, errorMessage);
    if (root.isEmpty()) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT, errorMessage);
        return false;
    }

    const auto apiRoot = root.value("apiRoot").toString();
    if (!apiRoot.isEmpty()) {
        m_data->nide8ApiRoot = normalizedRoot(apiRoot);
    } else {
        m_data->nide8ApiRoot = defaultServiceRoot().toString();
    }

    const auto meta = root.value("meta").toObject();
    m_data->nide8ServerName = meta.value("serverName").toString();
    m_data->nide8ServerAddress = meta.value("serverIP").toString();
    m_data->nide8JarHash = root.value("jarHash").toString();
    return true;
}

void Nide8AuthStep::requestAuthenticate()
{
    QJsonObject agent;
    agent["name"] = BuildConfig.LAUNCHER_DISPLAYNAME;
    agent["version"] = BuildConfig.printableVersionString();

    QJsonObject body;
    body["agent"] = agent;
    body["username"] = m_data->nide8Username;
    body["password"] = m_data->nide8Password;
    body["clientToken"] = clientToken();
    body["requestUser"] = true;

    auto [request, response] = Net::Upload::makeByteArray(apiEndpoint("authserver/authenticate"), QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_upload = request;
    m_upload->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(jsonHeaders()));
    m_task.reset(new NetJob("Nide8Authenticate", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_upload);
    connect(m_task.get(), &Task::finished, this, [this, response] { authenticateFinished(response); });
    m_task->start();
}

void Nide8AuthStep::authenticateFinished(QByteArray* response)
{
    if (m_upload->error() != QNetworkReply::NoError) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, responseErrorMessage(*response, m_upload->errorString()));
        return;
    }
    QString errorMessage;
    if (!parseTokenResponse(*response, errorMessage)) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, errorMessage);
        return;
    }
    emit finished(AccountTaskState::STATE_WORKING, tr("Logged in with Unified Pass."));
}

void Nide8AuthStep::requestValidate()
{
    QJsonObject body;
    body["accessToken"] = m_data->yggdrasilToken.token;
    body["clientToken"] = clientToken();

    auto [request, response] = Net::Upload::makeByteArray(apiEndpoint("authserver/validate"), QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_upload = request;
    m_upload->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(jsonHeaders()));
    m_task.reset(new NetJob("Nide8Validate", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_upload);
    connect(m_task.get(), &Task::finished, this, [this, response] { validateFinished(response); });
    m_task->start();
}

void Nide8AuthStep::validateFinished(QByteArray* response)
{
    if (m_upload->replyStatusCode() == 204 || m_upload->error() == QNetworkReply::NoError) {
        emit finished(AccountTaskState::STATE_WORKING, tr("Unified Pass token is valid."));
        return;
    }
    requestRefresh();
}

void Nide8AuthStep::requestRefresh()
{
    QJsonObject body;
    body["accessToken"] = m_data->yggdrasilToken.token;
    body["clientToken"] = clientToken();
    body["requestUser"] = true;

    auto [request, response] = Net::Upload::makeByteArray(apiEndpoint("authserver/refresh"), QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_upload = request;
    m_upload->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(jsonHeaders()));
    m_task.reset(new NetJob("Nide8Refresh", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_upload);
    connect(m_task.get(), &Task::finished, this, [this, response] { refreshFinished(response); });
    m_task->start();
}

void Nide8AuthStep::refreshFinished(QByteArray* response)
{
    if (m_upload->error() != QNetworkReply::NoError) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, responseErrorMessage(*response, m_upload->errorString()));
        return;
    }
    QString errorMessage;
    if (!parseTokenResponse(*response, errorMessage)) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, errorMessage);
        return;
    }
    emit finished(AccountTaskState::STATE_WORKING, tr("Refreshed Unified Pass token."));
}

bool Nide8AuthStep::parseTokenResponse(const QByteArray& response, QString& errorMessage)
{
    const auto root = objectFromJson(response, errorMessage);
    if (root.isEmpty()) {
        return false;
    }

    const auto accessToken = root.value("accessToken").toString();
    const auto returnedClientToken = root.value("clientToken").toString(clientToken());
    const auto selectedProfile = root.value("selectedProfile").toObject();
    const auto profileId = selectedProfile.value("id").toString();
    const auto profileName = selectedProfile.value("name").toString();

    if (accessToken.isEmpty() || profileId.isEmpty() || profileName.isEmpty()) {
        errorMessage = responseErrorMessage(response, tr("Unified Pass response did not include a complete token/profile."));
        return false;
    }

    m_data->yggdrasilToken.token = accessToken;
    m_data->yggdrasilToken.validity = Validity::Certain;
    m_data->yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc();
    m_data->yggdrasilToken.extra["clientToken"] = returnedClientToken;
    m_data->minecraftProfile.id = profileId;
    m_data->minecraftProfile.name = profileName;
    m_data->minecraftProfile.validity = Validity::Certain;
    m_data->minecraftEntitlement.canPlayMinecraft = true;
    m_data->minecraftEntitlement.ownsMinecraft = true;
    m_data->minecraftEntitlement.validity = Validity::Assumed;
    m_data->nide8Password.clear();
    return true;
}

QString Nide8AuthStep::responseErrorMessage(const QByteArray& response, const QString& fallback) const
{
    QString parseError;
    const auto root = objectFromJson(response, parseError);
    const auto message = root.value("errorMessage").toString();
    const auto error = root.value("error").toString();
    if (!message.isEmpty() && !error.isEmpty()) {
        return QString("%1: %2").arg(error, message);
    }
    if (!message.isEmpty()) {
        return message;
    }
    return fallback.isEmpty() ? parseError : fallback;
}
