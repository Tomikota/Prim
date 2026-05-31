#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QStringList>

struct ChatInfo {
    int id;
    QString name;
    QString phoneNumber;
    QString ip;
    int port;
};

struct MessageInfo {
    QString sender;
    QString type;
    QString content;
    QDateTime timestamp;
    bool isOutbox;
};

class DatabaseManager {
public:
    static DatabaseManager& instance() {
        static DatabaseManager inst;
        return inst;
    }

    bool init();
    bool registerUser(const QString& phoneNumber, const QString& password);
    bool loginUser(const QString& phoneNumber, const QString& password);

    bool addChat(const QString& name, const QString& phoneNumber);
    bool deleteChat(int chatId);
    bool renameChat(int chatId, const QString& newName);
    bool updateChatAddress(const QString& phoneNumber, const QString& ip, int port);
    QList<ChatInfo> getChats();

    bool saveMessage(int chatId, const QString& sender, const QString& type, const QString& content, bool isOutbox);
    QList<MessageInfo> getMessages(int chatId);

    bool createGroup(const QString& name, const QString& uuid, const QStringList& members);
    int getGroupIdByUuid(const QString& uuid);
    QString getGroupUuid(int groupId);
    QStringList getGroupMembers(const QString& uuid);
    bool saveGroupMessage(const QString& groupUuid, const QString& sender, const QString& type, const QString& content, bool isOutbox);
    QList<MessageInfo> getGroupMessages(const QString& groupUuid);

private:
    DatabaseManager() = default;
    QSqlDatabase m_db;
};