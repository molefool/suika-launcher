// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#pragma once

#include <QDialog>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthFlow.h"
#include "minecraft/auth/MinecraftAccount.h"

class QLineEdit;
class QPushButton;

class UnifiedPassLoginDialog : public QDialog {
    Q_OBJECT

   public:
    static MinecraftAccountPtr newAccount(QWidget* parent);

   private:
    explicit UnifiedPassLoginDialog(QWidget* parent = nullptr);

    void accept() override;

   private slots:
    void browseAuthJar();
    void openRegisterPage();
    void updateRegisterButton();

   private:
    QString defaultAuthJarPath() const;
    bool serverIdLooksValid() const;

   private:
    QLineEdit* m_serverId = nullptr;
    QLineEdit* m_username = nullptr;
    QLineEdit* m_password = nullptr;
    QLineEdit* m_authJarPath = nullptr;
    QPushButton* m_registerButton = nullptr;

    MinecraftAccountPtr m_account;
    shared_qobject_ptr<AuthFlow> m_authflowTask;
};
