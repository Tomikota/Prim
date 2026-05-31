#include "MediaManager.h"
#include <QMediaFormat>
#include <QUrl>

MediaManager::MediaManager(QObject* parent) : QObject(parent) {
    m_audioSession.setAudioInput(&m_audioInput);
    m_audioSession.setRecorder(&m_audioRecorder);
    m_audioRecorder.setMediaFormat(QMediaFormat::FileFormat::Wave);
}

void MediaManager::startAudioRecording(const QString& path) {
    m_audioRecorder.setOutputLocation(QUrl::fromLocalFile(path));
    m_audioRecorder.record();
}

void MediaManager::stopAudioRecording() {
    m_audioRecorder.stop();
}