/***********************************************************************
 *
 * Copyright (C) 2010, 2014, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "timer.h"

#include "alert.h"
#include "alert_layer.h"
#include "document.h"
#include "deltas.h"
#include "stack.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QTimeEdit>
#include <QTimer>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

namespace
{
	QTime removeMSecs(const QTime& time)
	{
		return QTime(time.hour(), time.minute(), time.second());
	}

	QDateTime removeMSecs(const QDateTime& datetime)
	{
		return QDateTime(datetime.date(), removeMSecs(datetime.time()));
	}
}

//-----------------------------------------------------------------------------

Timer::Timer(Stack* documents, QWidget* parent)
	: QFrame(parent),
	m_type(0),
	m_started(false),
	m_finished(false),
	m_documents(documents)
{
	init();

	// Set default delay
	m_delay_edit->setTime(QTime(0, 15));
}

//-----------------------------------------------------------------------------

Timer::Timer(int type, const QStringList& values, Stack* documents, QWidget* parent)
	: QFrame(parent),
	m_type(type),
	m_started(false),
	m_finished(false),
	m_documents(documents)
{
	init();

	// Set default values
	QTime time = QTime::fromString(values.value(0), Qt::ISODate);
	m_type_box->setCurrentIndex(m_type);
	if (m_type == 0) {
		m_delay_edit->setTime(time);
	} else {
		m_end_edit->setTime(time);
	}
	m_memo_edit->setText(values.value(1));

	// Start timer
	editAccepted();
}

//-----------------------------------------------------------------------------

Timer::Timer(const QString& id, Stack* documents, QWidget* parent)
	: QFrame(parent),
	m_id(id),
	m_started(false),
	m_finished(false),
	m_documents(documents)
{
	init();

	QSettings settings;
	settings.beginGroup("Timers");

	// Load values
	QStringList values = settings.value(m_id).toStringList();
	m_type = values.value(0).toInt();
	QDateTime start = removeMSecs(QDateTime::fromString(values.value(1), Qt::ISODate));
	QDateTime end = removeMSecs(QDateTime::fromString(values.value(2), Qt::ISODate));
	QString memo = values.value(3);
	QDateTime current = QDateTime::currentDateTime();
	if (start.isNull() || end.isNull() || start > end || start > current || end < current || start.daysTo(end) > 1) {
		remove();
		return;
	}
	if (values.count() == 9) {
		m_character_count = values[4].toInt();
		m_character_and_space_count = values[5].toInt();
		m_page_count = values[6].toInt();
		m_paragraph_count = values[7].toInt();
		m_word_count = values[8].toInt();
	}

	// Set editor values
	m_end_edit->setTime(end.time());
	m_type_box->setCurrentIndex(m_type);
	m_memo_edit->setText(memo);

	// Start timer
	m_start = removeMSecs(start);
	startTimer();
}

//-----------------------------------------------------------------------------

Timer::~Timer()
{
	if (m_started && !m_finished) {
		save();
	}
	qDeleteAll(m_deltas);
	m_deltas.clear();
}

//-----------------------------------------------------------------------------

bool Timer::isEditing() const
{
	return m_edit->isVisible();
}

//-----------------------------------------------------------------------------

bool Timer::isRunning() const
{
	return m_timer->isActive();
}

//-----------------------------------------------------------------------------

QString Timer::memo() const
{
	return m_memo;
}

//-----------------------------------------------------------------------------

QString Timer::memoShort() const
{
	return m_memo_short;
}

//-----------------------------------------------------------------------------

int Timer::msecsFrom(const QDateTime& start) const
{
	if (start.date() == m_end.date()) {
		return start.time().msecsTo(m_end.time()) + 1000;
	} else {
		return start.time().msecsTo(QTime(23, 59, 59, 999)) + QTime(0, 0).msecsTo(m_end.time()) + 1001;
	}
}

//-----------------------------------------------------------------------------

int Timer::msecsTotal() const
{
	return m_delay_msecs;
}

//-----------------------------------------------------------------------------

void Timer::cancelEditing()
{
	if (isEditing()) {
		editRejected();
	}
}

//-----------------------------------------------------------------------------

void Timer::save()
{
	QSettings settings;
	settings.beginGroup("Timers");

	// Find ID
	if (m_id.isEmpty()) {
		int i = 1;
		forever {
			QString timer_id = QString("Timer%1").arg(i);
			if (settings.contains(timer_id)) {
				i++;
			} else {
				m_id = timer_id;
				break;
			}
		}
	}

	// Find stats
	updateCounts();

	// Write timer
	QStringList values;
	values.append(QString::number(m_type));
	values.append(m_start.toString(Qt::ISODate));
	values.append(m_end.toString(Qt::ISODate));
	values.append(m_memo);
	values.append(QString::number(m_character_count));
	values.append(QString::number(m_character_and_space_count));
	values.append(QString::number(m_page_count));
	values.append(QString::number(m_paragraph_count));
	values.append(QString::number(m_word_count));
	settings.setValue(m_id, values);
}

//-----------------------------------------------------------------------------

bool Timer::operator<=(const Timer& timer) const
{
	return m_end <= timer.m_end;
}

//-----------------------------------------------------------------------------

QString Timer::toString(const QString& time, const QString& memo)
{
	if (!memo.isEmpty()) {
		return tr("<b>%1</b> - %2").arg(time.simplified()).arg(memo);
	} else {
		return QLatin1String("<b>") + time.simplified() + QLatin1String("</b>");
	}
}

//-----------------------------------------------------------------------------

void Timer::delayChanged(const QTime& delay)
{
	QTime end = removeMSecs(QTime::currentTime()).addSecs(QTime(0,0,0).secsTo(delay));
	m_end_edit->blockSignals(true);
	m_end_edit->setTime(end);
	m_end_edit->blockSignals(false);
}

//-----------------------------------------------------------------------------

void Timer::endChanged(const QTime& end)
{
	QTime delay = QTime(0,0,0).addSecs(removeMSecs(QTime::currentTime()).secsTo(end));
	m_delay_edit->blockSignals(true);
	m_delay_edit->setTime(delay);
	m_delay_edit->blockSignals(false);
}

//-----------------------------------------------------------------------------

void Timer::editAccepted()
{
	m_type = m_type_box->currentIndex();
	if (!startTimer()) {
		return editRejected();
	}

	QSettings settings;
	settings.beginGroup("Timers");

	// Prepend values to recent list
	QString key = QString("Recent%1").arg(m_type);
	QStringList recent = settings.value(key).toStringList();
	QString defaults = ((m_type == 0) ? m_delay_edit : m_end_edit)->time().toString(Qt::ISODate)+ " " + m_memo;
	recent.removeAll(defaults);
	recent.prepend(defaults);
	while (recent.count() > 5) {
		recent.removeLast();
	}
	settings.setValue(key, recent);

	save();

	emit changed(this);
}

//-----------------------------------------------------------------------------

void Timer::editRejected()
{
	m_edit->hide();
	if (!m_started) {
		remove();
	} else if (m_end < QDateTime::currentDateTime()) {
		timerFinished();
	} else {
		setMode(false);
		m_type_box->setCurrentIndex(m_type);
		m_delay_edit->setTime(m_delay);
		m_end_edit->setTime(m_end.time());
		m_memo_edit->setText(m_memo);
	}
}

//-----------------------------------------------------------------------------

void Timer::editClicked()
{
	endChanged(m_end_edit->time());
	setMode(true);
	emit edited(this);
}

//-----------------------------------------------------------------------------

void Timer::removeClicked()
{
	if (QMessageBox::question(this, tr("Question"), tr("Delete timer?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
		remove();
	}
}

//-----------------------------------------------------------------------------

void Timer::timerFinished()
{
	if (!isEditing()) {
		updateCounts();
		qDeleteAll(m_deltas);
		m_deltas.clear();

		QStringList details;
		details << tr("<b>Words:</b> %L1").arg(m_word_count);
		details << tr("<b>Pages:</b> %L1").arg(m_page_count);
		details << tr("<b>Paragraphs:</b> %L1").arg(m_paragraph_count);
		details << tr("<b>Characters:</b> %L1 / %L2").arg(m_character_count).arg(m_character_and_space_count);

		remove();
		m_documents->alerts()->addAlert(new Alert(Alert::NoIcon, m_display_label->text(), details, true));
	}
}

//-----------------------------------------------------------------------------

void Timer::documentAdded(Document* document)
{
	m_deltas.insert(document, new Deltas(document));
}

//-----------------------------------------------------------------------------

void Timer::documentRemoved(Document* document)
{
	Deltas* delta = m_deltas.take(document);
	m_character_count += delta->characterCount();
	m_character_and_space_count += delta->characterAndSpaceCount();
	m_page_count += delta->pageCount();
	m_paragraph_count += delta->paragraphCount();
	m_word_count += delta->wordCount();
	delete delta;
}

//-----------------------------------------------------------------------------

void Timer::init()
{
	m_delay_msecs = 0;
	m_character_count = 0;
	m_character_and_space_count = 0;
	m_page_count = 0;
	m_paragraph_count = 0;
	m_word_count = 0;

	setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

	// Create edit widgets
	m_edit = new QWidget(this);

	m_type_box = new QComboBox(m_edit);
	m_type_box->addItem(tr("Set Delay"));
	m_type_box->addItem(tr("Set Time"));

	QStackedWidget* time_labels = new QStackedWidget(this);
	QLabel* label = new QLabel(tr("Delay:"), time_labels);
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	time_labels->addWidget(label);
	label = new QLabel(tr("Time:"), time_labels);
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	time_labels->addWidget(label);
	connect(m_type_box, QOverload<int>::of(&QComboBox::currentIndexChanged), time_labels, &QStackedWidget::setCurrentIndex);

	QStackedWidget* time_edits = new QStackedWidget(this);
	connect(m_type_box, QOverload<int>::of(&QComboBox::currentIndexChanged), time_edits, &QStackedWidget::setCurrentIndex);

	m_delay_edit = new QTimeEdit(time_edits);
	m_delay_edit->setDisplayFormat(tr("HH:mm:ss"));
	m_delay_edit->setCurrentSection(QDateTimeEdit::MinuteSection);
	m_delay_edit->setWrapping(true);
	time_edits->addWidget(m_delay_edit);
	connect(m_delay_edit, &QTimeEdit::timeChanged, this, &Timer::delayChanged);

	m_end_edit = new QTimeEdit(time_edits);
	m_end_edit->setDisplayFormat(QLocale().timeFormat(QLocale::LongFormat).contains("AP", Qt::CaseInsensitive) ? "h:mm:ss AP" : "HH:mm:ss");
	m_end_edit->setCurrentSection(QDateTimeEdit::MinuteSection);
	m_end_edit->setWrapping(true);
	time_edits->addWidget(m_end_edit);
	connect(m_end_edit, &QTimeEdit::timeChanged, this, &Timer::endChanged);

	m_memo_edit = new QLineEdit(tr("Alarm"), m_edit);
	m_memo_edit->setMaxLength(140);

	QDialogButtonBox* edit_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, m_edit);
	m_ok_button = edit_buttons->button(QDialogButtonBox::Ok);
	m_ok_button->setDefault(true);
	connect(edit_buttons, &QDialogButtonBox::accepted, this, &Timer::editAccepted);
	connect(edit_buttons, &QDialogButtonBox::rejected, this, &Timer::editRejected);

	// Lay out edit widgets
	QGridLayout* edit_layout = new QGridLayout(m_edit);
	edit_layout->setContentsMargins(0, 0, 0, 0);
	edit_layout->setColumnStretch(1, 1);
	edit_layout->addWidget(new QLabel(tr("Type:"), m_edit), 0, 0, Qt::AlignRight | Qt::AlignVCenter);
	edit_layout->addWidget(m_type_box, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
	edit_layout->addWidget(time_labels, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
	edit_layout->addWidget(time_edits, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
	edit_layout->addWidget(new QLabel(tr("Memo:"), m_edit), 2, 0, Qt::AlignRight | Qt::AlignVCenter);
	edit_layout->addWidget(m_memo_edit, 2, 1);
	edit_layout->addWidget(edit_buttons, 3, 0, 1, 2);

	// Create display widgets
	m_display = new QWidget(this);

	m_display_label = new QLabel(m_display);
	m_display_label->setWordWrap(true);

	QDialogButtonBox* display_buttons = new QDialogButtonBox(Qt::Horizontal, m_display);
	m_edit_button = display_buttons->addButton(tr("Edit"), QDialogButtonBox::AcceptRole);
	m_edit_button->setDefault(true);
	display_buttons->addButton(tr("Delete"), QDialogButtonBox::RejectRole);
	connect(display_buttons, &QDialogButtonBox::accepted, this, &Timer::editClicked);
	connect(display_buttons, &QDialogButtonBox::rejected, this, &Timer::removeClicked);

	// Lay out display widgets
	QVBoxLayout* display_layout = new QVBoxLayout(m_display);
	display_layout->setContentsMargins(0, 0, 0, 0);
	display_layout->addWidget(m_display_label);
	display_layout->addWidget(display_buttons);

	// Lay out window
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_edit);
	layout->addWidget(m_display);

	// Add timer
	m_timer = new QTimer(this);
	m_timer->setSingleShot(true);
	connect(m_timer, &QTimer::timeout, this, &Timer::timerFinished);

	// Show edit widgets by default
	setMode(true);
}

//-----------------------------------------------------------------------------

void Timer::remove()
{
	m_finished = true;
	if (!m_id.isEmpty()) {
		QSettings().remove("Timers/" + m_id);
	}
	if (m_timer->isActive()) {
		m_timer->stop();
	}
	deleteLater();
}

//-----------------------------------------------------------------------------

void Timer::setMode(bool edit)
{
	if (edit) {
		m_display->hide();
		m_edit->show();
		m_ok_button->setFocus();
	} else {
		m_edit->hide();
		m_display->show();
		m_edit_button->setFocus();
	}
}

//-----------------------------------------------------------------------------

bool Timer::startTimer()
{
	// Don't restart unchanged timers
	if (m_timer->isActive() && m_end.time() == m_end_edit->time()) {
		setMode(false);
		return false;
	}

	// Setup timer, making sure to ignore milliseconds
	m_delay = m_delay_edit->time();
	QDateTime start = removeMSecs(QDateTime::currentDateTime());
	if (!m_start.isValid()) {
		m_start = start;
	}
	if (m_type == 0) {
		m_end = start.addSecs(QTime(0,0,0).secsTo(m_delay));
	} else {
		m_end.setTime(m_end_edit->time());
		m_end.setDate((m_end.time() > start.time()) ? start.date() : start.date().addDays(1));
	}
	m_end_edit->setTime(m_end.time());
	m_timer->start((start.secsTo(m_end) * 1000) - QTime::currentTime().msec());

	// Show values
	m_memo = m_memo_edit->text().simplified();
	m_memo.truncate(140);
	m_memo_short = fontMetrics().elidedText(m_memo, Qt::ElideRight, 300);
	m_delay_msecs = m_start.secsTo(m_end) * 1000;
	m_display_label->setText(toString(m_end.time().toString(Qt::DefaultLocaleLongDate), m_memo));
	setMode(false);

	// Create document deltas
	if (m_deltas.isEmpty()) {
		int count = m_documents->count();
		for (int i = 0; i < count; ++i) {
			Document* document = m_documents->document(i);
			m_deltas.insert(document, new Deltas(document));
		}
	}

	m_started = true;
	return true;
}

//-----------------------------------------------------------------------------

void Timer::updateCounts()
{
	QList<Deltas*> deltas = m_deltas.values();
	for (Deltas* delta : deltas) {
		m_character_count += delta->characterCount();
		m_character_and_space_count += delta->characterAndSpaceCount();
		m_page_count += delta->pageCount();
		m_paragraph_count += delta->paragraphCount();
		m_word_count += delta->wordCount();
		delta->refresh();
	}
}

//-----------------------------------------------------------------------------
