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
    QMap<QTcpSocket*, QByteArray> buffers;
    QMap<QTcpSocket*, quint16> nextBlockSizes;
    void SendToClient(const QString&, QTcpSocket*, const QByteArray& = QByteArray());
    void sendFileChunks(const QString& fileName, const QByteArray& fileData, QTcpSocket* excludeSocket);
private slots:
    void slotReadyRead();
    void clientDisconnected();
};
#endif // SERVER_H

struct FileReceiveState {
    QString fileName;
    qint64 totalBytes = 0;
    qint64 receivedBytes = 0;
    QByteArray fileData;
};
extern QMap<QTcpSocket*, FileReceiveState> fileStates;
