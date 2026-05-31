#include "MessageModel.h"
#include <QPainter>
#include <QStyleOptionViewItem>

bool MessageDelegate::isDarkTheme = true;

MessageModel::MessageModel(QObject* parent) : QAbstractListModel(parent) {}

void MessageModel::setMessages(const QList<Message>& messages) {
    beginResetModel();
    m_messages = messages;
    m_playingIndex = -1;
    endResetModel();
}

void MessageModel::addMessage(const Message& message) {
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(message);
    endInsertRows();
}

void MessageModel::clear() {
    beginResetModel();
    m_messages.clear();
    m_playingIndex = -1;
    endResetModel();
}

void MessageModel::setPlayingIndex(int index) {
    int oldIndex = m_playingIndex;
    m_playingIndex = index;

    if (oldIndex != -1) {
        emit dataChanged(this->index(oldIndex), this->index(oldIndex), { IsPlayingRole });
    }
    if (m_playingIndex != -1) {
        emit dataChanged(this->index(m_playingIndex), this->index(m_playingIndex), { IsPlayingRole });
    }
}

int MessageModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_messages.size();
}

QVariant MessageModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_messages.size()) return QVariant();
    const auto& msg = m_messages[index.row()];
    switch (role) {
    case SenderRole: return msg.sender;
    case TextRole: return msg.text;
    case TypeRole: return msg.type;
    case FilePathRole: return msg.filePath;
    case TimestampRole: return msg.timestamp;
    case IsOutboxRole: return msg.isOutbox;
    case IsPlayingRole: return index.row() == m_playingIndex;
    default: return QVariant();
    }
}

QHash<int, QByteArray> MessageModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[SenderRole] = "sender";
    roles[TextRole] = "text";
    roles[TypeRole] = "type";
    roles[FilePathRole] = "filePath";
    roles[TimestampRole] = "timestamp";
    roles[IsOutboxRole] = "isOutbox";
    roles[IsPlayingRole] = "isPlaying";
    return roles;
}

MessageDelegate::MessageDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void MessageDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    bool isOutbox = index.data(MessageModel::IsOutboxRole).toBool();
    QString text = index.data(MessageModel::TextRole).toString();
    QString sender = index.data(MessageModel::SenderRole).toString();
    QString type = index.data(MessageModel::TypeRole).toString();
    QDateTime timestamp = index.data(MessageModel::TimestampRole).toDateTime();
    QString timeStr = timestamp.toString("hh:mm");

    QRect rect = option.rect;

    int bubbleWidth = qMin(rect.width() - 80, 420);
    int bubbleHeight = rect.height() - 6;

    int x = isOutbox ? (rect.right() - bubbleWidth - 12) : (rect.left() + 12);
    int y = rect.top() + 3;
    QRect bubbleRect(x, y, bubbleWidth, bubbleHeight);

    QColor bubbleColor;
    QColor textColor;
    QColor senderColor;
    QColor timeColor;
    QColor shadowColor;

    if (isDarkTheme) {
        bubbleColor = isOutbox ? QColor("#2b5278") : QColor("#182533");
        textColor = QColor("#ffffff");
        senderColor = QColor("#5288c1");
        timeColor = QColor("#708499");
        shadowColor = QColor(10, 12, 11, 40);
    }
    else {
        bubbleColor = isOutbox ? QColor("#e1f3fc") : QColor("#ffffff");
        textColor = QColor("#3c3d3a");
        senderColor = QColor("#7c9a7b");
        timeColor = QColor("#a3a29e");
        shadowColor = QColor(150, 150, 145, 30);
    }

    painter->setBrush(shadowColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(bubbleRect.translated(0, 1), 14, 14);

    painter->setBrush(bubbleColor);
    painter->drawRoundedRect(bubbleRect, 14, 14);

    QRect contentRect = bubbleRect.adjusted(12, 8, -12, -8);

    if (!isOutbox) {
        QFont senderFont = option.font;
        senderFont.setBold(true);
        senderFont.setPointSize(9);
        painter->setFont(senderFont);
        painter->setPen(senderColor);
        painter->drawText(contentRect, Qt::AlignTop | Qt::AlignLeft, sender);
        contentRect.setTop(contentRect.top() + 18);
    }

    QFont textFont = option.font;
    textFont.setPointSize(10);
    painter->setFont(textFont);
    painter->setPen(textColor);

    if (type == "voice") {
        bool isPlaying = index.data(MessageModel::IsPlayingRole).toBool();
        QString voiceText = isPlaying ? "🔊 Проигрывается..." : "🎤 Голосовое сообщение";
        painter->drawText(contentRect, Qt::AlignVCenter | Qt::AlignLeft, voiceText);
    }
    else if (type == "image") {
        painter->drawText(contentRect, Qt::AlignVCenter | Qt::AlignLeft, "🖼️ Фотография (кликните)");
    }
    else {
        painter->drawText(contentRect, Qt::TextWordWrap, text);
    }

    QFont timeFont = option.font;
    timeFont.setPointSize(8);
    painter->setFont(timeFont);
    painter->setPen(timeColor);
    painter->drawText(bubbleRect.adjusted(0, 0, -12, -6), Qt::AlignBottom | Qt::AlignRight, timeStr);

    painter->restore();
}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QString type = index.data(MessageModel::TypeRole).toString();
    bool isOutbox = index.data(MessageModel::IsOutboxRole).toBool();

    int extraHeight = 16;
    if (!isOutbox) {
        extraHeight += 18;
    }

    if (type == "voice" || type == "image") {
        return QSize(option.rect.width(), 45 + extraHeight);
    }

    QString text = index.data(MessageModel::TextRole).toString();

    QFont textFont = option.font;
    textFont.setPointSize(10);
    QFontMetrics fm(textFont);

    int maxTextWidth = qMin(option.rect.width() - 80, 420) - 28;
    QRect textRect = fm.boundingRect(0, 0, maxTextWidth, 2000, Qt::TextWordWrap, text);

    int calculatedHeight = textRect.height() + extraHeight + 8;
    return QSize(option.rect.width(), qMax(50, calculatedHeight));
}