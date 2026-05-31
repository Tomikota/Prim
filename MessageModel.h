#pragma once
#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QStyledItemDelegate>

struct Message {
    QString sender;
    QString text;
    QString type;
    QString filePath;
    QDateTime timestamp;
    bool isOutbox;
};

class MessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum MessageRoles {
        SenderRole = Qt::UserRole + 1,
        TextRole,
        TypeRole,
        FilePathRole,
        TimestampRole,
        IsOutboxRole,
        IsPlayingRole
    };

    explicit MessageModel(QObject* parent = nullptr);

    void setMessages(const QList<Message>& messages);
    void addMessage(const Message& message);
    void clear();

    void setPlayingIndex(int index);
    int playingIndex() const { return m_playingIndex; }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QList<Message> m_messages;
    int m_playingIndex = -1;
};

class MessageDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    static bool isDarkTheme;

    explicit MessageDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};