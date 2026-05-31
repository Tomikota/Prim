#include "DatabaseManager.h"
#include <QCryptographicHash>

bool DatabaseManager::init() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("local_messenger.db");
    if (!m_db.open()) return false;

    QSqlQuery q;
    q.exec("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, phone_number TEXT UNIQUE, password_hash TEXT)");
    q.exec("CREATE TABLE IF NOT EXISTS chats (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, phone_number TEXT UNIQUE, ip TEXT, port INTEGER)");
    q.exec("CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, chat_id INTEGER, sender TEXT, type TEXT, content TEXT, timestamp DATETIME, is_outbox INTEGER, group_uuid TEXT)");

    q.exec("CREATE TABLE IF NOT EXISTS groups (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, uuid TEXT UNIQUE)");
    q.exec("CREATE TABLE IF NOT EXISTS group_members (group_id INTEGER, phone_number TEXT)");

    q.exec("ALTER TABLE messages ADD COLUMN group_uuid TEXT");

    return true;
}

bool DatabaseManager::registerUser(const QString& phoneNumber, const QString& password) {
    QString hash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
    QSqlQuery q;
    q.prepare("INSERT INTO users (phone_number, password_hash) VALUES (:phone, :hash)");
    q.bindValue(":phone", phoneNumber);
    q.bindValue(":hash", hash);
    return q.exec();
}

bool DatabaseManager::loginUser(const QString& phoneNumber, const QString& password) {
    QString hash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
    QSqlQuery q;
    q.prepare("SELECT id FROM users WHERE phone_number = :phone AND password_hash = :hash");
    q.bindValue(":phone", phoneNumber);
    q.bindValue(":hash", hash);
    return q.exec() && q.next();
}

bool DatabaseManager::addChat(const QString& name, const QString& phoneNumber) {
    QSqlQuery q;
    q.prepare("INSERT INTO chats (name, phone_number, ip, port) VALUES (:name, :phone, '', 0)");
    q.bindValue(":name", name);
    q.bindValue(":phone", phoneNumber);
    return q.exec();
}

bool DatabaseManager::deleteChat(int chatId) {
    QSqlQuery q;
    q.prepare("DELETE FROM chats WHERE id = :id");
    q.bindValue(":id", chatId);
    q.exec();
    q.prepare("DELETE FROM messages WHERE chat_id = :id");
    q.bindValue(":id", chatId);
    return q.exec();
}

bool DatabaseManager::renameChat(int chatId, const QString& newName) {
    QSqlQuery q;
    q.prepare("UPDATE chats SET name = :name WHERE id = :id");
    q.bindValue(":name", newName);
    q.bindValue(":id", chatId);
    return q.exec();
}

bool DatabaseManager::updateChatAddress(const QString& phoneNumber, const QString& ip, int port) {
    QSqlQuery q;
    q.prepare("UPDATE chats SET ip = :ip, port = :port WHERE phone_number = :phone");
    q.bindValue(":ip", ip);
    q.bindValue(":port", port);
    q.bindValue(":phone", phoneNumber);
    return q.exec();
}

QList<ChatInfo> DatabaseManager::getChats() {
    QList<ChatInfo> list;
    QSqlQuery q("SELECT id, name, phone_number, ip, port FROM chats");
    while (q.next()) {
        list.append({ q.value(0).toInt(), q.value(1).toString(), q.value(2).toString(), q.value(3).toString(), q.value(4).toInt() });
    }
    return list;
}

bool DatabaseManager::saveMessage(int chatId, const QString& sender, const QString& type, const QString& content, bool isOutbox) {
    QSqlQuery q;
    q.prepare("INSERT INTO messages (chat_id, sender, type, content, timestamp, is_outbox, group_uuid) VALUES (:chat_id, :sender, :type, :content, :time, :outbox, NULL)");
    q.bindValue(":chat_id", chatId);
    q.bindValue(":sender", sender);
    q.bindValue(":type", type);
    q.bindValue(":content", content);
    q.bindValue(":time", QDateTime::currentDateTime());
    q.bindValue(":outbox", isOutbox ? 1 : 0);
    return q.exec();
}

