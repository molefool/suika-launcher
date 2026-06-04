// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#pragma once

#include <QString>

namespace Suika::ServerList {

const QString& defaultServerName();
const QString& defaultServerAddress();

QString gameRootForInstanceRoot(const QString& instanceRoot);
bool ensureDefaultServerEntry(const QString& gameRoot);
bool ensureDefaultServerEntryForInstanceRoot(const QString& instanceRoot);

}  // namespace Suika::ServerList
