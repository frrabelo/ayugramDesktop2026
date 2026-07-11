// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/forward/ayu_sync.h"

#include "apiwrap.h"
#include "api/api_sending.h"
#include "ayu/utils/telegram_helpers.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "core/file_utilities.h"
#include "data/data_document.h"
#include "data/data_photo.h"
#include "data/data_photo_media.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "storage/file_download_mtproto.h"
#include "storage/localimageloader.h"

namespace AyuSync {

QString pathForSave(not_null<Main::Session*> session) {
	auto path = Core::App().settings().downloadPath();
	if (path.isEmpty()) {
		path = File::DefaultDownloadPath(session);
	}
	if (path == FileDialog::Tmp()) {
		path = session->local().tempDirectory();
	}
	if (!path.endsWith('/') && !path.endsWith('\\')) {
		path += '/';
	}
	path += "AyuGram Desktop/";
	QDir().mkpath(path);
	return path;
}

QString filePath(not_null<Main::Session*> session, const Data::Media *media) {
	if (!media) {
		return {};
	}

	if (const auto document = media->document()) {
		QString baseName = document->filename();
		if (baseName.isEmpty()) {
			if (document->isVoiceMessage()) {
				baseName = "audio_" + QString::number(document->getDC()) + "_" +
					QString::number(document->id) + ".ogg";
			} else if (document->isVideoMessage()) {
				baseName = "round_" + QString::number(document->getDC()) + "_" +
					QString::number(document->id) + ".mp4";
			} else if (document->isGifv()) {
				baseName = "gif_" + QString::number(document->getDC()) + "_" +
					QString::number(document->id) + ".gif";
			} else if (document->isVideoFile()) {
				baseName = "video_" + QString::number(document->getDC()) + "_" +
					QString::number(document->id) + ".mp4";
			} else {
				baseName = "file_" + QString::number(document->getDC()) + "_" +
					QString::number(document->id);
			}
		}
		return pathForSave(session) + baseName;
	} else if (const auto photo = media->photo()) {
		return pathForSave(session) + QString::number(photo->getDC()) + "_" + QString::number(photo->id) + ".jpg";
	}

	return {};
}

qint64 fileSize(not_null<HistoryItem*> item) {
	if (const auto path = filePath(&item->history()->session(), item->media()); !path.isEmpty()) {
		QFile file(path);
		if (file.exists()) {
			auto size = file.size();
			return size;
		}
	}
	return 0;
}

void loadDocuments(not_null<Main::Session*> session, const std::vector<not_null<HistoryItem*>> &items) {
	for (const auto &item : items) {
		auto latch = std::make_shared<TimedCountDownLatch>(1);
		crl::on_main([=] {
			if (const auto media = item->media()) {
				if (const auto data = media->document()) {
					const auto size = fileSize(item);
					if (size == data->size) {
						latch->countDown();
						return;
					}
					if (size && size < data->size) {
						QFile file(filePath(session, media));
						file.remove();
					}
					loadDocumentSync(session, data, item, latch);
				} else if (const auto photo = media->photo()) {
					const auto size = fileSize(item);
					if (size == photo->imageByteSize(Data::PhotoSize::Large)) {
						latch->countDown();
						return;
					}
					loadPhotoSync(session, std::pair(photo, item->fullId()), latch);
				} else {
					latch->countDown();
				}
			} else {
				latch->countDown();
			}
		});
		latch->await(std::chrono::minutes(15));
	}
}

void loadDocumentSync(not_null<Main::Session*> session, DocumentData *data, not_null<HistoryItem*> item, const std::shared_ptr<TimedCountDownLatch> &latch) {
	auto lifetime = std::make_shared<rpl::lifetime>();

	auto path = filePath(session, item->media());
	if (path.isEmpty()) {
		latch->countDown();
		return;
	}
	const auto fullId = item->fullId();
	const auto expectedSize = data->size;

	data->save(Data::FileOriginMessage(fullId), path);

	session->downloaderTaskFinished() | rpl::filter([=]
	{
		QFile file(path);
		qint64 size = file.exists() ? file.size() : 0;
		return !data || data->status == FileDownloadFailed || size == expectedSize;
	}) | rpl::on_next([=]() mutable
	{
		latch->countDown();
		crl::on_main([lifetime] {
			lifetime->destroy();
		});
	}, *lifetime);
}

void forwardMessagesSync(not_null<Main::Session*> session,
						 const std::vector<not_null<HistoryItem*>> &items,
						 const ApiWrap::SendAction &action,
						 Data::ForwardOptions options) {
	auto latch = std::make_shared<TimedCountDownLatch>(1);

	crl::on_main([=]
	{
		session->api().forwardMessages(Data::ResolvedForwardDraft(items, options),
									   action,
									   [=]
									   {
										   latch->countDown();
									   });
	});


	latch->await(std::chrono::minutes(1));
}

void loadPhotoSync(not_null<Main::Session*> session, const std::pair<not_null<PhotoData*>, FullMsgId> &photo, const std::shared_ptr<TimedCountDownLatch> &latch) {
	const auto folderPath = pathForSave(session);
	const auto downloadPath = folderPath.isEmpty() ? Core::App().settings().downloadPath() : folderPath;

	const auto path = downloadPath.isEmpty()
						  ? File::DefaultDownloadPath(session)
						  : downloadPath == FileDialog::Tmp()
								? session->local().tempDirectory()
								: downloadPath;
	if (path.isEmpty()) {
		latch->countDown();
		return;
	}
	if (!QDir().mkpath(path)) {
		latch->countDown();
		return;
	}

	auto lifetime = std::make_shared<rpl::lifetime>();

	const auto view = photo.first->createMediaView();
	if (!view) {
		latch->countDown();
		return;
	}
	view->wanted(Data::PhotoSize::Large, photo.second);

	const auto finalCheck = [=]
	{
		return !photo.first->loading();
	};

	const auto saveToFiles = [=]
	{
		if (!view->loaded()) {
			return;
		}
		QDir directory(path);
		const auto dir = directory.absolutePath();
		const auto nameBase = dir.endsWith('/') ? dir : dir + '/';
		const auto fullPath = nameBase + QString::number(photo.first->getDC()) + "_" + QString::number(photo.first->id)
			+ ".jpg";
		view->saveToFile(fullPath);
	};

	if (finalCheck()) {
		saveToFiles();
		latch->countDown();
	} else {
		session->downloaderTaskFinished() | rpl::filter([=]
		{
			return finalCheck();
		}) | rpl::on_next([=]() mutable
		{
			saveToFiles();
			latch->countDown();
			crl::on_main([lifetime] {
				lifetime->destroy();
			});
		}, *lifetime);
	}
}
}

