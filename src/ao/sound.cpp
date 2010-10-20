/***********************************************************************
 *
 * Copyright (C) 2010 Graeme Gott <graeme@gottcode.org>
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

#include "sound.h"

#include <QtEndian>
#include <QFile>
#include <QThread>

//-----------------------------------------------------------------------------

QString Sound::m_path;

//-----------------------------------------------------------------------------

namespace
{
	int total_sounds = 0;

	struct Header
	{
		// RIFF chunk
		char riff_id[4];
		quint32 riff_size;
		char format[4];

		// format chunk
		char fmt_id[4];
		quint32 fmt_size;
		quint16 audio_format;
		quint16 num_channels;
		quint32 sample_rate;
		quint32 byte_rate;
		quint16 block_align;
		quint16 bits_per_sample;

		// data chunk
		char data_id[4];
		quint32 data_size;
	};

	class SoundThread : public QThread
	{
	public:
		SoundThread()
			: m_format(0)
		{
		}

		void play(ao_sample_format* format, const QByteArray& data)
		{
			m_format = format;
			m_data = data;
			start();
		}

	protected:
		virtual void run();

	private:
		ao_sample_format* m_format;
		QByteArray m_data;
	};

	void SoundThread::run()
	{
		int driver = ao_default_driver_id();
		if (driver == -1) {
			qWarning("Unable to fetch ID of default libao driver.");
			return;
		}

		ao_device* device = ao_open_live(driver, m_format, 0);
		if (device == 0) {
			qWarning("Unable to open default libao device.");
			return;
		}

		if (ao_play(device, m_data.data(), m_data.size()) == 0) {
			qWarning("Error while playing sound.");
		}

		if (ao_close(device) == 0) {
			qWarning("Error while closing default libao device.");
		}
	}

	QList<SoundThread*> sound_threads;
}

//-----------------------------------------------------------------------------

Sound::Sound(const QString& filename, QObject* parent)
	: QObject(parent)
{
	if (total_sounds == 0) {
		ao_initialize();
	}
	total_sounds++;

	QFile file(m_path + "/" + filename);
	if (!file.open(QFile::ReadOnly)) {
		qWarning("Unable to open audio file '%s'.", qPrintable(filename));
		return;
	}

	// Read header
	Header header;
	if (file.read(reinterpret_cast<char*>(&header), sizeof(Header)) != sizeof(Header)) {
		qWarning("Unable to read audio file header.");
		return;
	}

	if (memcmp(&header.riff_id, "RIFF", 4) == 0) {
		m_format.byte_format = AO_FMT_LITTLE;
	} else if (memcmp(&header.riff_id, "RIFX", 4) == 0) {
		m_format.byte_format = AO_FMT_BIG;
	} else {
		qWarning("Not a RIFF file.");
		return;
	}

	if ((memcmp(&header.format, "WAVE", 4) != 0) || (memcmp(&header.fmt_id, "fmt ", 4) != 0) || (header.audio_format != 1)) {
		qWarning("Not a supported WAVE file.");
		return;
	}

	m_format.bits = qFromLittleEndian<quint16>(header.bits_per_sample);
	m_format.rate = qFromLittleEndian<quint32>(header.sample_rate);
	m_format.channels = qFromLittleEndian<quint16>(header.num_channels);
	m_format.matrix = 0;

	// Read data
	m_data = file.read(header.data_size);
	file.close();
	if (static_cast<quint32>(m_data.size()) != header.data_size) {
		qWarning("Unable to read all WAVE data.");
		return;
	}
}

//-----------------------------------------------------------------------------

Sound::Sound(const Sound& sound)
	: QObject(sound.parent())
{
	total_sounds++;
	m_format = sound.m_format;
	m_data = sound.m_data;
}

//-----------------------------------------------------------------------------

Sound& Sound::operator=(const Sound& sound)
{
	total_sounds++;
	m_format = sound.m_format;
	m_data = sound.m_data;
	return *this;
}

//-----------------------------------------------------------------------------

Sound::~Sound()
{
	total_sounds--;
	if (total_sounds == 0) {
		foreach (SoundThread* sound_thread, sound_threads) {
			sound_thread->wait();
		}
		qDeleteAll(sound_threads);
		sound_threads.clear();
		ao_shutdown();
	}
}

//-----------------------------------------------------------------------------

void Sound::play()
{
	SoundThread* thread = 0;
	foreach (SoundThread* sound_thread, sound_threads) {
		if (sound_thread->isFinished()) {
			thread = sound_thread;
			break;
		}
	}
	if (thread == 0) {
		thread = new SoundThread;
		sound_threads.append(thread);
	}
	thread->play(&m_format, m_data);
}

//-----------------------------------------------------------------------------

void Sound::setPath(const QString& path)
{
	m_path = path;
}

//-----------------------------------------------------------------------------
