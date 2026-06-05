/* Copyright 2015-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BaseEntity.h"

#include "Exception.h"
#include "FileSystem.h"
#include "Json.h"
#include "modplatform/helpers/HashUtils.h"
#include "net/ApiDownload.h"
#include "net/ChecksumValidator.h"
#include "net/DownloadMirror.h"
#include "net/HttpMetaCache.h"
#include "net/Mode.h"
#include "net/NetJob.h"

#include "Application.h"
#include "settings/SettingsObject.h"
#include "BuildConfig.h"
#include "tasks/Task.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QRegularExpression>
#include <utility>

namespace Meta {

enum class ResponseTransform {
    None,
    MinecraftVersionManifest,
    MinecraftVersionFile,
    JavaRuntimeIndex,
    JavaRuntimeVersion,
    FabricLoaderList,
    FabricLoaderVersion,
    FabricIntermediaryList,
    NeoForgeList
};

constexpr auto kBMCLAPIBaseUrl = "https://bmclapi2.bangbang93.com/";
constexpr auto kJavaRuntimeIndexPath = "v1/products/java-runtime/2ec0cc96c44e5a76b9c8b7c39df7210883d12871/all.json";
constexpr auto kFabricProfileMinecraftVersion = "1.21.1";

QString bmclapiUrl(const QString& path)
{
    return QString::fromLatin1(kBMCLAPIBaseUrl) + path;
}

QString normalizedIsoTime(QString time)
{
    if (time.isEmpty()) {
        return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }

    static const QRegularExpression timezoneWithoutColon(QStringLiteral("([+-]\\d\\d)(\\d\\d)$"));
    time.replace(timezoneWithoutColon, QStringLiteral("\\1:\\2"));
    return time;
}

QString syntheticReleaseTime(int rank)
{
    return QDateTime::currentDateTimeUtc().addSecs(-rank).toString(Qt::ISODate);
}

QJsonArray makeRequires(const QString& uid, const QString& equalsVersion = {})
{
    QJsonObject require;
    require.insert("uid", uid);
    if (!equalsVersion.isEmpty()) {
        require.insert("equals", equalsVersion);
    }
    return { require };
}

QJsonObject versionEntry(const QString& version,
                         const QString& releaseTime,
                         const QString& type = QStringLiteral("release"),
                         bool recommended = false,
                         const QJsonArray& requirements = {})
{
    QJsonObject out;
    out.insert("version", version);
    out.insert("releaseTime", releaseTime);
    out.insert("type", type);
    out.insert("recommended", recommended);
    if (!requirements.isEmpty()) {
        out.insert("requires", requirements);
    }
    return out;
}

QString minecraftVersionFromMetaFilename(const QString& filename)
{
    const QString prefix = QStringLiteral("net.minecraft/");
    const QString suffix = QStringLiteral(".json");
    if (!filename.startsWith(prefix) || !filename.endsWith(suffix) || filename == QLatin1String("net.minecraft/index.json")) {
        return {};
    }
    return filename.mid(prefix.size(), filename.size() - prefix.size() - suffix.size());
}

QString versionFromComponentMetaFilename(const QString& filename, const QString& component)
{
    const QString prefix = component + QLatin1Char('/');
    const QString suffix = QStringLiteral(".json");
    if (!filename.startsWith(prefix) || !filename.endsWith(suffix) || filename == prefix + QLatin1String("index.json")) {
        return {};
    }
    return filename.mid(prefix.size(), filename.size() - prefix.size() - suffix.size());
}

QString javaVersionFromMetaFilename(const QString& filename)
{
    const QString javaVersion = versionFromComponentMetaFilename(filename, QStringLiteral("net.minecraft.java"));
    return javaVersion.startsWith(QLatin1String("java")) ? javaVersion : QString();
}

int javaMajorFromVersionId(const QString& versionId)
{
    if (!versionId.startsWith(QLatin1String("java"))) {
        return 0;
    }
    bool ok = false;
    const int major = versionId.mid(4).toInt(&ok);
    return ok ? major : 0;
}

int javaMajorForRuntimeName(const QString& runtimeName)
{
    if (runtimeName == QLatin1String("jre-legacy")) {
        return 8;
    }
    if (runtimeName == QLatin1String("java-runtime-alpha")) {
        return 16;
    }
    if (runtimeName == QLatin1String("java-runtime-beta") || runtimeName == QLatin1String("java-runtime-gamma") ||
        runtimeName == QLatin1String("java-runtime-gamma-snapshot")) {
        return 17;
    }
    if (runtimeName == QLatin1String("java-runtime-delta")) {
        return 21;
    }
    if (runtimeName == QLatin1String("java-runtime-epsilon")) {
        return 25;
    }
    return 0;
}

QString prismRuntimeOS(QString mojangRuntimeOS)
{
    if (mojangRuntimeOS == QLatin1String("linux")) {
        return QStringLiteral("linux-x64");
    }
    if (mojangRuntimeOS == QLatin1String("mac-os")) {
        return QStringLiteral("mac-os-x64");
    }
    if (mojangRuntimeOS == QLatin1String("linux-i386")) {
        return QStringLiteral("linux-x86");
    }
    return mojangRuntimeOS;
}

QJsonObject javaVersionObject(const QString& versionName, int fallbackMajor)
{
    QJsonObject out;
    out.insert("name", versionName);

    QList<int> numbers;
    static const QRegularExpression numberRegex(QStringLiteral("(\\d+)"));
    auto match = numberRegex.globalMatch(versionName);
    while (match.hasNext()) {
        numbers.append(match.next().captured(1).toInt());
    }

    int major = fallbackMajor;
    int minor = 0;
    int security = 0;
    int build = 0;
    if (numbers.size() >= 2 && numbers.at(0) == 1) {
        major = numbers.at(1);
        minor = numbers.value(2);
        security = numbers.value(3);
        build = numbers.value(4);
    } else if (!numbers.isEmpty()) {
        major = numbers.value(0, fallbackMajor);
        minor = numbers.value(1);
        security = numbers.value(2);
        build = numbers.value(3);
    }

    out.insert("major", major);
    out.insert("minor", minor);
    out.insert("security", security);
    if (build > 0) {
        out.insert("build", build);
    }
    return out;
}

QJsonObject syntheticBMCLAPIIndex()
{
    auto package = [](const QString& uid, const QString& name) {
        QJsonObject obj;
        obj.insert("uid", uid);
        obj.insert("name", name);
        return obj;
    };

    QJsonObject out;
    out.insert("formatVersion", 1);
    out.insert("packages", QJsonArray{ package(QStringLiteral("net.minecraft"), QStringLiteral("Minecraft")),
                                       package(QStringLiteral("net.minecraft.java"), QStringLiteral("Java Runtimes")),
                                       package(QStringLiteral("net.fabricmc.fabric-loader"), QStringLiteral("Fabric Loader")),
                                       package(QStringLiteral("net.fabricmc.intermediary"), QStringLiteral("Fabric Intermediary")),
                                       package(QStringLiteral("net.neoforged"), QStringLiteral("NeoForge")) });
    return out;
}

QJsonObject minecraftManifestToVersionList(const QJsonObject& obj)
{
    QJsonObject out;
    out.insert("formatVersion", 1);
    out.insert("name", "Minecraft");
    out.insert("uid", "net.minecraft");

    const auto latest = obj.value("latest").toObject();
    const auto recommendedRelease = latest.value("release").toString();

    QJsonArray versions;
    const auto rawVersions = obj.value("versions").toArray();
    for (const auto& rawVersion : rawVersions) {
        const auto versionObj = rawVersion.toObject();
        const auto versionId = versionObj.value("id").toString();
        if (versionId.isEmpty()) {
            continue;
        }

        QJsonObject version;
        version.insert("version", versionId);
        version.insert("type", versionObj.value("type").toString());
        version.insert("releaseTime", versionObj.value("releaseTime").toString());
        version.insert("recommended", versionId == recommendedRelease);
        versions.append(version);
    }

    out.insert("versions", versions);
    return out;
}

QJsonObject minecraftVersionToMetaVersion(QJsonObject obj, const QString& fallbackVersion)
{
    const auto versionId = obj.value("id").toString(fallbackVersion);
    obj.insert("formatVersion", 1);
    obj.insert("uid", "net.minecraft");
    obj.insert("version", versionId);
    obj.insert("name", "Minecraft");
    if (!obj.contains("releaseTime") && obj.contains("time")) {
        obj.insert("releaseTime", obj.value("time"));
    }
    const auto javaVersion = obj.value("javaVersion").toObject();
    const auto javaComponent = javaVersion.value("component").toString();
    const auto javaMajor = javaVersion.value("majorVersion").toInt();
    if (!javaComponent.isEmpty()) {
        obj.insert("compatibleJavaName", javaComponent);
    }
    if (javaMajor > 0) {
        obj.insert("compatibleJavaMajors", QJsonArray{ javaMajor });
    }
    if (!obj.contains("type")) {
        obj.insert("type", "release");
    }
    return obj;
}

QJsonObject javaRuntimeIndexToVersionList(const QJsonObject& obj)
{
    QMap<int, QDateTime> latestReleaseByMajor;
    for (auto osIt = obj.constBegin(); osIt != obj.constEnd(); ++osIt) {
        if (osIt.key() == QLatin1String("gamecore")) {
            continue;
        }
        const auto runtimes = osIt.value().toObject();
        for (auto runtimeIt = runtimes.constBegin(); runtimeIt != runtimes.constEnd(); ++runtimeIt) {
            const int major = javaMajorForRuntimeName(runtimeIt.key());
            if (major == 0) {
                continue;
            }
            for (const auto& rawRuntime : runtimeIt.value().toArray()) {
                const auto runtime = rawRuntime.toObject();
                const auto release = QDateTime::fromString(normalizedIsoTime(runtime.value("version").toObject().value("released").toString()),
                                                           Qt::ISODate);
                if (!latestReleaseByMajor.contains(major) || latestReleaseByMajor.value(major) < release) {
                    latestReleaseByMajor.insert(major, release);
                }
            }
        }
    }

    QJsonObject out;
    out.insert("formatVersion", 1);
    out.insert("name", "Java Runtimes");
    out.insert("uid", "net.minecraft.java");

    QJsonArray versions;
    for (const int major : { 25, 21, 17, 16, 8 }) {
        if (!latestReleaseByMajor.contains(major)) {
            continue;
        }
        versions.append(versionEntry(QStringLiteral("java%1").arg(major), latestReleaseByMajor.value(major).toUTC().toString(Qt::ISODate)));
    }
    out.insert("versions", versions);
    return out;
}

QJsonObject javaRuntimeVersionToMetaVersion(const QJsonObject& obj, const QString& javaVersionId)
{
    const int requestedMajor = javaMajorFromVersionId(javaVersionId);

    QJsonObject out;
    out.insert("formatVersion", 1);
    out.insert("uid", "net.minecraft.java");
    out.insert("version", javaVersionId);
    out.insert("name", QStringLiteral("Java %1").arg(requestedMajor));
    out.insert("type", "release");

    QDateTime latestRelease;
    QJsonArray runtimesOut;
    for (auto osIt = obj.constBegin(); osIt != obj.constEnd(); ++osIt) {
        if (osIt.key() == QLatin1String("gamecore")) {
            continue;
        }
        const auto runtimeOS = prismRuntimeOS(osIt.key());
        const auto runtimes = osIt.value().toObject();
        for (auto runtimeIt = runtimes.constBegin(); runtimeIt != runtimes.constEnd(); ++runtimeIt) {
            if (javaMajorForRuntimeName(runtimeIt.key()) != requestedMajor) {
                continue;
            }
            for (const auto& rawRuntime : runtimeIt.value().toArray()) {
                const auto runtime = rawRuntime.toObject();
                const auto manifest = runtime.value("manifest").toObject();
                const auto version = runtime.value("version").toObject();
                const auto manifestUrl = manifest.value("url").toString();
                if (manifestUrl.isEmpty()) {
                    continue;
                }

                const auto releaseTime = normalizedIsoTime(version.value("released").toString());
                const auto releaseDate = QDateTime::fromString(releaseTime, Qt::ISODate);
                if (!latestRelease.isValid() || latestRelease < releaseDate) {
                    latestRelease = releaseDate;
                }

                QJsonObject runtimeOut;
                QJsonObject checksum;
                checksum.insert("type", "sha1");
                checksum.insert("hash", manifest.value("sha1").toString());
                runtimeOut.insert("checksum", checksum);
                runtimeOut.insert("downloadType", "manifest");
                runtimeOut.insert("name", runtimeIt.key());
                runtimeOut.insert("packageType", "jre");
                runtimeOut.insert("releaseTime", releaseTime);
                runtimeOut.insert("runtimeOS", runtimeOS);
                runtimeOut.insert("url", manifestUrl);
                runtimeOut.insert("vendor", "mojang");
                runtimeOut.insert("version", javaVersionObject(version.value("name").toString(), requestedMajor));
                runtimesOut.append(runtimeOut);
            }
        }
    }

    out.insert("releaseTime", latestRelease.isValid() ? latestRelease.toUTC().toString(Qt::ISODate) : syntheticReleaseTime(0));
    out.insert("runtimes", runtimesOut);
    return out;
}

QJsonObject fabricLoaderListToVersionList(const QJsonArray& rawVersions)
{
    QJsonObject out;
    out.insert("formatVersion", 1);
    out.insert("name", "Fabric Loader");
    out.insert("uid", "net.fabricmc.fabric-loader");

    QJsonArray versions;
    bool recommendedSet = false;
    for (int i = 0; i < rawVersions.size(); ++i) {
        const auto rawVersion = rawVersions.at(i).toObject();
        const auto versionId = rawVersion.value("version").toString();
        if (versionId.isEmpty()) {
            continue;
        }

        const bool stable = rawVersion.value("stable").toBool();
        const bool recommended = stable && !recommendedSet;
        recommendedSet = recommendedSet || recommended;
        versions.append(versionEntry(versionId, syntheticReleaseTime(i), stable ? QStringLiteral("release") : QStringLiteral("snapshot"),
                                     recommended, makeRequires(QStringLiteral("net.fabricmc.intermediary"))));
    }

    out.insert("versions", versions);
    return out;
}

QJsonObject fabricLoaderProfileToMetaVersion(const QJsonObject& obj, const QString& loaderVersion)
{
    QJsonObject out;
    out.insert("formatVersion", 1);
    out.insert("uid", "net.fabricmc.fabric-loader");
    out.insert("version", loaderVersion);
    out.insert("name", "Fabric Loader");
    out.insert("order", 10);
    out.insert("releaseTime", normalizedIsoTime(obj.value("releaseTime").toString()));
    out.insert("type", obj.value("type").toString("release"));
    out.insert("mainClass", obj.value("mainClass").toString());
    out.insert("requires", makeRequires(QStringLiteral("net.fabricmc.intermediary")));

    QJsonArray libraries;
    for (const auto& rawLibrary : obj.value("libraries").toArray()) {
        const auto library = rawLibrary.toObject();
        const auto name = library.value("name").toString();
        if (name.isEmpty() || name.startsWith(QLatin1String("net.fabricmc:intermediary:"))) {
            continue;
        }
        QJsonObject libraryOut;
        libraryOut.insert("name", name);
        libraryOut.insert("url", library.value("url").toString("https://maven.fabricmc.net/"));
        libraries.append(libraryOut);
    }
    out.insert("libraries", libraries);
    return out;
}

QJsonObject fabricIntermediaryListToVersionList(const QJsonArray& rawVersions)
{
    QJsonObject out;
    out.insert("formatVersion", 1);
    out.insert("name", "Fabric Intermediary");
    out.insert("uid", "net.fabricmc.intermediary");

    QJsonArray versions;
    bool recommendedSet = false;
    for (int i = 0; i < rawVersions.size(); ++i) {
        const auto rawVersion = rawVersions.at(i).toObject();
        const auto versionId = rawVersion.value("version").toString();
        if (versionId.isEmpty()) {
            continue;
        }

        const bool stable = rawVersion.value("stable").toBool();
        const bool recommended = stable && !recommendedSet;
        recommendedSet = recommendedSet || recommended;
        auto version = versionEntry(versionId, syntheticReleaseTime(i), stable ? QStringLiteral("release") : QStringLiteral("snapshot"),
                                    recommended, makeRequires(QStringLiteral("net.minecraft"), versionId));
        version.insert("volatile", true);
        versions.append(version);
    }

    out.insert("versions", versions);
    return out;
}

QJsonObject fabricIntermediaryMetaVersion(const QString& minecraftVersion)
{
    QJsonObject library;
    library.insert("name", QStringLiteral("net.fabricmc:intermediary:%1").arg(minecraftVersion));
    library.insert("url", "https://maven.fabricmc.net");

    QJsonObject out;
    out.insert("formatVersion", 1);
    out.insert("uid", "net.fabricmc.intermediary");
    out.insert("version", minecraftVersion);
    out.insert("name", "Intermediary Mappings");
    out.insert("order", 11);
    out.insert("releaseTime", syntheticReleaseTime(0));
    out.insert("type", "release");
    out.insert("volatile", true);
    out.insert("requires", makeRequires(QStringLiteral("net.minecraft"), minecraftVersion));
    out.insert("libraries", QJsonArray{ library });
    return out;
}

QString inferNeoForgeMinecraftVersion(const QString& neoForgeVersion)
{
    const auto baseVersion = neoForgeVersion.section('-', 0, 0);
    const auto parts = baseVersion.split('.');
    if (parts.size() < 2) {
        return {};
    }

    bool ok = false;
    const int first = parts.at(0).toInt(&ok);
    if (!ok) {
        return {};
    }

    if (first >= 25) {
        return parts.size() >= 3 ? QStringLiteral("%1.%2.%3").arg(parts.at(0), parts.at(1), parts.at(2))
                                 : QStringLiteral("%1.%2").arg(parts.at(0), parts.at(1));
    }
    if (first >= 20) {
        return QStringLiteral("1.%1.%2").arg(first).arg(parts.at(1));
    }
    return {};
}

QJsonObject neoForgeListToVersionList(const QJsonObject& obj)
{
    QJsonObject out;
    out.insert("formatVersion", 1);
    out.insert("name", "NeoForge");
    out.insert("uid", "net.neoforged");

    QJsonArray versions;
    const auto files = obj.value("files").toArray();
    int rank = 0;
    for (int i = files.size() - 1; i >= 0; --i) {
        const auto file = files.at(i).toObject();
        if (file.value("type").toString() != QLatin1String("DIRECTORY")) {
            continue;
        }
        const auto versionId = file.value("name").toString();
        const auto minecraftVersion = inferNeoForgeMinecraftVersion(versionId);
        if (versionId.isEmpty() || minecraftVersion.isEmpty()) {
            continue;
        }
        versions.append(versionEntry(versionId, syntheticReleaseTime(rank++),
                                     versionId.contains(QLatin1String("beta")) ? QStringLiteral("snapshot") : QStringLiteral("release"),
                                     false, makeRequires(QStringLiteral("net.minecraft"), minecraftVersion)));
    }

    out.insert("versions", versions);
    return out;
}

bool bmclapiSyntheticMetaObject(const QString& filename, QJsonObject& obj)
{
    if (filename == QLatin1String("index.json")) {
        obj = syntheticBMCLAPIIndex();
        return true;
    }

    const auto fabricIntermediaryVersion = versionFromComponentMetaFilename(filename, QStringLiteral("net.fabricmc.intermediary"));
    if (!fabricIntermediaryVersion.isEmpty()) {
        obj = fabricIntermediaryMetaVersion(fabricIntermediaryVersion);
        return true;
    }
    return false;
}

bool bmclapiMetaRequest(const QString& filename, QUrl& url, ResponseTransform& transform, QString& parameter)
{
    transform = ResponseTransform::None;
    parameter.clear();

    if (filename == QLatin1String("net.minecraft/index.json")) {
        url = QUrl(bmclapiUrl(QStringLiteral("mc/game/version_manifest_v2.json")));
        transform = ResponseTransform::MinecraftVersionManifest;
        return true;
    }

    const auto minecraftVersion = minecraftVersionFromMetaFilename(filename);
    if (!minecraftVersion.isEmpty()) {
        url = QUrl(bmclapiUrl(QStringLiteral("version/%1/json").arg(minecraftVersion)));
        transform = ResponseTransform::MinecraftVersionFile;
        parameter = minecraftVersion;
        return true;
    }

    if (filename == QLatin1String("net.minecraft.java/index.json")) {
        url = QUrl(bmclapiUrl(QString::fromLatin1(kJavaRuntimeIndexPath)));
        transform = ResponseTransform::JavaRuntimeIndex;
        return true;
    }

    const auto javaVersion = javaVersionFromMetaFilename(filename);
    if (!javaVersion.isEmpty()) {
        url = QUrl(bmclapiUrl(QString::fromLatin1(kJavaRuntimeIndexPath)));
        transform = ResponseTransform::JavaRuntimeVersion;
        parameter = javaVersion;
        return true;
    }

    if (filename == QLatin1String("net.fabricmc.fabric-loader/index.json")) {
        url = QUrl(bmclapiUrl(QStringLiteral("fabric-meta/v2/versions/loader")));
        transform = ResponseTransform::FabricLoaderList;
        return true;
    }

    const auto fabricLoaderVersion = versionFromComponentMetaFilename(filename, QStringLiteral("net.fabricmc.fabric-loader"));
    if (!fabricLoaderVersion.isEmpty()) {
        url = QUrl(bmclapiUrl(QStringLiteral("fabric-meta/v2/versions/loader/%1/%2/profile/json")
                                  .arg(QString::fromLatin1(kFabricProfileMinecraftVersion), fabricLoaderVersion)));
        transform = ResponseTransform::FabricLoaderVersion;
        parameter = fabricLoaderVersion;
        return true;
    }

    if (filename == QLatin1String("net.fabricmc.intermediary/index.json")) {
        url = QUrl(bmclapiUrl(QStringLiteral("fabric-meta/v2/versions/game")));
        transform = ResponseTransform::FabricIntermediaryList;
        return true;
    }

    if (filename == QLatin1String("net.neoforged/index.json")) {
        url = QUrl(bmclapiUrl(QStringLiteral("neoforge/meta/api/maven/details/releases/net/neoforged/neoforge")));
        transform = ResponseTransform::NeoForgeList;
        return true;
    }

    return false;
}

class ParsingValidator : public Net::Validator {
   public: /* con/des */
    ParsingValidator(BaseEntity* entity, ResponseTransform transform, QString parameter = {})
        : m_entity(entity), m_transform(transform), m_parameter(std::move(parameter)) {};
    virtual ~ParsingValidator() = default;

   public: /* methods */
    bool init(QNetworkRequest&) override
    {
        m_data.clear();
        m_transformedData.clear();
        return true;
    }
    bool write(QByteArray& data) override
    {
        this->m_data.append(data);
        return true;
    }
    bool abort() override
    {
        m_data.clear();
        return true;
    }
    bool validate(QNetworkReply&) override
    {
        auto fname = m_entity->localFilename();
        try {
            auto doc = Json::requireDocument(m_data, fname);
            QJsonObject obj;
            if (m_transform == ResponseTransform::MinecraftVersionManifest) {
                obj = minecraftManifestToVersionList(Json::requireObject(doc, fname));
            } else if (m_transform == ResponseTransform::MinecraftVersionFile) {
                obj = minecraftVersionToMetaVersion(Json::requireObject(doc, fname), m_parameter);
            } else if (m_transform == ResponseTransform::JavaRuntimeIndex) {
                obj = javaRuntimeIndexToVersionList(Json::requireObject(doc, fname));
            } else if (m_transform == ResponseTransform::JavaRuntimeVersion) {
                obj = javaRuntimeVersionToMetaVersion(Json::requireObject(doc, fname), m_parameter);
            } else if (m_transform == ResponseTransform::FabricLoaderList) {
                obj = fabricLoaderListToVersionList(Json::requireArray(doc, fname));
            } else if (m_transform == ResponseTransform::FabricLoaderVersion) {
                obj = fabricLoaderProfileToMetaVersion(Json::requireObject(doc, fname), m_parameter);
            } else if (m_transform == ResponseTransform::FabricIntermediaryList) {
                obj = fabricIntermediaryListToVersionList(Json::requireArray(doc, fname));
            } else if (m_transform == ResponseTransform::NeoForgeList) {
                obj = neoForgeListToVersionList(Json::requireObject(doc, fname));
            } else {
                obj = Json::requireObject(doc, fname);
            }
            if (m_transform != ResponseTransform::None) {
                m_transformedData = QJsonDocument(obj).toJson(QJsonDocument::Compact);
            }
            m_entity->parse(obj);
            return true;
        } catch (const Exception& e) {
            qWarning() << "Unable to parse response:" << e.cause();
            return false;
        }
    }

    const QByteArray& transformedData() const { return m_transformedData; }

   private: /* data */
    QByteArray m_data;
    QByteArray m_transformedData;
    BaseEntity* m_entity;
    ResponseTransform m_transform = ResponseTransform::None;
    QString m_parameter;
};

