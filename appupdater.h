#ifndef APPUPDATER_H
#define APPUPDATER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVersionNumber>
#include <QUrl>
#include <QFile>
#include <QProgressBar>
#include <QLabel>

/**
 * AppUpdater
 * ----------
 * Checks the GitHub Releases API for a newer version and downloads
 * the platform-appropriate asset.  The caller wires signals to its UI.
 *
 * Typical usage:
 *
 *   auto *upd = new AppUpdater("octocat", "myapp", "1.2.3", this);
 *   connect(upd, &AppUpdater::updateAvailable, this, [=](const QString& ver, const QUrl& url){
 *       // show a button / toast
 *   });
 *   connect(upd, &AppUpdater::downloadProgress, progressBar, &QProgressBar::setValue);
 *   connect(upd, &AppUpdater::downloadFinished, this, [=](const QString& path){
 *       // prompt user to install
 *   });
 *   upd->checkForUpdates();
 *
 * The updater only downloads; it does NOT replace the running binary itself
 * (that requires a separate launcher/wrapper, like the original Python demo).
 * On Windows it can launch the installer, on macOS it opens the dmg, on Linux
 * it marks the AppImage executable and opens the folder.
 */
class AppUpdater : public QObject
{
    Q_OBJECT

public:
    explicit AppUpdater(const QString &repoOwner,
                        const QString &repoName,
                        const QString &currentVersion,
                        QObject       *parent = nullptr);

    ~AppUpdater();

    // Trigger an async check. Emits updateAvailable() or upToDate() when done.
    void checkForUpdates();

    // Begin downloading the asset URL found during checkForUpdates().
    // Emits downloadProgress (0-100), downloadFinished, or downloadError.
    void startDownload();

    // Abort an in-progress download.
    void cancelDownload();

    // Attempt to launch / open the downloaded file using the OS default handler.
    // Call from a downloadFinished slot.
    bool launchInstaller(const QString &filePath);

    QString latestVersion()  const { return m_latestVersion; }
    QUrl    downloadUrl()    const { return m_downloadUrl;   }
    bool    isDownloading()  const { return m_downloading;   }

signals:
    void updateAvailable(const QString &latestVersion, const QUrl &downloadUrl);
    void upToDate(const QString &currentVersion);
    void checkError(const QString &errorMessage);

    void downloadProgress(int percent);
    void downloadFinished(const QString &savedFilePath);
    void downloadError(const QString &errorMessage);
    void downloadCancelled();

private slots:
    void onCheckReply(QNetworkReply *reply);
    void onDownloadReadyRead();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();

private:
    QString pickAssetForPlatform(const QJsonArray &assets) const;
    static bool isNewer(const QString &candidate, const QString &current);

    QNetworkAccessManager *m_nam            = nullptr;
    QNetworkReply         *m_downloadReply  = nullptr;
    QFile                 *m_outFile        = nullptr;

    QString m_repoOwner;
    QString m_repoName;
    QString m_currentVersion;
    QString m_latestVersion;
    QUrl    m_downloadUrl;
    QString m_savedFilePath;

    bool    m_downloading  = false;
};

#endif // APPUPDATER_H
