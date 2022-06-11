/*
	SPDX-FileCopyrightText: 2013-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "daily_progress_dialog.h"

#include "daily_progress.h"
#include "preferences.h"

#include <QApplication>
#include <QColor>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QSettings>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

//-----------------------------------------------------------------------------

class DailyProgressDialog::Delegate : public QStyledItemDelegate
{
public:
	explicit Delegate(QObject* parent = nullptr)
		: QStyledItemDelegate(parent)
	{
	}

	void changeEvent(QEvent* event)
	{
		if ((event->type() == QEvent::PaletteChange) || (event->type() == QEvent::StyleChange)) {
			m_pixmap = QPixmap();
		}
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);

		if (opt.text.isEmpty()) {
			return;
		} else if ((index.column() > 0) && (index.column() < 8)) {
			opt.rect = opt.rect.adjusted(2,2,-2,-2);

			const int progress = qBound(0, index.data(Qt::UserRole).toInt(), 100);
			if (progress == 0) {
				opt.backgroundBrush = opt.palette.alternateBase();
			} else if (progress == 100) {
				const qreal pixelratio = painter->device()->devicePixelRatioF();
				painter->drawPixmap(QPointF(opt.rect.topLeft()), fetchStarBackground(opt, pixelratio));
				opt.font.setBold(true);
			} else {
				const qreal k = (progress * 0.009) + 0.1;
				const qreal ik = 1.0 - k;
				const QColor base = opt.palette.color(QPalette::Active, QPalette::AlternateBase);
				const QColor highlight = opt.palette.color(QPalette::Active, QPalette::Highlight);
				opt.backgroundBrush = QColor(std::lround((highlight.red() * k) + (base.red() * ik)),
						std::lround((highlight.green() * k) + (base.green() * ik)),
						std::lround((highlight.blue() * k) + (base.blue() * ik)));
				opt.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Active, QPalette::Text));
			}

			if (progress >= 50) {
				opt.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Active, QPalette::HighlightedText));
			}
		} else {
			opt.backgroundBrush = opt.palette.color(QPalette::Active, QPalette::Base);
		}

		const QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
		style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
	}

private:
	QPixmap fetchStarBackground(const QStyleOptionViewItem& option, const qreal pixelratio) const
	{
		if (m_pixmap.size() != (option.rect.size() * pixelratio)) {
			// Create success background image
			QStyleOptionViewItem opt = option;
			opt.rect = QRect(QPoint(0,0), opt.rect.size());
			m_pixmap = QPixmap(opt.rect.size() * pixelratio);
			m_pixmap.setDevicePixelRatio(pixelratio);

			// Draw view item background
			QPainter p(&m_pixmap);
			p.fillRect(QRectF(opt.rect), opt.palette.color(QPalette::Active, QPalette::Base));
			opt.backgroundBrush = opt.palette.color(QPalette::Active, QPalette::Highlight);
			const QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
			style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, &p, opt.widget);

			// Create star
			const qreal pi = acos(-1.0);
			static const QPolygonF star{
				{ 0.0, -1.0 },
				{ cos(0.3 * pi), sin(0.3 * pi) },
				{ cos(1.1 * pi), sin(1.1 * pi) },
				{ cos(1.9 * pi), sin(1.9 * pi) },
				{ cos(2.7 * pi), sin(2.7 * pi) }
			};

			// Draw star
			p.setRenderHint(QPainter::Antialiasing);
			p.setPen(Qt::NoPen);
			QColor background = opt.palette.color(QPalette::Active, QPalette::HighlightedText);
			background.setAlpha(64);
			p.setBrush(background);
			const qreal size = (opt.rect.width() * 0.5) - 2.0;
			const qreal offset = 2.0 + size;
			p.translate(offset, offset);
			p.scale(size, size);
			p.drawPolygon(star, Qt::WindingFill);
		}
		return m_pixmap;
	}

private:
	mutable QPixmap m_pixmap;
};

//-----------------------------------------------------------------------------

DailyProgressDialog::DailyProgressDialog(DailyProgress* progress, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
	, m_progress(progress)
{
	setWindowTitle(tr("Daily Progress"));

	// Set up tableview
	m_display = new QTableView(this);
	m_display->setModel(progress);
	m_display->setShowGrid(false);
	m_display->verticalHeader()->hide();
	m_display->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_display->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	connect(progress, &DailyProgress::modelReset, this, &DailyProgressDialog::modelReset);

	m_delegate = new Delegate(this);
	m_display->setItemDelegate(m_delegate);

	// Find maximum size needed to show day names
	m_display->horizontalHeader()->setMinimumSectionSize(0);
	int size = 0;
	for (int i = 1; i < 8; ++i) {
		size = std::max(size, m_display->horizontalHeader()->sectionSizeHint(i));
	}

	// Set rows to all be the same height
	for (int r = 0, count = m_progress->rowCount(); r < count; ++r) {
		m_display->setRowHeight(r, size);
	}

	// Make outside columns stretch and data columns fixed in size
	m_display->horizontalHeader()->setSectionsClickable(false);
	m_display->horizontalHeader()->setSectionsMovable(false);
	m_display->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_display->setColumnWidth(0, 0);
	for (int c = 1; c < 8; ++c) {
		m_display->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
		m_display->setColumnWidth(c, size);
	}
	m_display->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Stretch);
	m_display->setColumnWidth(8, 0);

	// Set minimum size to always show up to 5 weeks of data
	const int frame = (m_display->style()->pixelMetric(QStyle::PM_DefaultFrameWidth) * 2) + 4;
	const int min_width = m_display->fontMetrics().averageCharWidth() * 10;
	m_display->setMinimumWidth(size * 7 + frame + m_display->verticalScrollBar()->sizeHint().width() + min_width);
	m_display->setMinimumHeight((size * 5) + frame + m_display->horizontalHeader()->sizeHint().height());
	m_display->scrollToBottom();

	// Set up streak labels
	m_streaks = new QWidget(this);
	m_streaks->setVisible(false);

	m_longest_streak = new QLabel(m_streaks);

	QFrame* streak_divider = new QFrame(m_streaks);
	streak_divider->setFrameStyle(QFrame::Sunken | QFrame::VLine);

	m_current_streak = new QLabel(m_streaks);

	QHBoxLayout* streaks_layout = new QHBoxLayout(m_streaks);
	streaks_layout->setContentsMargins(0, 0, 0, 0);
	streaks_layout->addWidget(m_longest_streak);
	streaks_layout->addWidget(streak_divider);
	streaks_layout->addWidget(m_current_streak);

	connect(m_progress, &DailyProgress::streaksChanged, this, &DailyProgressDialog::streaksChanged);
	streaksChanged();

	// Lay out dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_display);
	layout->addWidget(m_streaks);

	// Restore size
	resize(QSettings().value("DailyProgressDialog/Size", sizeHint()).toSize());
}

//-----------------------------------------------------------------------------

void DailyProgressDialog::loadPreferences()
{
	m_streaks->setVisible(Preferences::instance().goalStreaks());
}

//-----------------------------------------------------------------------------

void DailyProgressDialog::changeEvent(QEvent* event)
{
	m_delegate->changeEvent(event);
	QDialog::changeEvent(event);
}

//-----------------------------------------------------------------------------

void DailyProgressDialog::closeEvent(QCloseEvent* event)
{
	QSettings().setValue("DailyProgressDialog/Size", size());
	QDialog::closeEvent(event);
}

//-----------------------------------------------------------------------------

void DailyProgressDialog::hideEvent(QHideEvent* event)
{
	Q_EMIT visibleChanged(false);

	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------

void DailyProgressDialog::showEvent(QShowEvent* event)
{
	Q_EMIT visibleChanged(true);

	m_display->scrollToBottom();

	QDialog::showEvent(event);
}

//-----------------------------------------------------------------------------

void DailyProgressDialog::modelReset()
{
	const int size = m_display->rowHeight(0);
	for (int r = 0, count = m_progress->rowCount(); r < count; ++r) {
		m_display->setRowHeight(r, size);
	}
	m_display->scrollToBottom();
}

//-----------------------------------------------------------------------------

void DailyProgressDialog::streaksChanged()
{
	QDate streak_start, streak_end;

	m_progress->findLongestStreak(streak_start, streak_end);
	m_longest_streak->setText(createStreakText(tr("Longest streak"), streak_start, streak_end));
	m_longest_streak->setEnabled(streak_end.isValid());

	m_progress->findCurrentStreak(streak_start, streak_end);
	m_current_streak->setText(createStreakText(tr("Current streak"), streak_start, streak_end));
	m_current_streak->setEnabled(streak_end == QDate::currentDate());
}

//-----------------------------------------------------------------------------

QString DailyProgressDialog::createStreakText(const QString& title, const QDate& start, const QDate& end) const
{
	const int length = start.isValid() ? (start.daysTo(end) + 1) : 0;
	QString start_str, end_str;
	if (length > 0) {
		const QLocale locale;
		start_str = locale.toString(start, QLocale::ShortFormat);
		end_str = locale.toString(end, QLocale::ShortFormat);
	} else {
		start_str = end_str = tr("N/A");
	}

	return QString("<center><b>%1</b><br><big>%2</big><br><small>%3</small></center>")
			.arg(title,
			tr("%n day(s)", "", length),
			tr("%1 &ndash; %2").arg(start_str, end_str));
}

//-----------------------------------------------------------------------------
