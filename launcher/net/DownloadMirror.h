// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#pragma once

#include <QString>
#include <QUrl>

namespace Net::DownloadMirror {

enum class Source { Official, BMCLAPI };

QString sourceToString(Source source);
Source sourceFromString(const QString& source);
Source currentSource();

QUrl rewriteUrl(QUrl url);
QUrl rewriteUrl(QUrl url, Source source);

void setSourceOverrideForTests(Source source);
void clearSourceOverrideForTests();

}  // namespace Net::DownloadMirror
