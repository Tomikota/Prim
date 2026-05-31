#pragma once
#include <QMainWindow>
#include <QSqlQueryModel>
#include <QItemSelection>
#include <QMediaPlayer> 
#include <QAudioOutput>  
#include <QPushButton>
#include "DatabaseManager.h"
#include "NetworkManager.h"
#include "MessageModel.h"
#include "MediaManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PrimClass; }
QT_END_NAMESPACE

class Prim : public QMainWindow {
    Q_OBJECT

public:
    Prim(QWidget* parent = nullptr);
    ~Prim();
    void setUserName(const QString& name);

private slots:
    void onAddChatClicked();
    void onRenameChatClicked();
    void onDeleteChatClicked();
    void onChatSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void onSendMessageClicked();
    void onSendPhotoClicked();
    void onDataReceived(QString type, QString sender, QString textContent, QByteArray fileData, QString fileName, QString groupUuid);
    void onVoiceButtonClicked();
    void onCreateGroupClicked();
    void onToggleThemeClicked();

private:
    Ui::PrimClass* ui;
    NetworkManager* m_netManager;
    MediaManager* m_mediaManager;

    QSqlQueryModel* m_chatModel;
    MessageModel* m_messageModel;

    QString m_currentUserName;
    int m_activeChatId;
    QString m_activeGroupUuid;

    bool m_isRecordingAudio;

    bool m_isDarkTheme;
    QPushButton* m_btnToggleTheme;

    QMediaPlayer* m_audioPlayer;
    QAudioOutput* m_audioOutput;

    void loadChats();
    void loadMessages(int chatId);
    void loadGroupMessages(const QString& groupUuid);
    void setupStyles();
};