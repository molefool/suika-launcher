// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Suika Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Suika Launcher Contributors
 */

#include <QTest>

#include <net/DownloadMirror.h>

class DownloadMirrorTest : public QObject {
    Q_OBJECT

   private slots:
    void cleanup() { Net::DownloadMirror::clearSourceOverrideForTests(); }

    void test_officialKeepsOriginal()
    {
        const QUrl url("https://libraries.minecraft.net/com/mojang/authlib/1.5.22/authlib-1.5.22.jar");
        QCOMPARE(Net::DownloadMirror::rewriteUrl(url, Net::DownloadMirror::Source::Official), url);
    }

    void test_bmclapiMappings()
    {
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://resources.download.minecraft.net/ab/abcdef"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/assets/ab/abcdef"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://libraries.minecraft.net/com/mojang/authlib/1.5.22/authlib-1.5.22.jar"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/maven/com/mojang/authlib/1.5.22/authlib-1.5.22.jar"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://piston-data.mojang.com/v1/objects/hash/client.jar"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/v1/objects/hash/client.jar"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://launcher.mojang.com/v1/objects/hash/java"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/v1/objects/hash/java"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(
                     QUrl("https://launchermeta.mojang.com/v1/products/java-runtime/2ec0cc96c44e5a76b9c8b7c39df7210883d12871/all.json"),
                     Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/v1/products/java-runtime/2ec0cc96c44e5a76b9c8b7c39df7210883d12871/all.json"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/mc/game/version_manifest_v2.json"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://files.minecraftforge.net/maven/net/minecraftforge/forge/1.20.1/forge.jar"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/maven/net/minecraftforge/forge/1.20.1/forge.jar"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://files.minecraftforge.net/net/minecraftforge/forge/promotions_slim.json"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/maven/net/minecraftforge/forge/promotions_slim.json"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://meta.fabricmc.net/v2/versions/loader"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/fabric-meta/v2/versions/loader"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://maven.fabricmc.net/net/fabricmc/fabric-loader/0.16.0/fabric-loader.jar"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/maven/net/fabricmc/fabric-loader/0.16.0/fabric-loader.jar"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://meta.quiltmc.org/v3/versions/loader"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/quilt-meta/v3/versions/loader"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://maven.quiltmc.org/repository/release/org/quiltmc/quilt-loader/0.26.0/loader.jar"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/maven/org/quiltmc/quilt-loader/0.26.0/loader.jar"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://maven.neoforged.net/releases/net/neoforged/neoforge/21.1.0/neoforge.jar"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/maven/net/neoforged/neoforge/21.1.0/neoforge.jar"));
        QCOMPARE(Net::DownloadMirror::rewriteUrl(QUrl("https://maven.neoforged.net/api/maven/details/releases/net/neoforged/neoforge"),
                                                 Net::DownloadMirror::Source::BMCLAPI),
                 QUrl("https://bmclapi2.bangbang93.com/neoforge/meta/api/maven/details/releases/net/neoforged/neoforge"));
    }

    void test_bmclapiDoesNotRewriteUnrelatedHosts()
    {
        const QList<QUrl> urls = {
            QUrl("https://auth.mc-user.com:233/aa9441c0487a11e88feb525400b59b6a/authserver/refresh"),
            QUrl("https://login.mc-user.com:233/aa9441c0487a11e88feb525400b59b6a/loginreg"),
            QUrl("https://meta.prismlauncher.org/v1/net.minecraft/index.json"),
            QUrl("https://piston-meta.mojang.com/v1/packages/5643d63e75d54da43c9501cdb1988530a9bc5d7f/manifest.json"),
            QUrl("https://piston-meta.mojang.com/v1/packages/bd326ebbea07119b1f603bafc4ea0f64e3605c8b/1.21.5.json"),
            QUrl("https://api.modrinth.com/v2/project/foo"),
            QUrl("https://cdn.modrinth.com/data/foo/bar.jar"),
            QUrl("https://textures.minecraft.net/texture/abcdef"),
        };

        for (const auto& url : urls) {
            QCOMPARE(Net::DownloadMirror::rewriteUrl(url, Net::DownloadMirror::Source::BMCLAPI), url);
        }
    }
};

QTEST_GUILESS_MAIN(DownloadMirrorTest)

#include "DownloadMirror_test.moc"
