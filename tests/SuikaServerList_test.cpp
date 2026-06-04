// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <io/stream_reader.h>
#include <minecraft/SuikaServerList.h>
#include <tag_compound.h>
#include <tag_list.h>
#include <tag_string.h>

#include <memory>
#include <sstream>

class SuikaServerListTest : public QObject {
    Q_OBJECT

   private:
    std::unique_ptr<nbt::tag_compound> readServersDat(const QString& filename)
    {
        QByteArray input = FS::read(filename);
        std::istringstream stream(std::string(input.constData(), input.size()));
        auto pair = nbt::io::read_compound(stream);
        return std::move(pair.second);
    }

   private slots:
    void test_writesDefaultServer()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString gameRoot = FS::PathCombine(tempDir.path(), "minecraft");
        QVERIFY(Suika::ServerList::ensureDefaultServerEntry(gameRoot));

        auto root = readServersDat(FS::PathCombine(gameRoot, "servers.dat"));
        QVERIFY(root);
        QVERIFY(root->has_key("servers", nbt::tag_type::List));

        auto& servers = root->at("servers").as<nbt::tag_list>();
        QCOMPARE(servers.size(), size_t(1));

        auto& server = servers[0].as<nbt::tag_compound>();
        const std::string name(server["name"]);
        const std::string address(server["ip"]);
        QCOMPARE(QString::fromUtf8(name.c_str()), Suika::ServerList::defaultServerName());
        QCOMPARE(QString::fromUtf8(address.c_str()), Suika::ServerList::defaultServerAddress());
    }

    void test_doesNotDuplicateDefaultServer()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString gameRoot = FS::PathCombine(tempDir.path(), "minecraft");
        QVERIFY(Suika::ServerList::ensureDefaultServerEntry(gameRoot));
        QVERIFY(Suika::ServerList::ensureDefaultServerEntry(gameRoot));

        auto root = readServersDat(FS::PathCombine(gameRoot, "servers.dat"));
        auto& servers = root->at("servers").as<nbt::tag_list>();
        QCOMPARE(servers.size(), size_t(1));
    }
};

QTEST_GUILESS_MAIN(SuikaServerListTest)

#include "SuikaServerList_test.moc"
