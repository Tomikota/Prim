#include "prim.h"
#include "ui_prim.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QDialog>
#include <QMediaPlayer> 
#include <QAudioOutput>  
#include <QUrl>          
#include <QFileDialog> 
#include <QFileInfo>   
#include <QLabel>      
#include <QVBoxLayout> 
#include <QHBoxLayout>
#include <QGridLayout>
#include <QUuid>
#include <QTimer>

namespace {
    struct LayoutSearcher {
        static QLayout* find(QLayout* layout, QWidget* w, int& idx) {
            if (!layout) return nullptr;
            for (int i = 0; i < layout->count(); ++i) {
                QLayoutItem* item = layout->itemAt(i);
                if (item->widget() == w) {
                    idx = i;
                    return layout;
                }
                if (item->layout()) {
                    QLayout* found = find(item->layout(), w, idx);
                    if (found) return found;
                }
            }
            return nullptr;
        }
    };
}

Prim::Prim(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::PrimClass), m_netManager(new NetworkManager(this)),
    m_mediaManager(new MediaManager(this)), m_messageModel(new MessageModel(this)),
    m_activeChatId(-1), m_activeGroupUuid(""), m_isRecordingAudio(false),
    m_isDarkTheme(true), m_btnToggleTheme(nullptr) {

    ui->setupUi(this);
    setupStyles();

    if (ui->btnVideo) {
        ui->btnVideo->hide();
    }

    if (ui->chatListView && ui->chatListView->parentWidget()) {
        QWidget* parentWidget = ui->chatListView->parentWidget();
        int index = -1;

        QLayout* targetLayout = LayoutSearcher::find(parentWidget->layout(), ui->chatListView, index);

        if (targetLayout) {
            QBoxLayout* boxLayout = qobject_cast<QBoxLayout*>(targetLayout);
            if (boxLayout && index != -1) {
                QWidget* topBar = new QWidget(parentWidget);
                QHBoxLayout* topLayout = new QHBoxLayout(topBar);
                topLayout->setContentsMargins(0, 0, 0, 4);
                topLayout->setSpacing(6);

                QPushButton* btnCreateGroup = new QPushButton("Группа+", topBar);
                btnCreateGroup->setToolTip("Создать групповой чат");

                QPushButton* btnToggleTheme = new QPushButton("Тема ☀️", topBar);
                btnToggleTheme->setToolTip("Сменить тему оформления");
                btnToggleTheme->setObjectName("btnToggleTheme");
                m_btnToggleTheme = btnToggleTheme;

                topLayout->addWidget(btnCreateGroup);
                topLayout->addWidget(btnToggleTheme);

                boxLayout->insertWidget(index, topBar);

                connect(btnCreateGroup, &QPushButton::clicked, this, &Prim::onCreateGroupClicked);
                connect(btnToggleTheme, &QPushButton::clicked, this, &Prim::onToggleThemeClicked);
            }
        }
    }

    m_audioPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioPlayer->setAudioOutput(m_audioOutput);

    m_chatModel = new QSqlQueryModel(this);
    ui->chatListView->setModel(m_chatModel);
    loadChats();

    ui->messageListView->setModel(m_messageModel);
    ui->messageListView->setItemDelegate(new MessageDelegate(this));

    m_netManager->startServer(12345);

    connect(ui->btnAddChat, &QPushButton::clicked, this, &Prim::onAddChatClicked);
    connect(ui->btnRenameChat, &QPushButton::clicked, this, &Prim::onRenameChatClicked);
    connect(ui->btnDeleteChat, &QPushButton::clicked, this, &Prim::onDeleteChatClicked);
    connect(ui->chatListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Prim::onChatSelectionChanged);

    connect(ui->btnSend, &QPushButton::clicked, this, &Prim::onSendMessageClicked);
    connect(ui->messageEdit, &QLineEdit::returnPressed, this, &Prim::onSendMessageClicked);

    connect(ui->btnPhoto, &QPushButton::clicked, this, &Prim::onSendPhotoClicked);
    connect(ui->btnVoice, &QPushButton::clicked, this, &Prim::onVoiceButtonClicked);

    connect(ui->messageListView, &QListView::clicked, [this](const QModelIndex& index) {
        QString type = index.data(MessageModel::TypeRole).toString();
        QString filePath = index.data(MessageModel::FilePathRole).toString();

        if (type == "voice") {
            if (m_messageModel->playingIndex() == index.row() && m_audioPlayer->playbackState() == QMediaPlayer::PlayingState) {
                m_audioPlayer->stop();
                m_messageModel->setPlayingIndex(-1);
            }
            else {
                m_messageModel->setPlayingIndex(index.row());
                QUrl url = QUrl::fromLocalFile(filePath);

                if (m_audioPlayer->source() == url) {
                    m_audioPlayer->setPosition(0);
                }
                else {
                    m_audioPlayer->setSource(url);
                }
                m_audioPlayer->play();
            }
        }
        else if (type == "image") {
            QDialog* viewer = new QDialog(this);
            viewer->setWindowTitle("Просмотр фотографии");
            QVBoxLayout* vLayout = new QVBoxLayout(viewer);
            QLabel* imgLabel = new QLabel(viewer);
            QPixmap pix(filePath);

            imgLabel->setPixmap(pix.scaled(600, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            vLayout->addWidget(imgLabel);

            viewer->setLayout(vLayout);
            viewer->exec();
            viewer->deleteLater();
        }
        });

    connect(m_audioPlayer, &QMediaPlayer::mediaStatusChanged, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            m_messageModel->setPlayingIndex(-1);
        }
        });

    connect(m_netManager, &NetworkManager::peerDiscovered, this, &Prim::loadChats);
    connect(m_netManager, &NetworkManager::dataReceived, this, &Prim::onDataReceived);
}

