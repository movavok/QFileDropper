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
void Server::sendFile(QTcpSocket* socket, const QString& fileName, QDataStream& in)
{
    QByteArray fileData;
    in >> fileData;
    QString login = logins.value(socket, "Unknown");
    QString fullHeader = "FILE:" + login + "@" + fileName;
    if (Sockets.size() <= 1) {
        SendToClient("INFO: No other clients connected. File wasn't sent", socket);
        qDebug() << "Upload cancelled: no other clients connected.";
        return;
    }
    for (QTcpSocket* client : Sockets) {
        if (client != socket && client->state() == QAbstractSocket::ConnectedState)
            SendToClient(fullHeader, client, fileData);
    }
    qDebug() << "File" << fileName << "was sent to clients by" << login;
    SendToClient("SENT:" + fileName, socket);
}
void Server::slotReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    buffers[socket].append(socket->readAll());

    while (true) {
        if (buffers[socket].size() < 2) break;

        QDataStream sizeStream(buffers[socket]);
        sizeStream.setVersion(QDataStream::Qt_6_7);
        quint16 blockSize = 0;
        sizeStream >> blockSize;

        if (buffers[socket].size() < blockSize + 2) break;

        QByteArray fullBlock = buffers[socket].mid(2, blockSize);
        QDataStream in(&fullBlock, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_6_7);

        QString header;
        in >> header;

        if (!logins.contains(socket)) {
            logins[socket] = header;
            qDebug() << "User logged in as:" << header;
        } else if (header.startsWith("FILE:")) {
            sendFile(socket, header.mid(5), in);
        } else if (header.startsWith("DISCONNECT:")) {
            socket->disconnectFromHost();
        }

        buffers[socket].remove(0, blockSize + 2);
    }
}
void Server::SendToClient(const QString& message, QTcpSocket* socket, const QByteArray& fileData)
{
    QByteArray Data;
    QDataStream out(&Data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_7);
    out << quint16(0) << message;
    if (!fileData.isEmpty()) out << fileData;
    out.device()->seek(0);
    out << quint16(Data.size() - sizeof(quint16));
    socket->write(Data);
}
void Server::clientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    QString login = logins.value(socket, "Unknown");
    logins.remove(socket);
    Sockets.removeAll(socket);
    buffers.remove(socket);
    nextBlockSizes.remove(socket);
    socket->deleteLater();
    qDebug() << "User disconnected:" << login;
}
