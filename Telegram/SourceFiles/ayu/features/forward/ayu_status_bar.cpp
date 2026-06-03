// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/forward/ayu_status_bar.h"
#include "ayu/utils/ayu_logger.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDesktopServices>

namespace AyuForward {

AyuStatusBar::AyuStatusBar(QWidget *parent, std::function<void()> resizeCallback)
	: RpWidget(parent)
	, _resizeCallback(resizeCallback)
{
	_statusText = "Iniciando...";
	// Set default size
	resize(parent->width(), desiredHeight());
}

AyuStatusBar::~AyuStatusBar() = default;

int AyuStatusBar::desiredHeight() const {
	return _expanded ? 140 : 45;
}

void AyuStatusBar::updateHeight() {
	int h = desiredHeight();
	resize(width(), h);
	if (_resizeCallback) {
		_resizeCallback();
	}
	update();
}

void AyuStatusBar::addLogLine(const QString &line) {
	_recentLogs.append(line);
	if (_recentLogs.size() > 20) {
		_recentLogs.removeFirst();
	}
	AyuLogger::log(line);
	update();
}

void AyuStatusBar::clearLogs() {
	_recentLogs.clear();
	update();
}

void AyuStatusBar::setProgress(int current, int total, const QString &statusText) {
	_currentProgress = current;
	_totalProgress = total;
	_statusText = statusText;
	update();
}

void AyuStatusBar::paintEvent(QPaintEvent *e) {
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	// Background (glassmorphism/dark tone)
	p.fillRect(rect(), QColor(26, 26, 30, 245));

	// Top and bottom subtle border lines
	p.setPen(QColor(255, 255, 255, 25));
	p.drawLine(0, 0, width(), 0);
	p.drawLine(0, height() - 1, width(), height() - 1);

	// Draw Title
	p.setPen(QColor(255, 255, 255));
	QFont titleFont = p.font();
	titleFont.setBold(true);
	titleFont.setPointSize(10);
	p.setFont(titleFont);
	p.drawText(15, 25, "Status do Sistema:");

	// Progress information
	QString progressStr = "";
	if (_totalProgress > 0) {
		progressStr = QString(" (%1/%2)").arg(_currentProgress).arg(_totalProgress);
	}
	QString statusFullText = _statusText + progressStr;

	titleFont.setBold(false);
	titleFont.setPointSize(9);
	p.setFont(titleFont);
	p.setPen(QColor(200, 200, 205));
	p.drawText(155, 25, statusFullText);

	// Draw Log lines if expanded
	if (_expanded) {
		p.setPen(QColor(170, 170, 175));
		int startY = 55;
		int stepY = 18;
		int linesToShow = std::min(4, _recentLogs.size());
		for (int i = 0; i < linesToShow; ++i) {
			// Show the most recent logs at the bottom
			int logIdx = _recentLogs.size() - linesToShow + i;
			p.drawText(20, startY + i * stepY, _recentLogs[logIdx]);
		}
	}

	// Layout Control Rects (interactive buttons)
	int btnWidth = 60;
	int btnHeight = 25;
	int pad = 10;

	// Logs Button
	_logsRect = QRect(width() - btnWidth * 2 - pad * 2, (45 - btnHeight) / 2, btnWidth, btnHeight);
	p.setPen(Qt::NoPen);
	p.setBrush(QColor(41, 121, 255, 50)); // Soft translucent blue
	p.drawRoundedRect(_logsRect, 4, 4);

	p.setPen(QColor(41, 121, 255));
	QFont btnFont = p.font();
	btnFont.setBold(true);
	p.setFont(btnFont);
	p.drawText(_logsRect, Qt::AlignCenter, "LOGS");

	// Expand Button (with vertical arrows)
	_expandRect = QRect(width() - btnWidth - pad, (45 - btnHeight) / 2, btnWidth, btnHeight);
	p.setBrush(QColor(255, 255, 255, 20));
	p.drawRoundedRect(_expandRect, 4, 4);

	p.setPen(QColor(220, 220, 225));
	int centerX = _expandRect.center().x();
	int centerY = _expandRect.center().y();

	// Draw custom vertical arrows (chevron style up/down)
	QPainterPath arrowPath;
	if (_expanded) {
		// Up arrow
		arrowPath.moveTo(centerX - 6, centerY + 3);
		arrowPath.lineTo(centerX, centerY - 3);
		arrowPath.lineTo(centerX + 6, centerY + 3);
	} else {
		// Down arrow
		arrowPath.moveTo(centerX - 6, centerY - 3);
		arrowPath.lineTo(centerX, centerY + 3);
		arrowPath.lineTo(centerX + 6, centerY - 3);
	}
	p.setBrush(Qt::NoBrush);
	p.setPen(QPen(QColor(220, 220, 225), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	p.drawPath(arrowPath);
}

void AyuStatusBar::mousePressEvent(QMouseEvent *e) {
	if (e->button() == Qt::LeftButton) {
		if (_logsRect.contains(e->pos())) {
			AyuLogger::openLogsFolder();
			e->accept();
		} else if (_expandRect.contains(e->pos())) {
			_expanded = !_expanded;
			updateHeight();
			e->accept();
		}
	}
	RpWidget::mousePressEvent(e);
}

void AyuStatusBar::resizeEvent(QResizeEvent *e) {
	RpWidget::resizeEvent(e);
	update();
}

} // namespace AyuForward