Prim::~Prim() {
    delete ui;
}

void Prim::setUserName(const QString& name) {
    m_currentUserName = name;
    this->setWindowTitle("Prim — " + m_currentUserName);
    m_netManager->setMyPhoneNumber(m_currentUserName);
}

void Prim::setupStyles() {
    if (m_isDarkTheme) {
        this->setStyleSheet(
            "QMainWindow { background-color: #0e1621; }"
            "QListView#chatListView { "
            "  background-color: #17212b; "
            "  border: none; "
            "  color: #ffffff; "
            "  font-size: 14px; "
            "  padding: 8px; "
            "}"
            "QListView#chatListView::item { "
            "  padding: 12px 14px; "
            "  margin-bottom: 4px; "
            "  border-radius: 8px; "
            "  color: #e2e8f0; "
            "}"
            "QListView#chatListView::item:hover { "
            "  background-color: #202b36; "
            "}"
            "QListView#chatListView::item:selected { "
            "  background-color: #2b5278; "
            "  color: #ffffff; "
            "  font-weight: bold; "
            "}"
            "QListView#messageListView { "
            "  background-color: #0e1621; "
            "  border: none; "
            "  padding: 10px; "
            "}"
            "QLineEdit { "
            "  background-color: #17212b; "
            "  border: 1px solid #24303f; "
            "  color: #ffffff; "
            "  border-radius: 8px; "
            "  padding: 10px 14px; "
            "  font-size: 13px; "
            "}"
            "QLineEdit:focus { "
            "  border: 1px solid #5288c1; "
            "}"
            "QPushButton { "
            "  background-color: #24303f; "
            "  color: #5288c1; "
            "  border: none; "
            "  border-radius: 8px; "
            "  padding: 10px 16px; "
            "  font-weight: bold; "
            "  font-size: 13px; "
            "}"
            "QPushButton:hover { "
            "  background-color: #2b394a; "
            "  color: #659bdf; "
            "}"
            "QPushButton:pressed { "
            "  background-color: #1e2834; "
            "}"
            "QScrollBar:vertical { "
            "  border: none; "
            "  background: #0e1621; "
            "  width: 6px; "
            "  margin: 0px; "
            "}"
            "QScrollBar::handle:vertical { "
            "  background: #2b394a; "
            "  min-height: 20px; "
            "  border-radius: 3px; "
            "}"
            "QScrollBar::handle:vertical:hover { "
            "  background: #3a4b5f; "
            "}"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
            "  height: 0px; "
            "}"
        );
    }
    else {
        this->setStyleSheet(
            "QMainWindow { background-color: #f9f8f6; }"
            "QListView#chatListView { "
            "  background-color: #ffffff; "
            "  border: none; "
            "  color: #3c3d3a; "
            "  font-size: 14px; "
            "  padding: 8px; "
            "  border-right: 1px solid #e5e5e0; "
            "}"
            "QListView#chatListView::item { "
            "  padding: 12px 14px; "
            "  margin-bottom: 4px; "
            "  border-radius: 8px; "
            "  color: #555550; "
            "}"
            "QListView#chatListView::item:hover { "
            "  background-color: #f2f1ed; "
            "}"
            "QListView#chatListView::item:selected { "
            "  background-color: #7c9a7b; "
            "  color: #ffffff; "
            "  font-weight: bold; "
            "}"
            "QListView#messageListView { "
            "  background-color: #f9f8f6; "
            "  border: none; "
            "  padding: 10px; "
            "}"
            "QLineEdit { "
            "  background-color: #ffffff; "
            "  border: 1px solid #dcdbd5; "
            "  color: #3c3d3a; "
            "  border-radius: 8px; "
            "  padding: 10px 14px; "
            "  font-size: 13px; "
            "}"
            "QLineEdit:focus { "
            "  border: 1px solid #7c9a7b; "
            "}"
            "QPushButton { "
            "  background-color: #f0efe9; "
            "  color: #7c9a7b; "
            "  border: none; "
            "  border-radius: 8px; "
            "  padding: 10px 16px; "
            "  font-weight: bold; "
            "  font-size: 13px; "
            "}"
            "QPushButton:hover { "
            "  background-color: #e5e4de; "
            "  color: #6a8569; "
            "}"
            "QPushButton:pressed { "
            "  background-color: #dad9d2; "
            "}"
            "QScrollBar:vertical { "
            "  border: none; "
            "  background: #f9f8f6; "
            "  width: 6px; "
            "  margin: 0px; "
            "}"
            "QScrollBar::handle:vertical { "
            "  background: #e5e4de; "
            "  min-height: 20px; "
            "  border-radius: 3px; "
            "}"
            "QScrollBar::handle:vertical:hover { "
            "  background: #dad9d2; "
            "}"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
            "  height: 0px; "
            "}"
        );
    }
}

