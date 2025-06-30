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

private:
    QMap<QTcpSocket*, QString> logins;
    QList<QTcpSocket*> Sockets;
    QByteArray Data;
    void SendToClient(const QString&);

public slots:
    void incomingConnection(qintptr socketDescriptor);

private slots:
    void slotReadyRead();
    void clientDisconnected();
};

#endif // SERVER_H
