
#include "GetSkinStep.h"

#include <QNetworkRequest>

#include "Application.h"
#include "SuikaI18n.h"

GetSkinStep::GetSkinStep(AccountData* data) : AuthStep(data) {}

QString GetSkinStep::describe()
{
    return SuikaI18n::translate("GetSkinStep", "Getting skin.", "正在获取皮肤。");
}

void GetSkinStep::perform()
{
    if (m_data->minecraftProfile.skin.url.isEmpty()) {
        emit finished(AccountTaskState::STATE_WORKING, SuikaI18n::translate("GetSkinStep", "No skin URL.", "没有皮肤 URL。"));
        return;
    }

    QUrl url(m_data->minecraftProfile.skin.url);

    auto [request, response] = Net::Download::makeByteArray(url);
    m_request = request;
    m_request->enableAutoRetry(true);

    m_task.reset(new NetJob("GetSkinStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, [this, response] { onRequestDone(response); });

    m_task->start();
}

void GetSkinStep::onRequestDone(QByteArray* response)
{
    if (m_request->error() == QNetworkReply::NoError)
        m_data->minecraftProfile.skin.data = *response;
    emit finished(AccountTaskState::STATE_WORKING, SuikaI18n::translate("GetSkinStep", "Got skin", "已获取皮肤"));
}