void Prim::onToggleThemeClicked() {
    m_isDarkTheme = !m_isDarkTheme;
    MessageDelegate::isDarkTheme = m_isDarkTheme;

    if (m_isDarkTheme) {
        m_btnToggleTheme->setText("Тема ☀️");
    }
    else {
        m_btnToggleTheme->setText("Тема 🌙");
    }

    setupStyles();
    ui->messageListView->viewport()->update();
    loadChats();
}

void Prim::loadChats() {
    QSqlQuery query(
        "SELECT name, id FROM chats "
        "UNION "
        "SELECT '[Группа] ' || name AS name, -id AS id FROM groups"
    );
    m_chatModel->setQuery(std::move(query));
}

void Prim::onAddChatClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, "Добавить чат", "Имя собеседника:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    QString phone = QInputDialog::getText(this, "Добавить чат", "Номер телефона собеседника:", QLineEdit::Normal, "+7", &ok);
    if (!ok || phone.isEmpty()) return;

    DatabaseManager::instance().addChat(name, phone);
    loadChats();
}

void Prim::onCreateGroupClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, "Создать группу", "Название группы:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    QString membersStr = QInputDialog::getText(this, "Добавить участников",
        "Введите телефонные номера участников через запятую (включая +7):\nПример: +79991112233, +79995556677",
        QLineEdit::Normal, "", &ok);
    if (!ok || membersStr.isEmpty()) return;

    QStringList members = membersStr.split(",");
    for (QString& m : members) {
        m = m.trimmed();
    }

    if (!members.contains(m_currentUserName)) {
        members.append(m_currentUserName);
    }

    QString groupUuid = QUuid::createUuid().toString();

    DatabaseManager::instance().createGroup(name, groupUuid, members);
    m_netManager->sendGroupData(members, "create_group", m_currentUserName, name, QByteArray(), members.join(","), groupUuid);
    loadChats();
    QMessageBox::information(this, "Успех", "Группа \"" + name + "\" успешно создана!");
}

