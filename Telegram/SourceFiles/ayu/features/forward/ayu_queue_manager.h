// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "history/history_item.h"
#include "api/api_common.h"
#include "main/main_session.h"
#include "ayu/features/forward/ayu_status_bar.h"
#include "ayu/utils/ayu_hardware_optimizer.h"
#include <vector>
#include <QString>
#include <QMutex>
#include <QWaitCondition>

namespace AyuForward {

class QueueManager {
public:
	QueueManager(
		not_null<Main::Session*> session,
		const Api::SendAction &action,
		const std::vector<not_null<HistoryItem*>> &items,
		Data::ForwardOptions options,
		AyuStatusBar *statusBar = nullptr);

	~QueueManager();

	void runQueue();

private:
	not_null<Main::Session*> _session;
	Api::SendAction _action;
	std::vector<not_null<HistoryItem*>> _items;
	Data::ForwardOptions _options;
	AyuStatusBar *_statusBar;

	AyuHardware::HardwareInfo _hardware;
	int _maxParallelDownloads;

	std::vector<not_null<HistoryItem*>> _downloadQueue;
	int _totalDownloads = 0;
	int _completedDownloads = 0;
	int _activeDownloads = 0;
	bool _downloadFailed = false;

	QMutex _downloadMutex;
	QWaitCondition _downloadFinishedCond;

	// Files that were downloaded specifically by this operation and need to be deleted
	std::vector<QString> _downloadedFiles;
	QMutex _filesMutex;

	void processDownloads();
	void runDownloadTask(not_null<HistoryItem*> item);
	void processUploads();
	void addDownloadedFile(const QString &path);
	void cleanupFiles();

	void updateStatus(int current, int total, const QString &text, const QString &logText = "");
};

} // namespace AyuForward
