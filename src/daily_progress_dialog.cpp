/***********************************************************************
 *
 * Copyright (C) 2013, 2014, 2016, 2017 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include "daily_progress_dialog.h"

#include "daily_progress.h"
#include "preferences.h"

#include <QApplication>
#include <QColor>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
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
	Delegate(QObject* parent = 0) :
		QStyledItemDelegate(parent)
	{
	}

	void changeEvent(QEvent* event)
	{
		if ((event->type() == QEvent::PaletteChange) || (event->type() == QEvent::StyleChange)) {
			m_pixmap = QPixmap();
		}
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);

		if (opt.text.isEmpty()) {
			return;
		} else if ((index.column() > 0) && (index.column() < 8)) {
			opt.rect = opt.rect.adjusted(2,2,-2,-2);

			int progress = qBound(0, index.data(Qt::UserRole).toInt(), 100);
			if (progress == 0) {
				opt.backgroundBrush = opt.palette.alternateBase();
			} else if (progress == 100) {
#if (QT_VERSION >= QT_VERSION_CHECK(5,6,0))
				const qreal pixelratio = painter->device()->devicePixelRatioF();
#else
				const qreal pixelratio = painter->device()->devicePixelRatio();
#endif
				painter->drawPixmap(QPointF(opt.rect.topLeft()), fetchStarBackground(opt, pixelratio));
				opt.backgroundBrush = Qt::transparent;
				opt.font.setBold(true);
			} else {
				qreal k = (progress * 0.009) + 0.1;
				qreal ik = 1.0 - k;
				QColor base = opt.palette.color(QPalette::Active, QPalette::AlternateBase);
				QColor highlight = opt.palette.color(QPalette::Active, QPalette::Highlight);
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

		QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
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
			QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
			style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, &p, opt.widget);

			// Create star
			QPolygonF star;
			const qreal pi = acos(-1.0);
			for (int i = 0; i < 5; ++i) {
				qreal angle = ((i * 0.8) - 0.5) * pi;
				star << QPointF(cos(angle), sin(angle));
			}

			// Draw star
			p.setRenderHint(QPainter::Antialiasing);
			p.setPen(Qt::NoPen);
			QColor background = opt.palette.color(QPalette::Active, QPalette::HighlightedText);
			background.setAlpha(64);
			p.setBrush(background);
			qreal size = (opt.rect.width() * 0.5) - 2.0;
			qreal offset = 2.0 + size;
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

DailyProgressDialog::DailyProgressDialog(DailyProgress* progress, QWidget* parent) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
	m_progress(progress)
{
	setWindowTitle(tr("Daily Progress"));

	// Set up tableview
	m_display = new QTableView(this);
	m_display->setModel(progress);
	m_display->setShowGrid(false);
	m_display->verticalHeader()->hide();
	m_display->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_display->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	connect(progress, SIGNAL(modelReset()), this, SLOT(modelReset()));

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
	int frame = (m_display->style()->pixelMetric(QStyle::PM_DefaultFrameWidth) * 2) + 4;
	int min_width = m_display->fontMetrics().averageCharWidth() * 10;
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
	streaks_layout->setMargin(0);
	streaks_layout->addWidget(m_longest_streak);
	streaks_layout->addWidget(streak_divider);
	streaks_layout->addWidget(m_current_streak);

	connect(m_progress, SIGNAL(streaksChanged()), this, SLOT(streaksChanged()));
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
	emit visibleChanged(false);

	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------

void DailyProgressDialog::showEvent(QShowEvent* event)
{
	emit visibleChanged(true);

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

QString DailyProgressDialog::createStreakText(const QString& title, const QDate& start, const QDate& end)
{
	int length = start.isValid() ? (start.daysTo(end) + 1) : 0;
	QString start_str, end_str;
	if (length > 0) {
		start_str = start.toString(Qt::DefaultLocaleShortDate);
		end_str = end.toString(Qt::DefaultLocaleShortDate);
	} else {
		start_str = end_str = tr("N/A");
	}

	return QString("<center><b>%1</b><br><big>%2</big><br><small>%3</small></center>")
			.arg(title)
			.arg(tr("%n day(s)", "", length))
			.arg(tr("%1 &ndash; %2").arg(start_str).arg(end_str));
}

//-----------------------------------------------------------------------------