QUrl BaseEntity::url() const
{
    auto s = APPLICATION->settings();
    QString metaOverride = s->get("MetaURLOverride").toString();
    if (metaOverride.isEmpty()) {
        return QUrl(BuildConfig.META_URL).resolved(localFilename());
    }
    return QUrl(metaOverride).resolved(localFilename());
}

Task::Ptr BaseEntity::loadTask(Net::Mode mode, bool forceReload)
{
    if (m_task && m_task->isRunning()) {
        return m_task;
    }
    m_task.reset(new BaseEntityLoadTask(this, mode, forceReload));
    return m_task;
}

bool BaseEntity::isLoaded() const
{
    // consider it loaded only if the main hash is either empty and was remote loadded or the hashes match and was loaded
    return m_sha256.isEmpty() ? m_load_status == LoadStatus::Remote : m_load_status != LoadStatus::NotLoaded && m_sha256 == m_file_sha256;
}

void BaseEntity::setSha256(QString sha256)
{
    m_sha256 = sha256;
}

BaseEntity::LoadStatus BaseEntity::status() const
{
    return m_load_status;
}

BaseEntityLoadTask::BaseEntityLoadTask(BaseEntity* parent, Net::Mode mode, bool forceReload)
    : m_entity(parent), m_mode(mode), m_force_reload(forceReload)
{}