void Prim::onRenameChatClicked() {
    QModelIndex index = ui->chatListView->currentIndex();
    if (!index.isValid()) return;

    int selectedId = m_chatModel->data(m_chatModel->index(index.row(), 1)).toInt();
    if (selectedId < 0) {
        QMessageBox::warning(this, "Внимание", "Переименование групп пока не поддерживается.");
        return;
    }

    QString currentName = m_chatModel->data(m_chatModel->index(index.row(), 0)).toString();

    bool ok;
    QString newName = QInputDialog::getText(this, "Переименовать контакт", "Новое имя собеседника:", QLineEdit::Normal, currentName, &ok);
    if (ok && !newName.isEmpty()) {
        DatabaseManager::instance().renameChat(selectedId, newName);
        loadChats();
    }
}

void Prim::onDeleteChatClicked() {
    QModelIndex index = ui->chatListView->currentIndex();
    if (!index.isValid()) return;
    int selectedId = m_chatModel->data(m_chatModel->index(index.row(), 1)).toInt();

    if (selectedId < 0) {
        QSqlQuery q;
        int groupId = -selectedId;
        q.prepare("DELETE FROM groups WHERE id = :id");
        q.bindValue(":id", groupId);
        q.exec();
        q.prepare("DELETE FROM group_members WHERE group_id = :id");
        q.bindValue(":id", groupId);
        q.exec();
    }
    else {
        DatabaseManager::instance().deleteChat(selectedId);
    }

    loadChats();
    m_messageModel->clear();
    m_activeChatId = -1;
    m_activeGroupUuid = "";
}

void Prim::onChatSelectionChanged(const QItemSelection& selected, const QItemSelection&) {
    if (selected.isEmpty()) return;
    QModelIndex index = selected.indexes().first();
    int selectedId = m_chatModel->data(m_chatModel->index(index.row(), 1)).toInt();

    if (selectedId > 0) {
        m_activeChatId = selectedId;
        m_activeGroupUuid = "";

        QSqlQuery q;
        q.prepare("SELECT phone_number FROM chats WHERE id = :id");
        q.bindValue(":id", m_activeChatId);
        if (q.exec() && q.next()) {
            QString peerPhone = q.value(0).toString();
            m_netManager->connectToPhone(peerPhone);
        }

        loadMessages(m_activeChatId);
    }
    else {
        m_activeChatId = -1;
        int groupId = -selectedId;
        m_activeGroupUuid = DatabaseManager::instance().getGroupUuid(groupId);
        loadGroupMessages(m_activeGroupUuid);
    }
}

void Prim::loadMessages(int chatId) {
    m_messageModel->clear();
    auto msgs = DatabaseManager::instance().getMessages(chatId);
    QList<Message> modelMsgs;
    for (const auto& m : msgs) {
        Message msg;
        msg.sender = m.sender;
        msg.text = m.content;
        msg.type = m.type;
        msg.filePath = m.content;
        msg.timestamp = m.timestamp;
        msg.isOutbox = m.isOutbox;
        modelMsgs.append(msg);
    }
    m_messageModel->setMessages(modelMsgs);

    QTimer::singleShot(10, [this]() {
        ui->messageListView->scrollToBottom();
        });
}

void Prim::loadGroupMessages(const QString& groupUuid) {
    m_messageModel->clear();
    auto msgs = DatabaseManager::instance().getGroupMessages(groupUuid);
    QList<Message> modelMsgs;
    for (const auto& m : msgs) {
        Message msg;
        msg.sender = m.sender;
        msg.text = m.content;
        msg.type = m.type;
        msg.filePath = m.content;
        msg.timestamp = m.timestamp;
        msg.isOutbox = m.isOutbox;
        modelMsgs.append(msg);
    }
    m_messageModel->setMessages(modelMsgs);

    QTimer::singleShot(10, [this]() {
        ui->messageListView->scrollToBottom();
        });
}

void Prim::onSendMessageClicked() {
    QString text = ui->messageEdit->text();
    if (text.isEmpty()) return;

    if (m_activeChatId != -1) {
        m_netManager->sendData("text", m_currentUserName, text);
        DatabaseManager::instance().saveMessage(m_activeChatId, m_currentUserName, "text", text, true);
        loadMessages(m_activeChatId);
    }
    else if (!m_activeGroupUuid.isEmpty()) {
        QStringList members = DatabaseManager::instance().getGroupMembers(m_activeGroupUuid);
        m_netManager->sendGroupData(members, "text", m_currentUserName, text, QByteArray(), "", m_activeGroupUuid);
        DatabaseManager::instance().saveGroupMessage(m_activeGroupUuid, m_currentUserName, "text", text, true);
        loadGroupMessages(m_activeGroupUuid);
    }

    ui->messageEdit->clear();
}

