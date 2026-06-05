/* Copyright 2013-2021 MultiMC Contributors
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

#include "ExtractNatives.h"
#include <launch/LaunchTask.h>
#include <minecraft/MinecraftInstance.h>

#include <QDir>
#include <QFileInfo>
#include "FileSystem.h"
#include "archive/ArchiveReader.h"
#include "archive/ArchiveWriter.h"

#ifdef major
#undef major
#endif
#ifdef minor
#undef minor
#endif

static QString replaceSuffix(QString target, const QString& suffix, const QString& replacement)
{
    if (!target.endsWith(suffix)) {
        return target;
    }
    target.resize(target.length() - suffix.length());
    return target + replacement;
}

static bool isNativeBinary(const QString& name)
{
    const auto lower = name.toLower();
    return lower.endsWith(QLatin1String(".dll")) || lower.endsWith(QLatin1String(".dylib")) || lower.endsWith(QLatin1String(".so")) ||
           lower.contains(QLatin1String(".so."));
}

static QString normalizedNativeEntryName(const QString& original)
{
    const auto lower = original.toLower();
    if (lower.startsWith(QLatin1String("meta-inf/")) || original.endsWith(QLatin1Char('/'))) {
        return {};
    }

    const auto parts = original.split(QLatin1Char('/'));
    if (parts.size() >= 3 && isNativeBinary(original)) {
        const auto platform = parts.at(0).toLower();
        if (platform == QLatin1String("linux") || platform == QLatin1String("macos") || platform == QLatin1String("windows")) {
            return QFileInfo(original).fileName();
        }
    }

    return original;
}

static bool unzipNatives(QString source, QString targetFolder, bool applyJnilibHack)
{
    MMCZip::ArchiveReader zip(source);
    QDir directory(targetFolder);

    auto extPtr = MMCZip::ArchiveWriter::createDiskWriter();
    auto ext = extPtr.get();

    return zip.parse([applyJnilibHack, directory, ext](MMCZip::ArchiveReader::File* f) {
        QString name = normalizedNativeEntryName(f->filename());
        if (name.isEmpty()) {
            return true;
        }
        if (applyJnilibHack) {
            name = replaceSuffix(name, ".jnilib", ".dylib");
        }
        QString absFilePath = directory.absoluteFilePath(name);
        return f->writeFile(ext, absFilePath, directory);
    });
}

void ExtractNatives::executeTask()
{
    auto instance = m_parent->instance();
    auto toExtract = instance->getNativeJars();
    if (toExtract.isEmpty()) {
        emitSucceeded();
        return;
    }

    auto outputPath = instance->getNativePath();
    FS::ensureFolderPathExists(outputPath);
    auto javaVersion = instance->getJavaVersion();
    bool jniHackEnabled = javaVersion.major() >= 8;
    for (const auto& source : toExtract) {
        if (!unzipNatives(source, outputPath, jniHackEnabled)) {
            const char* reason = QT_TR_NOOP("Couldn't extract native jar '%1' to destination '%2'");
            emit logLine(QString(reason).arg(source, outputPath), MessageLevel::Fatal);
            emitFailed(tr(reason).arg(source, outputPath));
            return;
        }
    }
    emitSucceeded();
}

void ExtractNatives::finalize()
{
    auto instance = m_parent->instance();
    QString target_dir = FS::PathCombine(instance->instanceRoot(), "natives/");
    QDir dir(target_dir);
    dir.removeRecursively();
}