void sendMessageSync(not_null<Main::Session*> session, Api::MessageToSend &&message) {
	const auto action = message.action;
	crl::on_main([=, message = std::move(message)]() mutable
	{
		// we cannot send events to objects
		// owned by a different thread
		// because sendMessage updates UI too

		session->api().sendMessage(std::move(message));
	});


	waitForMsgSync(session, action);
}

void waitForMsgSync(not_null<Main::Session*> session, const Api::SendAction &action) {
	auto latch = std::make_shared<TimedCountDownLatch>(1);
	auto lifetime = std::make_shared<rpl::lifetime>();

	const auto expectedPeerId = action.history->peer->id;

	crl::on_main([=]
	{
		session->data().itemIdChanged()
			| rpl::filter([=](const Data::Session::IdChange &update)
			{
				return expectedPeerId == update.newId.peer;
			}) | rpl::on_next([=]
			{
				latch->countDown();
			}, *lifetime);
	});

	latch->await(std::chrono::minutes(5));
	crl::on_main([=] {
		lifetime->destroy();
	});
}

void sendDocumentSync(not_null<Main::Session*> session,
					  Ui::PreparedGroup &group,
					  SendMediaType type,
					  TextWithTags &&caption,
					  const Api::SendAction &action) {
	auto groupId = std::make_shared<SendingAlbum>();
	groupId->groupId = base::RandomValue<uint64>();

	crl::on_main([=, lst = std::move(group.list), caption = std::move(caption)]() mutable
	{
		auto size = lst.files.size();
		if (!lst.files.empty()) {
			lst.files.front().caption = std::move(caption);
		}
		session->api().sendFiles(
			std::move(lst),
			type,
			size > 1 ? groupId : nullptr,
			action);
	});

	waitForMsgSync(session, action);
}

void sendStickerSync(not_null<Main::Session*> session,
					 Api::MessageToSend &&message,
					 not_null<DocumentData*> document) {
	const auto action = message.action;
	crl::on_main([=, message = std::move(message)]() mutable
	{
		Api::SendExistingDocument(std::move(message), document, std::nullopt);
	});

	waitForMsgSync(session, action);
}

void sendVoiceSync(not_null<Main::Session*> session,
				   const QByteArray &data,
				   int64_t duration,
				   bool video,
				   Api::MessageToSend &&message) {
	const auto action = message.action;

	crl::on_main([=]
	{
		const auto to = FileLoadTo(
			action.history->peer->id,
			action.options,
			action.replyTo,
			action.replaceMediaOf);
		session->api().fileLoader()->addTask(std::make_unique<FileLoadTask>(FileLoadTask::VoiceArgs{
			.session = session,
			.voice = data,
			.duration = duration,
			.waveform = QVector<signed char>(),
			.video = video,
			.to = to,
			.caption = message.textWithTags
		}));
	});
	waitForMsgSync(session, action);
}

} // namespace AyuSync
