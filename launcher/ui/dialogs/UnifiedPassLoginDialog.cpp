// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#include "UnifiedPassLoginDialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QUrl>
#include <QVBoxLayout>

#include "DesktopServices.h"
#include "SuikaI18n.h"
#include "minecraft/auth/Nide8AuthConstants.h"
#include "ui/dialogs/ProgressDialog.h"

namespace {
QString zhFallback(const char* source, const char* zhCN)
{
    return SuikaI18n::translate("UnifiedPassLoginDialog", source, zhCN);
}
}  // namespace

UnifiedPassLoginDialog::UnifiedPassLoginDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(zhFallback("Add Unified Pass Account", "添加统一通行证账号"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    m_serverId = new QLineEdit(this);
    m_serverId->setText(QString(Nide8Auth::DefaultServerId));
    m_serverId->hide();

    m_username = new QLineEdit(this);
    m_username->setPlaceholderText(zhFallback("Passport username or linked QQ email", "通行证用户名或绑定的 QQ 邮箱"));
    form->addRow(zhFallback("Username:", "用户名："), m_username);

    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    form->addRow(zhFallback("Password:", "密码："), m_password);

    auto* jarRow = new QWidget(this);
    auto* jarLayout = new QHBoxLayout(jarRow);
    jarLayout->setContentsMargins(0, 0, 0, 0);
    m_authJarPath = new QLineEdit(defaultAuthJarPath(), this);
    auto* browseButton = new QPushButton(zhFallback("Browse...", "浏览..."), this);
    connect(browseButton, &QPushButton::clicked, this, &UnifiedPassLoginDialog::browseAuthJar);
    jarLayout->addWidget(m_authJarPath);
    jarLayout->addWidget(browseButton);
    form->addRow(tr("nide8auth.jar:"), jarRow);

    layout->addLayout(form);

    auto* hint = new QLabel(
        zhFallback("Passwords are used only for this login request and are not saved. Usernames may contain any character supported by Unified "
                   "Pass.",
                   "密码只会用于本次登录请求，不会保存到本地。用户名不做字符限制，可输入统一通行证支持的任意字符。"),
        this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_registerButton = buttons->addButton(zhFallback("Register", "注册"), QDialogButtonBox::ActionRole);
    connect(buttons, &QDialogButtonBox::accepted, this, &UnifiedPassLoginDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &UnifiedPassLoginDialog::reject);
    connect(m_registerButton, &QPushButton::clicked, this, &UnifiedPassLoginDialog::openRegisterPage);
    layout->addWidget(buttons);

    updateRegisterButton();
}

QString UnifiedPassLoginDialog::defaultAuthJarPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QString(Nide8Auth::AuthJarFileName));
}

bool UnifiedPassLoginDialog::serverIdLooksValid() const
{
    static const QRegularExpression serverIdRegex(QStringLiteral("^[0-9A-Fa-f]{32}$"));
    return serverIdRegex.match(QString(Nide8Auth::DefaultServerId)).hasMatch();
}

void UnifiedPassLoginDialog::browseAuthJar()
{
    const auto path =
        QFileDialog::getOpenFileName(this, zhFallback("Select nide8auth.jar", "选择 nide8auth.jar"),
                                     QFileInfo(m_authJarPath->text()).absolutePath(),
                                     zhFallback("Java archives (*.jar);;All files (*)", "Java 归档 (*.jar);;所有文件 (*)"));
    if (!path.isEmpty()) {
        m_authJarPath->setText(path);
    }
}

void UnifiedPassLoginDialog::openRegisterPage()
{
    DesktopServices::openUrl(QUrl(QString("https://login.mc-user.com:233/%1/loginreg").arg(QString(Nide8Auth::DefaultServerId))));
}

void UnifiedPassLoginDialog::updateRegisterButton()
{
    m_registerButton->setEnabled(serverIdLooksValid());
}

void UnifiedPassLoginDialog::accept()
{
    if (m_username->text().isEmpty() || m_password->text().isEmpty()) {
        QMessageBox::warning(this, zhFallback("Missing credentials", "缺少登录信息"),
                             zhFallback("Please enter your Unified Pass username and password.", "请输入统一通行证用户名和密码。"));
        return;
    }

    m_account =
        MinecraftAccount::createBlankNide8(QString(Nide8Auth::DefaultServerId), m_username->text(), m_password->text(), m_authJarPath->text());
    m_authflowTask = m_account->login();

    ProgressDialog progress(this);
    progress.setSkipButton(true, zhFallback("Cancel", "取消"));
    progress.execWithTask(m_authflowTask.get());

    if (m_account->accountState() == AccountState::Online) {
        QDialog::accept();
    } else {
        m_account.reset();
    }
}

MinecraftAccountPtr UnifiedPassLoginDialog::newAccount(QWidget* parent)
{
    UnifiedPassLoginDialog dialog(parent);
    if (dialog.exec() == QDialog::Accepted) {
        return dialog.m_account;
    }
    return nullptr;
}
