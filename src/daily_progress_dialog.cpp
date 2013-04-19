/***********************************************************************
 *
 * Copyright (C) 2013 Graeme Gott <graeme@gottcode.org>
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

#include <QApplication>
#include <QColor>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QSettings>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

namespace
{

class Delegate : public QStyledItemDelegate
{
public:
	Delegate(QObject* parent = 0) :
		QStyledItemDelegate(parent)
	{
		// Load colors
		QPalette palette = QApplication::palette();
		m_foregrounds[0] = palette.color(QPalette::Text);
		m_foregrounds[1] = palette.color(QPalette::HighlightedText);
		m_foregrounds[2] = palette.color(QPalette::HighlightedText);
		m_backgrounds[0] = palette.color(QPalette::AlternateBase);
		m_backgrounds[1] = palette.color(QPalette::Highlight);
		m_backgrounds[1].setAlpha(128);
		m_backgrounds[2] = palette.color(QPalette::Highlight);
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		QStyleOptionViewItemV4 opt = option;
		initStyleOption(&opt, index);

		if (opt.text.isEmpty()) {
			return;
		} else {
			opt.rect = opt.rect.adjusted(2,2,-2,-2);
			int progress = index.data(Qt::UserRole).toInt();
			if (progress <= 0) {
				opt.palette.setColor(QPalette::Text, m_foregrounds[0]);
				opt.backgroundBrush = m_backgrounds[0];
			} else if (progress < 100) {
				opt.palette.setColor(QPalette::Text, m_foregrounds[1]);
				opt.backgroundBrush = m_backgrounds[1];
			} else {
				opt.palette.setColor(QPalette::Text, m_foregrounds[2]);
				opt.backgroundBrush = m_backgrounds[2];
			}
		}

		QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
		style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
	}

private:
	QColor m_backgrounds[3];
	QColor m_foregrounds[3];
};

}

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
	m_display->setItemDelegate(new Delegate(this));
	m_display->verticalHeader()->hide();
	m_display->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_display->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	// Find maximum size needed to show day names
	m_display->horizontalHeader()->setMinimumSectionSize(0);
	int size = 0;
	for (int i = 1; i < 8; ++i) {
		size = qMax(size, m_display->horizontalHeader()->sectionSizeHint(i));
	}

	// Set rows to all be the same height
	for (int r = 0, count = m_progress->rowCount(); r < count; ++r) {
		m_display->setRowHeight(r, size);
	}

	// Make outside columns stretch and data columns fixed in size
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
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
#else
	m_display->horizontalHeader()->setClickable(false);
	m_display->horizontalHeader()->setMovable(false);
	m_display->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
	m_display->setColumnWidth(0, 0);
	for (int c = 1; c < 8; ++c) {
		m_display->horizontalHeader()->setResizeMode(c, QHeaderView::Fixed);
		m_display->setColumnWidth(c, size);
	}
	m_display->horizontalHeader()->setResizeMode(8, QHeaderView::Stretch);
	m_display->setColumnWidth(8, 0);
#endif

	// Set minimum size to always show up to 5 weeks of data
	int frame = (m_display->style()->pixelMetric(QStyle::PM_DefaultFrameWidth) * 2) + 4;
	m_display->setMinimumWidth(size * 7 + frame + m_display->verticalScrollBar()->sizeHint().width());
	m_display->setMinimumHeight((size * 5) + frame + m_display->horizontalHeader()->sizeHint().height());
	m_display->scrollToBottom();

	// Set up streak labels
	m_longest_streak = new QLabel(this);

	QFrame* streak_divider = new QFrame(this);
	streak_divider->setFrameStyle(QFrame::Sunken | QFrame::VLine);

	m_current_streak = new QLabel(this);

	QHBoxLayout* streaks_layout = new QHBoxLayout;
	streaks_layout->setMargin(0);
	streaks_layout->addWidget(m_longest_streak);
	streaks_layout->addWidget(streak_divider);
	streaks_layout->addWidget(m_current_streak);

	connect(m_progress, SIGNAL(streaksChanged()), this, SLOT(streaksChanged()));
	streaksChanged();

	// Lay out dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_display);
	layout->addLayout(streaks_layout);

	// Restore size
	resize(QSettings().value("DailyProgressDialog/Size", sizeHint()).toSize());
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

void DailyProgressDialog::streaksChanged()
{
	QDate streak_start, streak_end;

	m_progress->findLongestStreak(streak_start, streak_end);
	m_longest_streak->setText(createStreakText(tr("Longest streak"), streak_start, streak_end));

	m_progress->findCurrentStreak(streak_start, streak_end);
	m_current_streak->setText(createStreakText(tr("Current streak"), streak_start, streak_end));
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