void Prim::onSendPhotoClicked() {
    if (m_activeChatId == -1 && m_activeGroupUuid.isEmpty()) return;

    QString filePath = QFileDialog::getOpenFileName(this, "Выбрать фотографию", "", "Изображения (*.png *.jpg *.jpeg *.bmp *.gif)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QFileInfo fileInfo(filePath);
        if (m_activeChatId != -1) {
            m_netManager->sendData("image", m_currentUserName, "Изображение", data, fileInfo.fileName());
            DatabaseManager::instance().saveMessage(m_activeChatId, m_currentUserName, "image", filePath, true);
            loadMessages(m_activeChatId);
        }
        else if (!m_activeGroupUuid.isEmpty()) {
            QStringList members = DatabaseManager::instance().getGroupMembers(m_activeGroupUuid);
            m_netManager->sendGroupData(members, "image", m_currentUserName, "Изображение", data, fileInfo.fileName(), m_activeGroupUuid);
            DatabaseManager::instance().saveGroupMessage(m_activeGroupUuid, m_currentUserName, "image", filePath, true);
            loadGroupMessages(m_activeGroupUuid);
        }
    }
}

void Prim::onVoiceButtonClicked() {
    if (m_activeChatId == -1 && m_activeGroupUuid.isEmpty()) return;

    QDir().mkdir("temp_records");
    QString fileName = "voice_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".wav";
    QString path = QDir::currentPath() + "/temp_records/" + fileName;

    static QString currentRecordingPath;

    if (!m_isRecordingAudio) {
        currentRecordingPath = path;
        m_mediaManager->startAudioRecording(currentRecordingPath);
        ui->btnVoice->setText("🛑 Стоп");
        m_isRecordingAudio = true;
    }
    else {
        m_mediaManager->stopAudioRecording();
        ui->btnVoice->setText("🎤 Голос");
        m_isRecordingAudio = false;

        QFile file(currentRecordingPath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();

            if (m_activeChatId != -1) {
                m_netManager->sendData("voice", m_currentUserName, "Голосовое сообщение", data, "voice.wav");
                DatabaseManager::instance().saveMessage(m_activeChatId, m_currentUserName, "voice", currentRecordingPath, true);
                loadMessages(m_activeChatId);
            }
            else if (!m_activeGroupUuid.isEmpty()) {
                QStringList members = DatabaseManager::instance().getGroupMembers(m_activeGroupUuid);
                m_netManager->sendGroupData(members, "voice", m_currentUserName, "Голосовое сообщение", data, "voice.wav", m_activeGroupUuid);
                DatabaseManager::instance().saveGroupMessage(m_activeGroupUuid, m_currentUserName, "voice", currentRecordingPath, true);
                loadGroupMessages(m_activeGroupUuid);
            }
        }
    }
}

void Prim::onDataReceived(QString type, QString sender, QString textContent, QByteArray fileData, QString fileName, QString groupUuid) {
    if (type == "create_group") {
        QStringList members = fileName.split(",");
        DatabaseManager::instance().createGroup(textContent, groupUuid, members);
        loadChats();
        return;
    }

    if (!groupUuid.isEmpty()) {
        if (type == "text") {
            DatabaseManager::instance().saveGroupMessage(groupUuid, sender, type, textContent, false);
        }
        else {
            QDir().mkdir("downloads");
            QString path = QDir::currentPath() + "/downloads/" + QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + fileName;
            QFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(fileData);
                file.close();
            }
            DatabaseManager::instance().saveGroupMessage(groupUuid, sender, type, path, false);
        }

        if (!m_activeGroupUuid.isEmpty() && m_activeGroupUuid == groupUuid) {
            loadGroupMessages(m_activeGroupUuid);
        }
    }
    else {
        if (m_activeChatId == -1) return;
        if (type == "text") {
            DatabaseManager::instance().saveMessage(m_activeChatId, sender, type, textContent, false);
        }
        else {
            QDir().mkdir("downloads");
            QString path = QDir::currentPath() + "/downloads/" + QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + fileName;
            QFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(fileData);
                file.close();
            }
            DatabaseManager::instance().saveMessage(m_activeChatId, sender, type, path, false);
        }
        loadMessages(m_activeChatId);
    }
}