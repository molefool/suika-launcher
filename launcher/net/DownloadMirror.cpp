// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#include "net/DownloadMirror.h"

#if defined(LAUNCHER_APPLICATION)
#include "Application.h"
#include "settings/SettingsObject.h"
#endif

namespace Net::DownloadMirror {
namespace {
constexpr auto kOfficialSource = "Official";
constexpr auto kBMCLAPISource = "BMCLAPI";
constexpr auto kBMCLAPI2Host = "bmclapi2.bangbang93.com";

bool g_sourceOverrideEnabled = false;
Source g_sourceOverride = Source::Official;

bool isObjectDownloadPath(const QString& path)
{
    return path.startsWith(QLatin1String("/v1/objects/"));
}

QString prefixedPath(QString prefix, QString path)
{
    if (!path.startsWith('/')) {
        path.prepend('/');
    }
    if (prefix.endsWith('/')) {
        prefix.chop(1);
    }
    return prefix + path;
}

QUrl withBMCLAPIHost(QUrl url, const QString& path, const char* host = kBMCLAPI2Host)
{
    url.setScheme(QStringLiteral("https"));
    url.setHost(QString::fromLatin1(host));
    url.setPath(path);
    return url;
}

QString stripPrefix(QString path, const QString& prefix)
{
    if (path.startsWith(prefix)) {
        path.remove(0, prefix.size());
        if (path.isEmpty()) {
            return QStringLiteral("/");
        }
    }
    return path;
}
}  // namespace

QString sourceToString(Source source)
{
    return source == Source::Official ? QString::fromLatin1(kOfficialSource) : QString::fromLatin1(kBMCLAPISource);
}

Source sourceFromString(const QString& source)
{
    if (source.compare(QLatin1String(kOfficialSource), Qt::CaseInsensitive) == 0) {
        return Source::Official;
    }
    return Source::BMCLAPI;
}

Source currentSource()
{
    if (g_sourceOverrideEnabled) {
        return g_sourceOverride;
    }

#if defined(LAUNCHER_APPLICATION)
    if (auto* app = APPLICATION_DYN; app && app->settings()) {
        return sourceFromString(app->settings()->get("DownloadSource").toString());
    }
#endif

    return Source::Official;
}

QUrl rewriteUrl(QUrl url)
{
    return rewriteUrl(url, currentSource());
}

QUrl rewriteUrl(QUrl url, Source source)
{
    if (source == Source::Official || !url.isValid()) {
        return url;
    }

    const QString host = url.host().toLower();
    const QString path = url.path();

    if ((host == QLatin1String("launchermeta.mojang.com") || host == QLatin1String("piston-meta.mojang.com")) &&
        path.startsWith(QLatin1String("/v1/packages/"))) {
        return withBMCLAPIHost(url, path);
    }

    if ((host == QLatin1String("launcher.mojang.com") || host == QLatin1String("piston-data.mojang.com")) &&
        (isObjectDownloadPath(path) || path.startsWith(QLatin1String("/mc/game/")))) {
        return withBMCLAPIHost(url, path);
    }

    if (host == QLatin1String("resources.download.minecraft.net")) {
        return withBMCLAPIHost(url, prefixedPath(QStringLiteral("/assets"), path));
    }

    if (host == QLatin1String("libraries.minecraft.net") || host == QLatin1String("maven.fabricmc.net") ||
        host == QLatin1String("maven.minecraftforge.net")) {
        return withBMCLAPIHost(url, prefixedPath(QStringLiteral("/maven"), path));
    }

    if (host == QLatin1String("files.minecraftforge.net")) {
        if (path.startsWith(QLatin1String("/maven"))) {
            return withBMCLAPIHost(url, path);
        }
    }

    if (host == QLatin1String("maven.quiltmc.org")) {
        return withBMCLAPIHost(url, prefixedPath(QStringLiteral("/maven"), stripPrefix(path, QStringLiteral("/repository/release"))));
    }

    if (host == QLatin1String("maven.neoforged.net") && path.startsWith(QLatin1String("/releases/"))) {
        return withBMCLAPIHost(url, prefixedPath(QStringLiteral("/maven"), stripPrefix(path, QStringLiteral("/releases"))));
    }

    return url;
}

void setSourceOverrideForTests(Source source)
{
    g_sourceOverride = source;
    g_sourceOverrideEnabled = true;
}

void clearSourceOverrideForTests()
{
    g_sourceOverrideEnabled = false;
}

}  // namespace Net::DownloadMirror
