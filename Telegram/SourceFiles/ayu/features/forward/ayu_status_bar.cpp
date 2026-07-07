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
	return _expanded ? 120 : 66;
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

	// Background (glassmorphism/dark tone matching the layout)
	p.fillRect(rect(), QColor(26, 32, 40, 255));

	// Full white border with margin as shown in the example images
	QRect borderRect = rect().adjusted(10, 5, -10, -5);
	p.setPen(QPen(Qt::white, 2));
	p.setBrush(Qt::NoBrush);
	p.drawRect(borderRect);

	// Draw Title "Status do sistema:"
	p.setPen(Qt::white);
	QFont titleFont = p.font();
	titleFont.setBold(true);
	titleFont.setPointSize(10);
	p.setFont(titleFont);
	p.drawText(20, 24, "Status do sistema:");

	// Draw Log lines
	p.setPen(QColor(220, 220, 225));
	QFont textFont = p.font();
	textFont.setBold(false);
	textFont.setPointSize(9);
	p.setFont(textFont);

	int startY = 40;
	int stepY = 16;

	if (!_recentLogs.isEmpty()) {
		int linesToShow = _expanded ? std::min(5, int(_recentLogs.size())) : std::min(2, int(_recentLogs.size()));
		for (int i = 0; i < linesToShow; ++i) {
			int logIdx = _recentLogs.size() - linesToShow + i;
			p.drawText(20, startY + i * stepY, _recentLogs[logIdx]);
		}
	} else {
		// Fallback: show progress status if logs list is empty
		p.drawText(20, startY, _statusText);
		if (_totalProgress > 0) {
			p.drawText(20, startY + stepY, QString("Progresso: %1 / %2").arg(_currentProgress).arg(_totalProgress));
		}
	}

	// Layout Control Rects (interactive buttons)
	int btnWidth = 50;
	int btnHeight = 42;
	int pad = 20;

	// Logs Button (labeled "Log's" on the right side)
	_logsRect = QRect(width() - btnWidth - pad, (height() - btnHeight) / 2, btnWidth, btnHeight);
	p.setPen(Qt::NoPen);
	p.setBrush(QColor(100, 110, 120, 180)); // Grayish background
	p.drawRoundedRect(_logsRect, 8, 8);

	p.setPen(Qt::white);
	QFont btnFont = p.font();
	btnFont.setBold(false);
	btnFont.setPointSize(9);
	p.setFont(btnFont);
	p.drawText(_logsRect, Qt::AlignCenter, "Log's");

	// Expand Button (arrows to the left of Log's button)
	_expandRect = QRect(width() - btnWidth - pad - 30, (height() - btnHeight) / 2, 20, btnHeight);
	
	// Draw custom vertical stacked arrows
	int centerX = _expandRect.center().x();
	int centerY = _expandRect.center().y();

	p.setBrush(Qt::NoBrush);
	p.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

	// Up arrow
	QPainterPath upArrow;
	upArrow.moveTo(centerX - 5, centerY - 3);
	upArrow.lineTo(centerX, centerY - 9);
	upArrow.lineTo(centerX + 5, centerY - 3);
	p.drawPath(upArrow);

	// Down arrow
	QPainterPath downArrow;
	downArrow.moveTo(centerX - 5, centerY + 3);
	downArrow.lineTo(centerX, centerY + 9);
	downArrow.lineTo(centerX + 5, centerY + 3);
	p.drawPath(downArrow);
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
