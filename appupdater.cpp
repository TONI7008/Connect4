#include "appupdater.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QSysInfo>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>
#include <QProcess>
#include <QDebug>

// ── Constructor / Destructor ──────────────────────────────────────────────────

AppUpdater::AppUpdater(const QString &repoOwner,
                       const QString &repoName,
                       const QString &currentVersion,
                       QObject       *parent)
    : QObject(parent)
    , m_repoOwner(repoOwner)
    , m_repoName(repoName)
    , m_currentVersion(currentVersion)
{
    m_nam = new QNetworkAccessManager(this);
}

AppUpdater::~AppUpdater()
{
    cancelDownload();
    if (m_outFile) { m_outFile->close(); delete m_outFile; }
}

// ── Check ─────────────────────────────────────────────────────────────────────

void AppUpdater::checkForUpdates()
{
    // GitHub REST API — returns JSON for the latest release
    QString urlStr = QString("https://api.github.com/repos/%1/%2/releases/latest")
                         .arg(m_repoOwner, m_repoName);

    QNetworkRequest req{QUrl(urlStr)};
    req.setRawHeader("Accept",     "application/vnd.github+json");
    req.setRawHeader("User-Agent", "Qt-AppUpdater/1.0");
    // Set a reasonable timeout via redirect policy
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onCheckReply(reply);
    });
}

void AppUpdater::onCheckReply(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit checkError(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(data, &pe);

    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        emit checkError("Failed to parse GitHub API response");
        return;
    }

    QJsonObject root = doc.object();

    // tag_name is typically "v1.2.3" — strip leading 'v'
    QString tag = root["tag_name"].toString();
    if (tag.startsWith('v', Qt::CaseInsensitive))
        tag = tag.mid(1);

    m_latestVersion = tag;

    if (!isNewer(tag, m_currentVersion)) {
        emit upToDate(m_currentVersion);
        return;
    }

    // Find the right asset for this platform
    QJsonArray assets = root["assets"].toArray();
    QString assetUrl  = pickAssetForPlatform(assets);

    if (assetUrl.isEmpty()) {
        emit checkError(QString("Update v%1 found but no asset for platform '%2'.")
                            .arg(tag, QSysInfo::productType()));
        return;
    }

    m_downloadUrl = QUrl(assetUrl);
    emit updateAvailable(m_latestVersion, m_downloadUrl);
}

// ── Download ──────────────────────────────────────────────────────────────────

void AppUpdater::startDownload()
{
    if (m_downloading || m_downloadUrl.isEmpty()) return;
    m_downloading = true;

    // Save to the system's Downloads folder
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDir().mkpath(downloadDir);

    QString fileName = QFileInfo(m_downloadUrl.path()).fileName();
    m_savedFilePath  = downloadDir + QDir::separator() + fileName;

    m_outFile = new QFile(m_savedFilePath);
    if (!m_outFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit downloadError("Cannot open file for writing: " + m_savedFilePath);
        m_downloading = false;
        delete m_outFile;
        m_outFile = nullptr;
        return;
    }

    QNetworkRequest req{m_downloadUrl};
    req.setRawHeader("User-Agent", "Qt-AppUpdater/1.0");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    m_downloadReply = m_nam->get(req);

    connect(m_downloadReply, &QNetworkReply::readyRead,
            this, &AppUpdater::onDownloadReadyRead);
    connect(m_downloadReply, &QNetworkReply::downloadProgress,
            this, &AppUpdater::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished,
            this, &AppUpdater::onDownloadFinished);
}

void AppUpdater::cancelDownload()
{
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
    }
    if (m_outFile) {
        m_outFile->close();
        m_outFile->remove(); // Delete partial file
        delete m_outFile;
        m_outFile = nullptr;
    }
    if (m_downloading) {
        m_downloading = false;
        emit downloadCancelled();
    }
}

void AppUpdater::onDownloadReadyRead()
{
    if (m_outFile && m_downloadReply)
        m_outFile->write(m_downloadReply->readAll());
}

void AppUpdater::onDownloadProgress(qint64 received, qint64 total)
{
    if (total > 0)
        emit downloadProgress((int)(received * 100 / total));
}

