// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/forward/ayu_queue_manager.h"
#include "ayu/features/forward/ayu_sync.h"
#include "ayu/features/forward/ayu_forward.h"
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
	const auto peerId = _action.history->peer->id;
	crl::on_main([=] {
		if (auto bar = AyuForward::getStatusBar(peerId)) {
			bar->setProgress(current, total, text);
			if (!logText.isEmpty()) {
				bar->addLogLine(logText);
			}
		}
	});
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

void QueueManager::runQueue() {
	AyuLogger::log("Iniciando processamento da fila de encaminhamento consecutiva...");

	int totalMessages = _items.size();
	updateStatus(0, totalMessages, "Processando...", "Iniciando encaminhamento...");

	for (int i = 0; i < totalMessages; i++) {
		const auto item = _items[i];
		updateStatus(i, totalMessages, "Processando...", QString("Processando mensagem %1 de %2").arg(i + 1).arg(totalMessages));

		// Encontrar todos os itens deste grupo se fizer parte de um álbum
		std::vector<not_null<HistoryItem*>> groupItems;
		groupItems.push_back(item);
		const auto groupId = item->groupId();
		if (groupId.value) {
			for (size_t k = i + 1; k < totalMessages; ++k) {
				const auto nextItem = _items[k];
				if (nextItem->groupId() != groupId) {
					break;
				}
				groupItems.push_back(nextItem);
			}
		}

		// 1. Download all media in this group/album first
		std::vector<QString> mediaPaths;
		bool downloadSuccess = true;
		for (const auto &gItem : groupItems) {
			if (mediaDownloadable(gItem->media())) {
				QString path = AyuSync::filePath(_session, gItem->media());
				mediaPaths.push_back(path);
				AyuLogger::log(QString("Baixando mídia para mensagem %1 em: %2").arg(gItem->id.bare).arg(path));

				try {
					std::vector<not_null<HistoryItem*>> singleItemVec = { gItem };
					AyuSync::loadDocuments(_session, singleItemVec);
					if (!(QFile::exists(path) && (QFile(path).size() > 0))) {
						downloadSuccess = false;
					}
				} catch (...) {
					downloadSuccess = false;
				}

				if (!downloadSuccess) {
					AyuLogger::logError(QString("Falha ao baixar mídia para mensagem %1.").arg(gItem->id.bare));
					break;
				}
				AyuLogger::log(QString("Download concluído para mensagem %1").arg(gItem->id.bare));
			}
		}

		if (!downloadSuccess) {
			updateStatus(i, totalMessages, "Falha no download", QString("Erro ao baixar mídia da mensagem %1").arg(i + 1));
			return;
		}

		// 2. Upload the message
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

		int startIndex = i;
		try {
			if (!mediaDownloadable(item->media())) {
				AyuSync::sendMessageSync(_session, std::move(message));
			} else if (const auto media = item->media()) {
				if (media->poll()) {
					AyuSync::sendMessageSync(_session, std::move(message));
				} else {
					std::vector<not_null<Data::Media*>> groupMedia;
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

					// Remove unfinished files
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
			}
			AyuLogger::log(QString("Upload confirmado para mensagem %1").arg(startIndex + 1));
		} catch (...) {
			AyuLogger::logError(QString("Falha ao enviar mensagem %1").arg(startIndex + 1));
		}

		// 3. Delete downloaded file immediately before moving to next item
		for (const auto &path : mediaPaths) {
			if (QFile::exists(path)) {
				QFile::remove(path);
				AyuLogger::log(QString("Arquivo temporário removido: %1").arg(path));
			}
		}

		// Advance index by processed group size
		i += groupItems.size() - 1;
	}

	updateStatus(totalMessages, totalMessages, "Finalizado", "Encaminhamento concluído com sucesso!");
}

void QueueManager::processDownloads() {}
void QueueManager::runDownloadTask(not_null<HistoryItem*> item) {}
void QueueManager::processUploads() {}

} // namespace AyuForward