QList<MessageInfo> DatabaseManager::getMessages(int chatId) {
    QList<MessageInfo> list;
    QSqlQuery q;
    q.prepare("SELECT sender, type, content, timestamp, is_outbox FROM messages WHERE chat_id = :id ORDER BY timestamp ASC");
    q.bindValue(":id", chatId);
    if (q.exec()) {
        while (q.next()) {
            list.append({ q.value(0).toString(), q.value(1).toString(), q.value(2).toString(), q.value(3).toDateTime(), q.value(4).toInt() == 1 });
        }
    }
    return list;
}

bool DatabaseManager::createGroup(const QString& name, const QString& uuid, const QStringList& members) {
    QSqlQuery q;
    q.prepare("INSERT OR IGNORE INTO groups (name, uuid) VALUES (:name, :uuid)");
    q.bindValue(":name", name);
    q.bindValue(":uuid", uuid);
    if (!q.exec()) return false;

    int groupId = getGroupIdByUuid(uuid);
    if (groupId == -1) return false;

    q.prepare("DELETE FROM group_members WHERE group_id = :group_id");
    q.bindValue(":group_id", groupId);
    q.exec();

    for (const QString& member : members) {
        if (member.trimmed().isEmpty()) continue;
        q.prepare("INSERT INTO group_members (group_id, phone_number) VALUES (:group_id, :phone)");
        q.bindValue(":group_id", groupId);
        q.bindValue(":phone", member.trimmed());
        q.exec();
    }
    return true;
}

int DatabaseManager::getGroupIdByUuid(const QString& uuid) {
    QSqlQuery q;
    q.prepare("SELECT id FROM groups WHERE uuid = :uuid");
    q.bindValue(":uuid", uuid);
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return -1;
}

QString DatabaseManager::getGroupUuid(int groupId) {
    QSqlQuery q;
    q.prepare("SELECT uuid FROM groups WHERE id = :id");
    q.bindValue(":id", groupId);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return "";
}

QStringList DatabaseManager::getGroupMembers(const QString& uuid) {
    QStringList list;
    int groupId = getGroupIdByUuid(uuid);
    if (groupId == -1) return list;

    QSqlQuery q;
    q.prepare("SELECT phone_number FROM group_members WHERE group_id = :id");
    q.bindValue(":id", groupId);
    if (q.exec()) {
        while (q.next()) {
            list.append(q.value(0).toString());
        }
    }
    return list;
}

bool DatabaseManager::saveGroupMessage(const QString& groupUuid, const QString& sender, const QString& type, const QString& content, bool isOutbox) {
    QSqlQuery q;
    q.prepare("INSERT INTO messages (chat_id, sender, type, content, timestamp, is_outbox, group_uuid) VALUES (NULL, :sender, :type, :content, :time, :outbox, :group_uuid)");
    q.bindValue(":sender", sender);
    q.bindValue(":type", type);
    q.bindValue(":content", content);
    q.bindValue(":time", QDateTime::currentDateTime());
    q.bindValue(":outbox", isOutbox ? 1 : 0);
    q.bindValue(":group_uuid", groupUuid);
    return q.exec();
}

QList<MessageInfo> DatabaseManager::getGroupMessages(const QString& groupUuid) {
    QList<MessageInfo> list;
    QSqlQuery q;
    q.prepare("SELECT sender, type, content, timestamp, is_outbox FROM messages WHERE group_uuid = :uuid ORDER BY timestamp ASC");
    q.bindValue(":uuid", groupUuid);
    if (q.exec()) {
        while (q.next()) {
            list.append({ q.value(0).toString(), q.value(1).toString(), q.value(2).toString(), q.value(3).toDateTime(), q.value(4).toInt() == 1 });
        }
    }
    return list;
}