void AppUpdater::onDownloadFinished()
{
    if (!m_downloadReply) return;

    QNetworkReply::NetworkError err = m_downloadReply->error();
    m_downloadReply->deleteLater();
    m_downloadReply = nullptr;

    if (m_outFile) {
        m_outFile->flush();
        m_outFile->close();
        delete m_outFile;
        m_outFile = nullptr;
    }

    m_downloading = false;

    if (err != QNetworkReply::NoError && err != QNetworkReply::OperationCanceledError) {
        emit downloadError("Download failed: network error " + QString::number(err));
        return;
    }

    if (err == QNetworkReply::NoError)
        emit downloadFinished(m_savedFilePath);
}

// ── Launch installer ──────────────────────────────────────────────────────────

bool AppUpdater::launchInstaller(const QString &filePath)
{
    if (filePath.isEmpty() || !QFile::exists(filePath)) return false;

#if defined(Q_OS_WIN)
    // On Windows, launch the installer executable directly
    return QProcess::startDetached(filePath, {});

#elif defined(Q_OS_MACOS)
    // On macOS, open the .dmg or .pkg
    return QProcess::startDetached("open", {filePath});

#else
    // On Linux, mark AppImage executable then open it; otherwise open the folder
    QFileInfo fi(filePath);
    if (fi.suffix().toLower() == "appimage") {
        QFile::setPermissions(filePath,
            QFile::permissions(filePath) | QFileDevice::ExeOwner | QFileDevice::ExeUser);
        return QProcess::startDetached(filePath, {});
    }
    // For other types, open the containing folder so the user can proceed manually
    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.dir().absolutePath()));
    return true;
#endif
}

// ── Helpers ───────────────────────────────────────────────────────────────────

// Returns true if `candidate` is strictly greater than `current` using
// QVersionNumber so "1.10.0" > "1.9.0" works correctly.
bool AppUpdater::isNewer(const QString &candidate, const QString &current)
{
    int sufA, sufB;
    QVersionNumber a = QVersionNumber::fromString(candidate, &sufA);
    QVersionNumber b = QVersionNumber::fromString(current,   &sufB);

    if (!a.isNull() && !b.isNull())
        return QVersionNumber::compare(a, b) > 0;

    // Fallback: lexicographic (works for semver if major/minor/patch are all single digits)
    return candidate > current;
}

// Pick the download URL from the asset list that best matches the current platform.
// Asset naming conventions we look for:
//   Windows : contains "win" or "windows" or ends with ".exe" / ".msi"
//   macOS   : contains "mac" or "macos" or "darwin" or ends with ".dmg" / ".pkg"
//   Linux   : contains "linux" or ends with ".AppImage" / ".tar.gz" / ".deb"
QString AppUpdater::pickAssetForPlatform(const QJsonArray &assets) const
{
    QString os = QSysInfo::productType().toLower(); // "windows", "macos", "ubuntu", "fedora"…
    QString kernelType = QSysInfo::kernelType().toLower(); // "winnt", "darwin", "linux"

    auto score = [&](const QString &name) -> int {
        QString n = name.toLower();
        int s = 0;

#if defined(Q_OS_WIN)
        if (n.contains("win"))     s += 2;
        if (n.endsWith(".exe") || n.endsWith(".msi")) s += 3;
#elif defined(Q_OS_MACOS)
        if (n.contains("mac") || n.contains("darwin") || n.contains("macos")) s += 2;
        if (n.endsWith(".dmg") || n.endsWith(".pkg"))  s += 3;
#else // Linux
        if (n.contains("linux"))   s += 2;
        if (n.endsWith(".appimage")) s += 4;
        if (n.endsWith(".tar.gz") || n.endsWith(".deb") || n.endsWith(".rpm")) s += 2;
#endif
        // Architecture bonus
        QString arch = QSysInfo::currentCpuArchitecture().toLower();
        if (arch.contains("x86_64") || arch.contains("amd64")) {
            if (n.contains("x86_64") || n.contains("amd64") || n.contains("x64")) s += 1;
        } else if (arch.contains("arm") || arch.contains("aarch64")) {
            if (n.contains("arm") || n.contains("aarch64")) s += 1;
        }

        return s;
    };

    QString bestUrl;
    int bestScore = 0;

    for (const QJsonValue &v : assets) {
        QJsonObject a = v.toObject();
        QString name  = a["name"].toString();
        QString url   = a["browser_download_url"].toString();
        int s = score(name);
        if (s > bestScore) {
            bestScore = s;
            bestUrl   = url;
        }
    }

    return bestUrl; // empty string if nothing matched
}
