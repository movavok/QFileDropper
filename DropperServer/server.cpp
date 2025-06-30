#include "server.h"
#include <QDataStream>
#include <QDebug>

Server::Server()
{
    if (this->listen(QHostAddress::Any, 8081)) qDebug() << "start";
    else qDebug() << "error";
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    connect(socket, &QTcpSocket::readyRead, this, &Server::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Server::clientDisconnected);

    Sockets.push_back(socket);
    qDebug() << "client connected" << socketDescriptor;
}

void Server::slotReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_6_7);

    QString message;
    in >> message;

    if (!logins.contains(socket)) {
        logins[socket] = message;
        qDebug() << "User logged in as:" << message;
        return;
    }

    QString login = logins[socket];
    QString fullMessage = login + ": " + message;

    SendToClient(fullMessage);
}

void Server::SendToClient(const QString& str)
{
    Data.clear();
    QDataStream out(&Data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_7);
    out << str;

    for (QTcpSocket* client : Sockets) {
        if (client->state() == QAbstractSocket::ConnectedState)
            client->write(Data);
    }
}

void Server::clientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QString login = logins.value(socket, "Unknown");

    logins.remove(socket);
    Sockets.removeAll(socket);
    socket->deleteLater();

    qDebug() << "User disconnected:" << login;
}
