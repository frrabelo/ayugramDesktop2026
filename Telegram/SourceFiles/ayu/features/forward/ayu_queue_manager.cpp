// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/forward/ayu_queue_manager.h"
#include "ayu/features/forward/ayu_sync.h"
#include "ayu/utils/ayu_logger.h"
#include "ayu/utils/telegram_helpers.h"

#include <QThreadPool>
#include <QRunnable>
#include <QFile>
#include <QDir>
#include <QMutexLocker>
#include "data/data_document.h"
#include "data/data_photo.h"
#include "ui/chat/attach/attach_prepare.h"
#include "styles/style_boxes.h"
#include "storage/storage_media_prepare.h"
#include "storage/localimageloader.h"
#include "data/data_session.h"
#include "history/history.h"

namespace AyuForward {

class DownloadRunnable : public QRunnable {
public:
	DownloadRunnable(std::function<void()> func) : _func(func) {}
	void run() override {
		_func();
	}
private:
	std::function<void()> _func;
};

// Check if a media file is downloadable and not already downloaded
static bool mediaDownloadable(Data::Media *media) {
	if (!media) {
		return false;
	}
	return media->document() != nullptr || media->photo() != nullptr;
}

QueueManager::QueueManager(
	not_null<Main::Session*> session,
	const Api::SendAction &action,
	const std::vector<not_null<HistoryItem*>> &items,
	Data::ForwardOptions options,
	AyuStatusBar *statusBar)
	: _session(session)
	, _action(action)
	, _items(items)
	, _options(options)
	, _statusBar(statusBar)
{
	_hardware = AyuHardware::detectHardware();
	_maxParallelDownloads = AyuHardware::calculateMaxParallelDownloads(_hardware);

	AyuLogger::initialize();
	AyuLogger::log(QString("Detectado hardware: %1 CPU Threads, %2 MB RAM, %3")
		.arg(_hardware.cpuThreads)
		.arg(_hardware.totalRamMb)
		.arg(_hardware.gpuName));
	AyuLogger::log(QString("Concorrência de download definida para: %1 threads paralelas").arg(_maxParallelDownloads));
}

QueueManager::~QueueManager() {
	cleanupFiles();
}

void QueueManager::addDownloadedFile(const QString &path) {
	QMutexLocker locker(&_filesMutex);
	if (!path.isEmpty() && !std::count(_downloadedFiles.begin(), _downloadedFiles.end(), path)) {
		_downloadedFiles.push_back(path);
	}
}

void QueueManager::cleanupFiles() {
	QMutexLocker locker(&_filesMutex);
	for (const auto &path : _downloadedFiles) {
		if (QFile::exists(path)) {
			QFile::remove(path);
			AyuLogger::log(QString("Limpeza: Arquivo temporário removido: %1").arg(path));
		}
	}
	_downloadedFiles.clear();
}

void QueueManager::updateStatus(int current, int total, const QString &text, const QString &logText) {
	if (!logText.isEmpty()) {
		AyuLogger::log(logText);
	}
	if (_statusBar) {
		crl::on_main([=] {
			_statusBar->setProgress(current, total, text);
			if (!logText.isEmpty()) {
				_statusBar->addLogLine(logText);
			}
		});
	}
}

void QueueManager::runQueue() {
	AyuLogger::log("Iniciando processamento da fila de encaminhamento...");

	// 1. Prepare download queue
	for (const auto &item : _items) {
		if (mediaDownloadable(item->media())) {
			_downloadQueue.push_back(item);
		}
	}

	_totalDownloads = _downloadQueue.size();
	_completedDownloads = 0;
	_activeDownloads = 0;

	if (_totalDownloads > 0) {
		updateStatus(0, _totalDownloads, "Baixando mídias...", QString("Iniciando download de %1 arquivos em paralelo...").arg(_totalDownloads));
		processDownloads();

		// Wait for all downloads to finish
		QMutexLocker locker(&_downloadMutex);
		while (_completedDownloads < _totalDownloads && !_downloadFailed) {
			_downloadFinishedCond.wait(&_downloadMutex);
		}

		if (_downloadFailed) {
			updateStatus(0, 0, "Falha no download", "Cancelando encaminhamento devido a falha no download de mídia.");
			return;
		}
		AyuLogger::log("Todos os downloads foram concluídos.");
	}

	// 2. Process uploads sequentially to preserve order
	processUploads();
}

void QueueManager::processDownloads() {
	QMutexLocker locker(&_downloadMutex);
	while (_activeDownloads < _maxParallelDownloads && _completedDownloads + _activeDownloads < _totalDownloads) {
		int index = _completedDownloads + _activeDownloads;
		not_null<HistoryItem*> item = _downloadQueue[index];
		_activeDownloads++;

		auto task = [this, item]() {
			runDownloadTask(item);
		};
		QThreadPool::globalInstance()->start(new DownloadRunnable(task));
	}
}

void QueueManager::runDownloadTask(not_null<HistoryItem*> item) {
	QString path = AyuSync::filePath(_session, item->media());
	AyuLogger::log(QString("Baixando arquivo para mensagem %1 em: %2").arg(item->id.bare).arg(path));

	bool success = false;
	try {
		std::vector<not_null<HistoryItem*>> singleItemVec = { item };
		AyuSync::loadDocuments(_session, singleItemVec);
		success = QFile::exists(path) && (QFile(path).size() > 0);
	} catch (...) {
		success = false;
	}

	QMutexLocker locker(&_downloadMutex);
	_activeDownloads--;

	if (success) {
		_completedDownloads++;
		addDownloadedFile(path);
		updateStatus(_completedDownloads, _totalDownloads, "Baixando mídias...", QString("Download concluído para mensagem %1").arg(item->id.bare));
	} else {
		_downloadFailed = true;
		AyuLogger::logError(QString("Falha ao baixar mídia para mensagem %1").arg(item->id.bare));
	}

	_downloadFinishedCond.wakeAll();

	// Trigger next download in queue
	locker.unlock();
	processDownloads();
}

static Ui::PreparedList prepareMedia(not_null<Main::Session*> session,
									 const std::vector<not_null<HistoryItem*>> &items,
									 int &i,
									 std::vector<not_null<Data::Media*>> &groupMedia) {
	const auto prepare = [&](not_null<Data::Media*> media)
	{
		groupMedia.emplace_back(media);
		auto prepared = Ui::PreparedFile(AyuSync::filePath(session, media));
		if (prepared.path.isEmpty()) {
			return prepared;
		}
		Storage::PrepareDetails(prepared, st::sendMediaPreviewSize, PhotoSideLimit());
		return prepared;
	};

	const auto startItem = items[i];
	const auto media = startItem->media();
	const auto groupId = startItem->groupId();

	Ui::PreparedList list;
	if (auto prepared = prepare(media); !prepared.path.isEmpty()) {
		list.files.emplace_back(std::move(prepared));
	}

	if (!groupId.value) {
		return list;
	}

	for (size_t k = i + 1; k < items.size(); ++k) {
		const auto nextItem = items[k];
		if (nextItem->groupId() != groupId) {
			break;
		}
		if (const auto nextMedia = nextItem->media()) {
			if (auto prepared = prepare(nextMedia); !prepared.path.isEmpty()) {
				list.files.emplace_back(std::move(prepared));
			}
			i = k;
		}
	}
	return list;
}

static void sendMedia(
	not_null<Main::Session*> session,
	const std::shared_ptr<Ui::PreparedBundle> &bundle,
	not_null<Data::Media*> primaryMedia,
	Api::MessageToSend &&message,
	bool sendImagesAsPhotos) {
	if (const auto document = primaryMedia->document(); document && document->sticker()) {
		AyuSync::sendStickerSync(session, std::move(message), document);
		return;
	}

	auto mediaType = [&]
	{
		if (const auto document = primaryMedia->document()) {
			if (document->isVoiceMessage()) {
				return SendMediaType::Audio;
			} else if (document->isVideoMessage()) {
				return SendMediaType::Round;
			} else if (document->isVideoFile() || document->isGifv()) {
				return SendMediaType::Photo;
			}
			return SendMediaType::File;
		}
		return SendMediaType::Photo;
	}();

	if (mediaType == SendMediaType::Round || mediaType == SendMediaType::Audio) {
		const auto path = bundle->groups.front().list.files.front().path;

		QFile file(path);
		auto failed = false;
		if (!file.open(QIODevice::ReadOnly)) {
			failed = true;
		}
		auto data = file.readAll();

		if (!failed && data.size()) {
			file.close();
			AyuSync::sendVoiceSync(session,
								   data,
								   primaryMedia->document()->duration(),
								   mediaType == SendMediaType::Round,
								   std::move(message));
			return;
		}
	}

	if (sendImagesAsPhotos) {
		mediaType = SendMediaType::Photo;
	}

	for (auto &group : bundle->groups) {
		AyuSync::sendDocumentSync(
			session,
			group,
			mediaType,
			std::move(message.textWithTags),
			message.action);
	}
}

void QueueManager::processUploads() {
	int totalMessages = _items.size();
	updateStatus(0, totalMessages, "Enviando mensagens...", "Iniciando upload de mensagens...");

	for (int i = 0; i < totalMessages; i++) {
		const auto item = _items[i];
		updateStatus(i, totalMessages, "Enviando mensagens...", QString("Enviando mensagem %1 de %2").arg(i + 1).arg(totalMessages));

		auto extractedText = extractText(item);
		if (extractedText.empty() && !mediaDownloadable(item->media())) {
			continue;
		}

		auto message = Api::MessageToSend(Api::SendAction(_session->data().history(_action.history->peer->id)));
		message.action.options.invertCaption = item->invertMedia();
		message.action.replyTo = _action.replyTo;

		if (_options != Data::ForwardOptions::NoNamesAndCaptions) {
			message.textWithTags = extractedText;
		}

		QString mediaPath = mediaDownloadable(item->media()) ? AyuSync::filePath(_session, item->media()) : "";

		try {
			if (!mediaDownloadable(item->media())) {
				AyuSync::sendMessageSync(_session, std::move(message));
			} else if (const auto media = item->media()) {
				if (media->poll()) {
					AyuSync::sendMessageSync(_session, std::move(message));
					continue;
				}

				std::vector<not_null<Data::Media*>> groupMedia;
				// i will be incremented in prepareMedia if there are group items
				int startIndex = i;
				auto preparedMedia = prepareMedia(_session, _items, i, groupMedia);

				Ui::SendFilesWay way;
				way.setGroupFiles(true);
				way.setSendImagesAsPhotos(false);
				for (const auto &media2 : groupMedia) {
					if (media2->photo()) {
						way.setSendImagesAsPhotos(true);
						break;
					}
				}

				// remove unfinished files
				for (int j = preparedMedia.files.size() - 1; j >= 0; j--) {
					auto &file = preparedMedia.files[j];
					QFile f(file.path);
					if ((groupMedia[j]->photo() && f.size() < groupMedia[j]->photo()->imageByteSize(Data::PhotoSize::Large)) ||
						(groupMedia[j]->document() && f.size() < groupMedia[j]->document()->size)) {
						preparedMedia.files.erase(preparedMedia.files.begin() + j);
					}
				}

				if (!preparedMedia.files.empty()) {
					auto groups = Ui::DivideByGroups(std::move(preparedMedia), way, _action.history->peer->slowmodeApplied());
					auto bundle = Ui::PrepareFilesBundle(std::move(groups), way, false);
					sendMedia(_session, bundle, media, std::move(message), way.sendImagesAsPhotos());
				}
			}

			// Delete temporary files for the sent items immediately
			QMutexLocker locker(&_filesMutex);
			if (!mediaPath.isEmpty() && QFile::exists(mediaPath)) {
				QFile::remove(mediaPath);
				// remove from downloaded list so we don't try to delete it again
				auto it = std::find(_downloadedFiles.begin(), _downloadedFiles.end(), mediaPath);
				if (it != _downloadedFiles.end()) {
					_downloadedFiles.erase(it);
				}
				AyuLogger::log(QString("Upload confirmado: Arquivo temporário removido: %1").arg(mediaPath));
			}
		} catch (...) {
			AyuLogger::logError(QString("Falha ao enviar mensagem %1").arg(i + 1));
		}
	}

	updateStatus(totalMessages, totalMessages, "Finalizado", "Encaminhamento concluído com sucesso!");
}

} // namespace AyuForward
