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

    Sockets.append(socket);
    qDebug() << "Client has been connected" << socketDescriptor;
}

void Server::slotReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_6_7);

    QString header;
    in >> header;

    if (!logins.contains(socket)) {
        logins[socket] = header;
        qDebug() << "User logged in as:" << header;
        return;
    }

    if (header.startsWith("FILE:")) {
        QString fileName = header.mid(5);
        QByteArray fileData;
        in >> fileData;

        QString login = logins[socket];
        QString fullHeader = "FILE:" + login + "@" + fileName;

        if (Sockets.size() <= 1) { // cancel downloading if no other clients
            QByteArray reply;
            QDataStream out(&reply, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_6_7);
            out << QString("INFO: No other clients connected. File wasn't sent");
            socket->write(reply);
            qDebug() << "Upload cancelled: no other clients connected.";
            return;
        }

        QByteArray dataToSend;
        QDataStream out(&dataToSend, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_7);
        out << fullHeader << fileData;

        for (QTcpSocket* client : Sockets) {
            if (client != socket && client->state() == QAbstractSocket::ConnectedState)
                client->write(dataToSend);
        }

        qDebug() << "File " << fileName << "was sent to clients by " << login;

        QByteArray confirm;
        QDataStream confirmOut(&confirm, QIODevice::WriteOnly);
        confirmOut.setVersion(QDataStream::Qt_6_7);
        confirmOut << QString("SENT:" + fileName);
        socket->write(confirm);
    }

    if (header.startsWith("DISCONNECT:")) {
        qDebug() << "Client requested disconnect:" << logins.value(socket, "Unknown");
        socket->disconnectFromHost();
        return;
    }
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