void BaseEntityLoadTask::executeTask()
{
    const auto localFilename = m_entity->localFilename();
    const QString fname = QDir("meta").absoluteFilePath(localFilename);
    const bool useBMCLAPIMeta = Net::DownloadMirror::currentSource() == Net::DownloadMirror::Source::BMCLAPI && m_mode == Net::Mode::Online;

    QJsonObject syntheticObject;
    if (useBMCLAPIMeta && bmclapiSyntheticMetaObject(localFilename, syntheticObject)) {
        const auto transformedData = QJsonDocument(syntheticObject).toJson(QJsonDocument::Compact);
        try {
            m_entity->parse(syntheticObject);
            FS::write(fname, transformedData);
            m_entity->m_file_sha256 = Hashing::hash(transformedData, Hashing::Algorithm::Sha256);
            m_entity->m_load_status = BaseEntity::LoadStatus::Remote;
            qInfo() << "Using synthetic BMCLAPI launcher metadata:" << localFilename;
            emitSucceeded();
        } catch (const Exception& e) {
            emitFailed(e.cause());
        }
        return;
    }

    QUrl bmclapiRequestUrl;
    ResponseTransform bmclapiResponseTransform = ResponseTransform::None;
    QString bmclapiTransformParameter;
    const bool hasBMCLAPIMetaRequest =
        useBMCLAPIMeta && bmclapiMetaRequest(localFilename, bmclapiRequestUrl, bmclapiResponseTransform, bmclapiTransformParameter);

    auto hashMatches = false;
    // the file exists on disk try to load it
    if (!hasBMCLAPIMetaRequest && QFile::exists(fname)) {
        try {
            QByteArray fileData;
            // read local file if nothing is loaded yet
            if (m_entity->m_load_status == BaseEntity::LoadStatus::NotLoaded || m_entity->m_file_sha256.isEmpty()) {
                setStatus(tr("Loading local file"));
                fileData = FS::read(fname);
                m_entity->m_file_sha256 = Hashing::hash(fileData, Hashing::Algorithm::Sha256);
            }

            // on online the hash needs to match
            const auto& expected = m_entity->m_sha256;
            const auto& actual = m_entity->m_file_sha256;
            hashMatches = expected == actual;
            if (m_mode == Net::Mode::Online && !m_entity->m_sha256.isEmpty() && !hashMatches) {
                throw Exception(QString("Checksum mismatch, expected sha256: %1, got: %2").arg(expected, actual));
            }

            // load local file
            if (m_entity->m_load_status == BaseEntity::LoadStatus::NotLoaded) {
                auto doc = Json::requireDocument(fileData, fname);
                auto obj = Json::requireObject(doc, fname);
                m_entity->parse(obj);
                m_entity->m_load_status = BaseEntity::LoadStatus::Local;
            }

        } catch (const Exception& e) {
            qDebug() << QString("Unable to parse file %1: %2").arg(fname, e.cause());
            // just make sure it's gone and we never consider it again.
            FS::deletePath(fname);
            m_entity->m_load_status = BaseEntity::LoadStatus::NotLoaded;
        }
    }
    // if we need remote update, run the update task
    auto wasLoadedOffline = m_entity->m_load_status != BaseEntity::LoadStatus::NotLoaded && m_mode == Net::Mode::Offline;
    // if has is not present allways fetch from remote(e.g. the main index file), else only fetch if hash doesn't match
    auto wasLoadedRemote = m_entity->m_sha256.isEmpty() ? m_entity->m_load_status == BaseEntity::LoadStatus::Remote : hashMatches;
    auto canUseCachedMainIndex = !useBMCLAPIMeta && m_entity->m_sha256.isEmpty() && localFilename == QLatin1String("index.json") &&
                                 m_entity->m_load_status != BaseEntity::LoadStatus::NotLoaded && !m_force_reload;
    if (wasLoadedOffline || canUseCachedMainIndex || (wasLoadedRemote && !m_force_reload)) {
        if (canUseCachedMainIndex) {
            qInfo() << "Using cached launcher metadata:" << fname;
        }
        emitSucceeded();
        return;
    }
    m_task.reset(new NetJob(QObject::tr("Download of meta file %1").arg(localFilename), APPLICATION->network()));
    auto url = m_entity->url();
    ResponseTransform responseTransform = ResponseTransform::None;
    QString transformParameter;
    if (hasBMCLAPIMetaRequest) {
        const auto originalUrl = url;
        url = bmclapiRequestUrl;
        responseTransform = bmclapiResponseTransform;
        transformParameter = bmclapiTransformParameter;
        qInfo() << "Using BMCLAPI metadata mirror:" << originalUrl.toString() << "->" << url.toString();
    } else if (useBMCLAPIMeta && localFilename.startsWith(QLatin1String("net.neoforged/")) &&
               localFilename != QLatin1String("net.neoforged/index.json")) {
        qInfo() << "NeoForge metadata keeps Prism-hosted ForgeWrapper as a temporary BMCLAPI exception:" << url.toString();
    }
    auto entry = APPLICATION->metacache()->resolveEntry("meta", localFilename);
    if (m_force_reload) {
        // clear validators so manual refreshes fetch a fresh body
        entry->setETag({});
        entry->setRemoteChangedTimestamp({});
    }
    entry->setStale(true);
    auto dl = Net::ApiDownload::makeCached(url, entry);
    /*
     * The validator parses the file and loads it into the object.
     * If that fails, the file is not written to storage.
     */
    if (!m_entity->m_sha256.isEmpty() && responseTransform == ResponseTransform::None)
        dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Algorithm::Sha256, m_entity->m_sha256));
    auto* parsingValidator = new ParsingValidator(m_entity, responseTransform, transformParameter);
    dl->addValidator(parsingValidator);
    m_task->addNetAction(dl);
    m_task->setAskRetry(false);
    connect(m_task.get(), &Task::failed, this, &BaseEntityLoadTask::emitFailed);
    connect(m_task.get(), &Task::succeeded, this, &BaseEntityLoadTask::emitSucceeded);
    connect(m_task.get(), &Task::succeeded, this, [this, fname, parsingValidator]() {
        const auto& transformedData = parsingValidator->transformedData();
        if (!transformedData.isEmpty()) {
            try {
                FS::write(fname, transformedData);
                m_entity->m_file_sha256 = Hashing::hash(transformedData, Hashing::Algorithm::Sha256);
            } catch (const Exception& e) {
                qWarning() << "Unable to write transformed metadata cache:" << e.cause();
            }
        } else {
            m_entity->m_file_sha256 = m_entity->m_sha256;
        }
        m_entity->m_load_status = BaseEntity::LoadStatus::Remote;
    });

    connect(m_task.get(), &Task::progress, this, &Task::setProgress);
    connect(m_task.get(), &Task::stepProgress, this, &BaseEntityLoadTask::propagateStepProgress);
    connect(m_task.get(), &Task::status, this, &Task::setStatus);
    connect(m_task.get(), &Task::details, this, &Task::setDetails);

    m_task->start();
}

bool BaseEntityLoadTask::canAbort() const
{
    return m_task ? m_task->canAbort() : false;
}

bool BaseEntityLoadTask::abort()
{
    if (m_task) {
        Task::abort();
        return m_task->abort();
    }
    return Task::abort();
}

}  // namespace Meta
