#ifndef ONYX_LIB_TTS_H_
#define ONYX_LIB_TTS_H_

#include "onyx/base/base.h"
#include "onyx/ui/ui.h"
#include "onyx/sound/sound.h"
#include "onyx/sound/async_player.h"
#include "tts_interface.h"

namespace tts
{

/// Define all tts states.
enum TTS_State
{
    TTS_INVALID = 0,
    TTS_PLAYING,
    TTS_PAUSED,
    TTS_STOPPED,
};

enum TTS_Valid
{
    TTS_DATA_INVALID = 0,
    TTS_PLUGIN_INVALID,
    TTS_VALID,
};
/// TTS engine container.
class TTS : public QObject
{
    Q_OBJECT
public:
    TTS(const QLocale & locale);
    ~TTS();

public:
    bool support();
    TTS_State state() { return state_; }
    TTS_Valid valid() { return valid_; }
    void setState(TTS_State state);
    void setValid(TTS_Valid valid);
    bool isPlaying() const { return state_ == TTS_PLAYING; }

    int span() { return span_; }
    void setSpan(const int s = 100) { span_ = s; }

    bool speakers(QStringList & list);
    bool currentSpeaker(QString & speaker);
    bool setSpeaker(const QString & speaker);

    bool speeds(QVector<int> & list);
    bool currentSpeed(int & speed);
    bool setSpeed(int speed);

    bool styles(QVector<int> & styles);
    bool currentStyle(int & style);
    bool setStyle(int style);

    Sound & sound();

public Q_SLOTS:
    bool speak(const QString & text);
    bool pause();
    bool resume();
    bool toggle();
    bool stop();
    void changeVolume(int, bool);

Q_SIGNALS:
    void speakDone();
    void TTSInitError();

private Q_SLOTS:
    void onSynthDone(bool ok, QByteArray &data);
    void onTimeout();
    void onPlayFinished(int);
private:
    bool loadPlugin();
    bool loadPreferPlugin(const QString & filePath);
    void init(const QLocale & locale);

private:
    scoped_ptr<Sound> sound_;
    TTS_State state_;
    TTS_Valid valid_;
    QByteArray data_;   ///< Cached data.
    scoped_ptr<TTSInterface> tts_impl_; ///< Backend instance.
    QTimer timer_;
    int span_;      ///< Serves as interval.
    int idle_count_;
};

}   // namespace tts

#endif  // ONYX_LIB_TTS_H_

