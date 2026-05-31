#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QMap>
#include <QStringList>

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    void setMyPhoneNumber(const QString& phone);
    bool startServer(quint16 tcpPort);

    void connectToPhone(const QString& phoneNumber);
    void connectToPeer(const QString& ip, quint16 port);

    void sendData(const QString& type, const QString& sender, const QString& textContent, const QByteArray& fileData = QByteArray(), const QString& fileName = QString(), const QString& groupUuid = QString());
    void sendGroupData(const QStringList& members, const QString& type, const QString& sender, const QString& textContent, const QByteArray& fileData = QByteArray(), const QString& fileName = QString(), const QString& groupUuid = QString());

signals:
    void dataReceived(QString type, QString sender, QString textContent, QByteArray fileData, QString fileName, QString groupUuid);
    void connectionStatusChanged(bool connected);
    void peerDiscovered();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

    void sendBroadcastDiscovery();
    void onUdpReadyRead();

private:
    QTcpServer* m_server;
    QTcpSocket* m_clientSocket;
    QTcpSocket* m_incomingSocket;

    QUdpSocket* m_udpSocket;
    QTimer* m_broadcastTimer;
    QString m_myPhoneNumber;
    quint16 m_myTcpPort;

    QMap<QString, QPair<QString, int>> m_discoveredPeers;
};