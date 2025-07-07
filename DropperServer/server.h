#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>

class Server : public QTcpServer
{
    Q_OBJECT

public:
    Server();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QMap<QTcpSocket*, QString> logins;
    QList<QTcpSocket*> Sockets;
    QByteArray Data;
    void sendFile(QTcpSocket*, const QString&, QDataStream&);
    void SendToClient(const QString&);

private slots:
    void slotReadyRead();
    void clientDisconnected();
};

#endif // SERVER_H
