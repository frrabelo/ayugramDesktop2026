// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "ui/rp_widget.h"
#include <QStringList>
#include <functional>

namespace AyuForward {

class AyuStatusBar : public Ui::RpWidget
{
	Q_OBJECT

public:
	explicit AyuStatusBar(QWidget *parent, std::function<void()> resizeCallback = nullptr);
	~AyuStatusBar() override;

	void addLogLine(const QString &line);
	void clearLogs();
	void setProgress(int current, int total, const QString &statusText);
	int desiredHeight() const;

protected:
	void paintEvent(QPaintEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;

private:
	bool _expanded = false;
	QStringList _recentLogs;
	int _currentProgress = 0;
	int _totalProgress = 0;
	QString _statusText;

	QRect _expandRect;
	QRect _logsRect;
	std::function<void()> _resizeCallback;

	void updateHeight();
};

} // namespace AyuForward
