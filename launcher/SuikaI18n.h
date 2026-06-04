// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#pragma once

#include <QCoreApplication>
#include <QLocale>
#include <QString>

#include "Application.h"
#include "settings/SettingsObject.h"

namespace SuikaI18n {

inline QString playerFacingName()
{
    return QStringLiteral("西瓜幻想乡");
}

inline bool useSimplifiedChinese()
{
    if (APPLICATION && APPLICATION->settings()) {
        const auto language = APPLICATION->settings()->get("Language").toString();
        if (language.startsWith(QLatin1String("zh"))) {
            return true;
        }
        if (!language.isEmpty()) {
            return false;
        }
    }
    return QLocale::system().language() == QLocale::Chinese;
}

inline QString translate(const char* context, const char* source, const char* zhCN)
{
    return useSimplifiedChinese() ? QString::fromUtf8(zhCN) : QCoreApplication::translate(context, source);
}

}  // namespace SuikaI18n
