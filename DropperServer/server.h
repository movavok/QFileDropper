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
    void sendFile(QTcpSocket*, const QString&, QDataStream&);
    void SendToClient(const QString&, QTcpSocket*, const QByteArray& = QByteArray());
private slots:
    void slotReadyRead();
    void clientDisconnected();
};
#endif // SERVER_H
