#include "NetworkManager.h"
#include "DatabaseManager.h"
#include <QDataStream>
#include <QSqlQuery>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent), m_server(new QTcpServer(this)), m_clientSocket(nullptr), m_incomingSocket(nullptr),
    m_udpSocket(new QUdpSocket(this)), m_broadcastTimer(new QTimer(this)), m_myTcpPort(12345) {

    connect(m_server, &QTcpServer::newConnection, this, &NetworkManager::onNewConnection);

    m_udpSocket->bind(12344, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::onUdpReadyRead);

    connect(m_broadcastTimer, &QTimer::timeout, this, &NetworkManager::sendBroadcastDiscovery);
    m_broadcastTimer->start(3000);
}

NetworkManager::~NetworkManager() {
    if (m_clientSocket) m_clientSocket->disconnectFromHost();
}

void NetworkManager::setMyPhoneNumber(const QString& phone) {
    m_myPhoneNumber = phone;
}

bool NetworkManager::startServer(quint16 tcpPort) {
    m_myTcpPort = tcpPort;
    if (m_server->isListening()) m_server->close();
    return m_server->listen(QHostAddress::Any, m_myTcpPort);
}

void NetworkManager::sendBroadcastDiscovery() {
    if (m_myPhoneNumber.isEmpty()) return;

    QString packet = QString("DISCOVER:%1:%2").arg(m_myPhoneNumber).arg(m_myTcpPort);
    m_udpSocket->writeDatagram(packet.toUtf8(), QHostAddress::Broadcast, 12344);
}

void NetworkManager::onUdpReadyRead() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress senderIp;
        quint16 senderPort;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderIp, &senderPort);

        QString message = QString::fromUtf8(datagram);
        if (message.startsWith("DISCOVER:")) {
            QStringList parts = message.split(":");
            if (parts.size() >= 3) {
                QString peerPhone = parts[1];
                int peerTcpPort = parts[2].toInt();

                if (peerPhone == m_myPhoneNumber) continue;

                QString ip = senderIp.toString();
                if (ip.startsWith("::ffff:")) {
                    ip = ip.mid(7);
                }

                m_discoveredPeers[peerPhone] = qMakePair(ip, peerTcpPort);

                if (DatabaseManager::instance().updateChatAddress(peerPhone, ip, peerTcpPort)) {
                    emit peerDiscovered();
                }
            }
        }
    }
}

void NetworkManager::connectToPhone(const QString& phoneNumber) {
    if (m_discoveredPeers.contains(phoneNumber)) {
        auto peer = m_discoveredPeers[phoneNumber];
        connectToPeer(peer.first, peer.second);
    }
    else {
        QSqlQuery q;
        q.prepare("SELECT ip, port FROM chats WHERE phone_number = :phone");
        q.bindValue(":phone", phoneNumber);
        if (q.exec() && q.next()) {
            QString lastIp = q.value(0).toString();
            int lastPort = q.value(1).toInt();
            if (!lastIp.isEmpty() && lastPort != 0) {
                connectToPeer(lastIp, lastPort);
            }
        }
    }
}

void NetworkManager::connectToPeer(const QString& ip, quint16 port) {
    if (m_clientSocket) {
        m_clientSocket->disconnectFromHost();
        m_clientSocket->deleteLater();
    }
    m_clientSocket = new QTcpSocket(this);
    connect(m_clientSocket, &QTcpSocket::connected, [this]() { emit connectionStatusChanged(true); });
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &NetworkManager::onDisconnected);
    m_clientSocket->connectToHost(ip, port);
}

void NetworkManager::onNewConnection() {
    m_incomingSocket = m_server->nextPendingConnection();
    connect(m_incomingSocket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
}

void NetworkManager::onReadyRead() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_6_0);
    static quint32 blockSize = 0;

    if (blockSize == 0) {
        if (socket->bytesAvailable() < sizeof(quint32)) return;
        in >> blockSize;
    }

    if (socket->bytesAvailable() < blockSize) return;

    QString type, senderName, textContent, fileName, groupUuid;
    QByteArray fileData;
    in >> type >> senderName >> textContent >> fileData >> fileName;

    if (!in.atEnd()) {
        in >> groupUuid;
    }
    blockSize = 0;

    emit dataReceived(type, senderName, textContent, fileData, fileName, groupUuid);
}

void NetworkManager::sendData(const QString& type, const QString& sender, const QString& textContent, const QByteArray& fileData, const QString& fileName, const QString& groupUuid) {
    if (!m_clientSocket || m_clientSocket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint32(0) << type << sender << textContent << fileData << fileName << groupUuid;
    out.device()->seek(0);
    out << quint32(block.size() - sizeof(quint32));
    m_clientSocket->write(block);
}

void NetworkManager::sendGroupData(const QStringList& members, const QString& type, const QString& sender, const QString& textContent, const QByteArray& fileData, const QString& fileName, const QString& groupUuid) {
    for (const QString& member : members) {
        if (member == m_myPhoneNumber) continue;

        QString ip;
        int port = 0;

        if (m_discoveredPeers.contains(member)) {
            ip = m_discoveredPeers[member].first;
            port = m_discoveredPeers[member].second;
        }
        else {
            QSqlQuery q;
            q.prepare("SELECT ip, port FROM chats WHERE phone_number = :phone");
            q.bindValue(":phone", member);
            if (q.exec() && q.next()) {
                ip = q.value(0).toString();
                port = q.value(1).toInt();
            }
        }

        if (!ip.isEmpty() && port != 0) {
            QTcpSocket* socket = new QTcpSocket(this);
            connect(socket, &QTcpSocket::connected, [socket, type, sender, textContent, fileData, fileName, groupUuid]() {
                QByteArray block;
                QDataStream out(&block, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_6_0);
                out << quint32(0) << type << sender << textContent << fileData << fileName << groupUuid;
                out.device()->seek(0);
                out << quint32(block.size() - sizeof(quint32));
                socket->write(block);
                socket->disconnectFromHost();
                });
            connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
            socket->connectToHost(ip, port);
        }
    }
}

void NetworkManager::onDisconnected() {
    emit connectionStatusChanged(false);
}