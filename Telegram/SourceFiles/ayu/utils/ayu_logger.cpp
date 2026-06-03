// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/utils/ayu_logger.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QMutex>
#include <QMutexLocker>

namespace AyuLogger {

QMutex logMutex;

QString getLogsDirectory() {
	QString appData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	if (appData.isEmpty()) {
		appData = QDir::tempPath();
	}
	QString logsDir = appData + "/ayu_logs";
	QDir().mkpath(logsDir);
	return logsDir;
}

QString getLogFilePath() {
	return getLogsDirectory() + "/ayugram_forward_logs.txt";
}

void writeLog(const QString &level, const QString &message) {
	QMutexLocker locker(&logMutex);
	QString filePath = getLogFilePath();

	// Check for rotation if file is too big (> 10MB)
	QFile checkFile(filePath);
	if (checkFile.exists() && checkFile.size() > 10 * 1024 * 1024) {
		checkFile.remove();
	}

	if (checkFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
		QTextStream stream(&checkFile);
		QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
		stream << "[" << timestamp << "] [" << level << "] " << message << "\n";
		checkFile.close();
	}
}

void initialize() {
	writeLog("INFO", "=== AyuGram Logger Initialized ===");
}

void log(const QString &message) {
	writeLog("INFO", message);
}

void logError(const QString &message) {
	writeLog("ERROR", message);
}

void openLogsFolder() {
	QString logsDir = getLogsDirectory();
	QDesktopServices::openUrl(QUrl::fromLocalFile(logsDir));
}

} // namespace AyuLogger
