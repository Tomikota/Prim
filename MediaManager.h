#pragma once
#include <QObject>
#include <QMediaCaptureSession>
#include <QAudioInput>
#include <QMediaRecorder>

class MediaManager : public QObject {
    Q_OBJECT
public:
    explicit MediaManager(QObject* parent = nullptr);

    void startAudioRecording(const QString& path);
    void stopAudioRecording();

private:
    QMediaCaptureSession m_audioSession;
    QAudioInput m_audioInput;
    QMediaRecorder m_audioRecorder;
};