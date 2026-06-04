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

class Nide8ProfileStep : public AuthStep {
    Q_OBJECT

   public:
    explicit Nide8ProfileStep(AccountData* data);
    ~Nide8ProfileStep() noexcept override = default;

    void perform() override;
    QString describe() override;

   private slots:
    void profileFinished(QByteArray* response);

   private:
    QUrl profileUrl() const;

   private:
    Net::Download::Ptr m_request;
    NetJob::Ptr m_task;
};
