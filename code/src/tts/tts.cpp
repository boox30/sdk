
#include "onyx/tts/tts.h"
#include "onyx/tts/tts_interface.h"
//#include "espeak/tts_espeak.h"
//#include "aisound/tts_aisound.h"
#include "QDebug"
namespace tts
{
static const int BPS = 16;
static const int SAMPLE_RATE = 44100;
static const int CHANNELS = 1;

TTS::TTS(const QLocale & locale)
: span_(3000)
, tts_impl_(0)
, idle_count_(0)
{
    init(locale); //Loading TTS plugins & init;
}

TTS::~TTS()
{
}

void TTS::init(const QLocale & locale)
{
    // load preferred tts plugin first.
    QString plugin = qgetenv("TTS_PREFERRED_PLUGIN");
    if (!plugin.isEmpty())
    {
        if(loadPreferPlugin(plugin))
        {
            if (tts_impl_->initialize(locale, sound()))
            {
                connect(tts_impl_.get(), SIGNAL(synthDone(bool, QByteArray &)),
                    this, SLOT(onSynthDone(bool, QByteArray &)));
                connect(&AsyncPlayer::instance(), SIGNAL(playFinished(int)), this, SLOT(onPlayFinished(int)));
                connect(&timer_, SIGNAL(timeout()), this, SLOT(onTimeout()));

                setState(TTS_STOPPED);
                setValid(TTS_VALID);
                return;
            }
        }
        tts_impl_.release();
    }

    if (loadPlugin())
    {
        if (tts_impl_->initialize(locale, sound()))
        {
            connect(tts_impl_.get(), SIGNAL(synthDone(bool, QByteArray &)),
                    this, SLOT(onSynthDone(bool, QByteArray &)));
            connect(&AsyncPlayer::instance(), SIGNAL(playFinished(int)), this, SLOT(onPlayFinished(int)));
            connect(&timer_, SIGNAL(timeout()), this, SLOT(onTimeout()));

            setState(TTS_STOPPED);
            setValid(TTS_VALID);
        }
        else
        {
            setValid(TTS_DATA_INVALID);
        }
    }
    else
    {
        setValid(TTS_PLUGIN_INVALID);
    }
}

/// Synthesis the text to speech. The data will be ready later.
bool TTS::speak(const QString & text)
{
    setState(TTS_PLAYING);
    if (tts_impl_)
    {
        return tts_impl_->synthText(text);
    }
    return false;
}

bool TTS::pause()
{
    setState(TTS_PAUSED);
    return true;
}

bool TTS::resume()
{
    setState(TTS_PLAYING);
    return true;
}

bool TTS::toggle()
{
    if (state() == TTS_PLAYING)
    {
        setState(TTS_PAUSED);
    }
    else if (state() == TTS_PAUSED)
    {
        setState(TTS_PLAYING);
    }
    return true;
}

bool TTS::stop()
{
    setState(TTS_STOPPED);
    data_.clear();
    return true;
}

void TTS::changeVolume(int v, bool m)
{
    sound().setVolume(v);
}

/// Segment of sentence has been finished. But it does not mean that
/// the whole input message has been finished.
void TTS::onSynthDone(bool ok, QByteArray &data)
{
    qDebug("TTS::onSynthDone data length %d", data.length());
    QApplication::processEvents();

    // If stopped, just ignore the chunk.
    if (state() == TTS_STOPPED)
    {
        data_.clear();
        return;
    }

    data_.append(data);

    // Send the data to async player. Before that, it's necessary to
    // check the state.
    if (state() == TTS_PLAYING)
    {
        onTimeout();
        // timer_.stop();
        // timer_.start(span_);
    }
}

void TTS::onTimeout()
{
    /* Dump to file.
    QFile file("dump.tts");
    if (file.open(QIODevice::Append))
    {
        file.write(data_);
        file.flush();
        file.close();
    }
    */
    qDebug("Send to play.");
    AsyncPlayer::instance().play(sound(), data_);
    data_.clear();
}

void TTS::setValid(TTS_Valid valid)
{
    valid_ = valid;
    emit TTSInitError();
}

void TTS::setState(TTS_State state)
{
    state_ = state;
    sound().enable(state == TTS_PLAYING);

    if (state == TTS_STOPPED)
    {
        tts_impl_->stop();
    }

    if (state == TTS_PAUSED)
    {
        tts_impl_->pause();
    }

    if (state == TTS_STOPPED || state == TTS_PAUSED)
    {
        AsyncPlayer::instance().waitForDone();
        sound_.reset(0);
    }

    if (state == TTS_PLAYING)
    {
        if (idle_count_ <= 0)
        {
            sys::SysStatus::instance().enableIdle(false);
            ++idle_count_;
        }
    }
    else
    {
        if (idle_count_ > 0)
        {
            sys::SysStatus::instance().enableIdle(true);
        }
        idle_count_ = 0;
    }
}

void TTS::onPlayFinished(int)
{
    if (isPlaying())
    {
        emit speakDone();
    }
}

/// Check if we have any tts engine installed.
/// Return false if there is no tts backend enabled. Otherwise returns true.
bool TTS::support()
{
    return true;
}

bool TTS::speakers(QStringList & list)
{
    if (tts_impl_)
    {
        return tts_impl_->speakers(list);
    }
    return false;
}

bool TTS::currentSpeaker(QString & speaker)
{
    if (tts_impl_)
    {
        return tts_impl_->currentSpeaker(speaker);
    }
    return false;
}

bool TTS::setSpeaker(const QString & speaker)
{
    if (tts_impl_)
    {
        return tts_impl_->setSpeaker(speaker);
    }
    return false;
}

bool TTS::speeds(QVector<int> & list)
{
    if (tts_impl_)
    {
        return tts_impl_->speeds(list);
    }
    return false;
}

bool TTS::currentSpeed(int & speed)
{
    if (tts_impl_)
    {
        return tts_impl_->currentSpeed(speed);
    }
    return false;
}


bool TTS::setSpeed(int speed)
{
    if (tts_impl_)
    {
        return tts_impl_->setSpeed(speed);
    }
    return false;
}

bool TTS::styles(QVector<int> & styles)
{
    if (tts_impl_)
    {
        return tts_impl_->styles(styles);
    }
    return false;
}

bool TTS::currentStyle(int & style)
{
    if (tts_impl_)
    {
        return tts_impl_->currentStyle(style);
    }
    return false;
}

bool TTS::setStyle(int style)
{
    if (tts_impl_)
    {
        return tts_impl_->setStyle(style);
    }
    return false;
}

Sound & TTS::sound()
{
    if (!sound_)
    {
        bool open = (qgetenv("DISABLE_TTS_SOUND_DEV").toInt() <= 0);
        qDebug("open sound device or not %d", open);
        sound_.reset(new Sound(open));
        if (open)
        {
            sound_->setBitsPerSample(BPS);
            sound_->setChannels(CHANNELS);
            sound_->setSamplingRate(SAMPLE_RATE);
            sound_->setVolume(sys::SysStatus::instance().volume());
        }
    }
    return *sound_;
}

bool TTS::loadPlugin()
{
    QDir dir("/usr/share/tts/plugins");
    QDir::Filters filters = QDir::Files|QDir::NoDotAndDotDot;
    QFileInfoList all = dir.entryInfoList(filters);
    for(QFileInfoList::iterator iter = all.begin(); iter != all.end(); ++iter)
    {
        if (iter->isFile())
        {
            QPluginLoader pluginLoader(iter->absoluteFilePath(), this);
            qDebug() << "TTS begins to create a plugin instance " << iter->absoluteFilePath();
            pluginLoader.load();
            if (!pluginLoader.isLoaded())
            {
                qDebug() << "Could not load tts plugin: " << iter->absoluteFilePath();
                continue;
            }
            QObject *plugin = pluginLoader.instance();
            if (plugin)
            {
                qDebug() << "TTS gets plugin, is to reset tts_impl_";
                tts_impl_.reset(qobject_cast<TTSInterface *>(plugin));
                if (tts_impl_)
                {
                    return true;
                }
            }
        }
    }
    return false;
}


bool TTS::loadPreferPlugin(const QString & filePath)
{
    QPluginLoader pluginLoader(filePath, this);
    qDebug() << "TTS begins to create a plugin instance " <<filePath;
    pluginLoader.load();
    if (!pluginLoader.isLoaded())
    {
        qDebug() << "Could not load tts plugin: ";
        return false;
    }
    QObject *plugin = pluginLoader.instance();
    if (plugin)
    {
        qDebug() << "TTS gets plugin, is to reset tts_impl_";
        tts_impl_.release();
        qDebug()<<"OK "<<__LINE__;
        tts_impl_.reset(qobject_cast<TTSInterface *>(plugin));
        if (tts_impl_)
        {
            qDebug()<<"All seens OK.";
            return true;
        }
    }
    qDebug()<<"Return OK";
    return false;
}
}
