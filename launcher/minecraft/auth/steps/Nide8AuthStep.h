// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#pragma once

#include <QObject>

#include "minecraft/auth/AuthStep.h"
#include "net/Download.h"
#include "net/NetJob.h"
#include "net/Upload.h"

class Nide8AuthStep : public AuthStep {
    Q_OBJECT

   public:
    explicit Nide8AuthStep(AccountData* data, bool refresh);
    ~Nide8AuthStep() noexcept override = default;

    void perform() override;
    QString describe() override;

   private slots:
    void metadataFinished(QByteArray* response);
    void authenticateFinished(QByteArray* response);
    void validateFinished(QByteArray* response);
    void refreshFinished(QByteArray* response);

   private:
    QUrl defaultServiceRoot() const;
    QUrl apiEndpoint(const QString& path) const;

    void requestMetadata();
    void requestAuthenticate();
    void requestValidate();
    void requestRefresh();

    bool parseMetadata(const QByteArray& response);
    bool parseTokenResponse(const QByteArray& response, QString& errorMessage);
    QString responseErrorMessage(const QByteArray& response, const QString& fallback) const;
    QString clientToken() const;

   private:
    bool m_refresh = false;
    Net::Download::Ptr m_download;
    Net::Upload::Ptr m_upload;
    NetJob::Ptr m_task;
};
