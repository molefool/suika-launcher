// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#include "Nide8ProfileStep.h"

#include <QNetworkReply>
#include <QNetworkRequest>

#include "Application.h"
#include "SuikaI18n.h"
#include "minecraft/auth/Nide8AuthConstants.h"
#include "minecraft/auth/Parsers.h"
#include "net/RawHeaderProxy.h"

namespace {
QString normalizedRoot(QString root)
{
    if (!root.endsWith('/')) {
        root += '/';
    }
    return root;
}

QString zhFallback(const char* source, const char* zhCN)
{
    return SuikaI18n::translate("Nide8ProfileStep", source, zhCN);
}
}  // namespace

Nide8ProfileStep::Nide8ProfileStep(AccountData* data) : AuthStep(data) {}

QString Nide8ProfileStep::describe()
{
    return zhFallback("Fetching Unified Pass profile.", "正在获取统一通行证玩家资料。");
}

QUrl Nide8ProfileStep::profileUrl() const
{
    const auto fallbackRoot = QString("https://auth.mc-user.com:233/%1/").arg(QString(Nide8Auth::DefaultServerId));
    const auto root = normalizedRoot(m_data->nide8ApiRoot.isEmpty() ? fallbackRoot : m_data->nide8ApiRoot);
    auto url = QUrl(root).resolved(QUrl(QString("sessionserver/session/minecraft/profile/%1").arg(m_data->minecraftProfile.id)));
    url.setQuery(QStringLiteral("unsigned=false"));
    return url;
}

void Nide8ProfileStep::perform()
{
    if (m_data->minecraftProfile.id.isEmpty()) {
        emit finished(AccountTaskState::STATE_WORKING, zhFallback("Unified Pass profile ID is empty.", "统一通行证玩家 UUID 为空。"));
        return;
    }

    auto [request, response] = Net::Download::makeByteArray(profileUrl());
    m_request = request;
    m_request->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(QList<Net::HeaderPair>{
        { "Accept", "application/json" },
    }));
    m_task.reset(new NetJob("Nide8ProfileStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);
    connect(m_task.get(), &Task::finished, this, [this, response] { profileFinished(response); });
    m_task->start();
}

void Nide8ProfileStep::profileFinished(QByteArray* response)
{
    if (m_request->error() == QNetworkReply::NoError) {
        MinecraftProfile profile = m_data->minecraftProfile;
        if (Parsers::parseMinecraftProfileMojang(*response, profile)) {
            m_data->minecraftProfile.skin = profile.skin;
            m_data->minecraftProfile.currentCape = profile.currentCape;
            m_data->minecraftProfile.capes = profile.capes;
        }
    }
    emit finished(AccountTaskState::STATE_WORKING, zhFallback("Got Unified Pass profile.", "已获取统一通行证玩家资料。"));
}
