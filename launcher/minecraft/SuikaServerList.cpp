// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#include "SuikaServerList.h"

#include "FileSystem.h"

#include <io/stream_reader.h>
#include <io/stream_writer.h>
#include <tag_compound.h>
#include <tag_list.h>
#include <tag_string.h>

#include <QDebug>
#include <QFileInfo>

#include <memory>
#include <sstream>

namespace {

std::unique_ptr<nbt::tag_compound> parseServersDat(const QString& filename)
{
    try {
        QByteArray input = FS::read(filename);
        std::istringstream stream(std::string(input.constData(), input.size()));
        auto pair = nbt::io::read_compound(stream);

        if (pair.first != "") {
            return nullptr;
        }

        return std::move(pair.second);
    } catch (...) {
        return nullptr;
    }
}

bool serializeServersDat(const QString& filename, nbt::tag_compound& root)
{
    try {
        if (!FS::ensureFilePathExists(filename)) {
            return false;
        }

        std::ostringstream stream;
        nbt::io::write_tag("", root, stream);
        const auto serialized = stream.str();
        FS::write(filename, QByteArray(serialized.data(), (int)serialized.size()));
        return true;
    } catch (...) {
        return false;
    }
}

bool sameAddress(const QString& lhs, const QString& rhs)
{
    return lhs.trimmed().compare(rhs.trimmed(), Qt::CaseInsensitive) == 0;
}

}  // namespace

namespace Suika::ServerList {

const QString& defaultServerName()
{
    static const QString name = QStringLiteral("西瓜幻想乡群组服务器");
    return name;
}

const QString& defaultServerAddress()
{
    static const QString address = QStringLiteral("mc.suika.fun");
    return address;
}

QString gameRootForInstanceRoot(const QString& instanceRoot)
{
    QFileInfo mcDir(FS::PathCombine(instanceRoot, "minecraft"));
    QFileInfo dotMCDir(FS::PathCombine(instanceRoot, ".minecraft"));

    if (dotMCDir.exists() && !mcDir.exists()) {
        return dotMCDir.filePath();
    }
    return mcDir.filePath();
}

bool ensureDefaultServerEntryForInstanceRoot(const QString& instanceRoot)
{
    return ensureDefaultServerEntry(gameRootForInstanceRoot(instanceRoot));
}

bool ensureDefaultServerEntry(const QString& gameRoot)
{
    if (gameRoot.isEmpty()) {
        qWarning() << "Suika default server cannot be added without a game root.";
        return false;
    }

    if (!FS::ensureFolderPathExists(gameRoot)) {
        qWarning() << "Failed to create game root for Suika default server:" << gameRoot;
        return false;
    }

    const QString path = FS::PathCombine(gameRoot, "servers.dat");
    const bool existed = QFileInfo::exists(path);
    auto root = existed ? parseServersDat(path) : std::make_unique<nbt::tag_compound>();

    if (!root) {
        qWarning() << "Failed to parse existing servers.dat; keeping it unchanged:" << path;
        return false;
    }

    nbt::tag_list* serversList = nullptr;
    if (root->has_key("servers")) {
        if (!root->has_key("servers", nbt::tag_type::List)) {
            qWarning() << "servers.dat has a non-list 'servers' tag; keeping it unchanged:" << path;
            return false;
        }

        auto& list = root->at("servers").as<nbt::tag_list>();
        if (list.el_type() != nbt::tag_type::Null && list.el_type() != nbt::tag_type::Compound) {
            qWarning() << "servers.dat has a non-compound server list; keeping it unchanged:" << path;
            return false;
        }
        serversList = &list;
    } else {
        nbt::tag_list list(nbt::tag_type::Compound);
        root->put("servers", nbt::value(std::move(list)));
        serversList = &root->at("servers").as<nbt::tag_list>();
    }

    for (auto& serverValue : *serversList) {
        auto& server = serverValue.as<nbt::tag_compound>();
        if (!server.has_key("ip", nbt::tag_type::String)) {
            continue;
        }

        const std::string addressString(server["ip"]);
        if (sameAddress(QString::fromUtf8(addressString.c_str()), defaultServerAddress())) {
            return true;
        }
    }

    nbt::tag_compound server;
    server.insert("name", defaultServerName().trimmed().toUtf8().toStdString());
    server.insert("ip", defaultServerAddress().trimmed().toUtf8().toStdString());
    serversList->push_back(std::move(server));

    if (!serializeServersDat(path, *root)) {
        qWarning() << "Failed to save Suika default server to servers.dat:" << path;
        return false;
    }

    qDebug() << "Added Suika default server" << defaultServerAddress() << "to" << path;
    return true;
}

}  // namespace Suika::ServerList
