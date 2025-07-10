#include "server.h"
#include <QDataStream>
#include <QDebug>

QMap<QTcpSocket*, FileReceiveState> fileStates;

Server::Server()
{
    if (this->listen(QHostAddress::Any, 8081)) qDebug() << "[INFO] Server started";
    else qDebug() << "[ERROR] Server start failed";
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    connect(socket, &QTcpSocket::readyRead, this, &Server::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Server::clientDisconnected);
    Sockets.append(socket);
    qDebug() << "[INFO] Client connected:" << socketDescriptor;
}

void Server::sendFileChunks(const QString& fileName, const QByteArray& fileData, QTcpSocket* excludeSocket)
{
    const qint64 chunkSize = 32 * 1024;
    qint64 totalBytes = fileData.size();
    qint64 bytesSent = 0;
    while (bytesSent < totalBytes) {
        QByteArray chunk = fileData.mid(bytesSent, chunkSize);
        QString header = QString("FILE:%1:%2:%3").arg(fileName).arg(bytesSent).arg(totalBytes);
        for (QTcpSocket* client : Sockets) {
            if (client != excludeSocket && client->state() == QAbstractSocket::ConnectedState) {
                SendToClient(header, client, chunk);
            }
        }
        bytesSent += chunk.size();
    }
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
            qDebug() << "[LOGIN] User logged in as:" << header;
        } else if (header.startsWith("FILE:")) {
            QStringList parts = header.split(":");
            if (parts.size() == 4) {
                QString fname = parts[1];
                qint64 offset = parts[2].toLongLong();
                qint64 total = parts[3].toLongLong();
                QByteArray chunk;
                in >> chunk;
                FileReceiveState& state = fileStates[socket];
                if (offset == 0) {
                    qDebug() << "[RECEIVE] Start receiving file:" << fname << "from" << logins[socket];
                    state.fileName = fname;
                    state.totalBytes = total;
                    state.receivedBytes = 0;
                    state.fileData.clear();
                }
                state.fileData.append(chunk);
                state.receivedBytes += chunk.size();

                if (state.receivedBytes >= state.totalBytes) {
                    if (Sockets.size() <= 1) {
                        SendToClient("INFO: No other clients connected. File wasn't sent", socket);
                        SendToClient("RESET_PROGRESS", socket);
                        qDebug() << "[SEND] Upload cancelled: no other clients connected.";
                        fileStates.remove(socket);
                        return;
                    }
                    qDebug() << "[SEND] File" << state.fileName << "received, forwarding to other clients...";
                    sendFileChunks(state.fileName, state.fileData, socket);
                    SendToClient("SENT:" + state.fileName, socket);
                    fileStates.remove(socket);
                }
            }
        } else if (header.startsWith("DISCONNECT:")) {
            socket->disconnectFromHost();
            qDebug() << "[DISCONNECT] User requested disconnect:" << logins.value(socket, "Unknown");
        } else if (header == "RESET_PROGRESS") {
            fileStates.remove(socket);
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
    qDebug() << "[DISCONNECT] User disconnected:" << login;
}
