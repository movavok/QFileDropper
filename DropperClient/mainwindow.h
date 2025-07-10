#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <iostream>
#include <QHostAddress>
#include <QTcpSocket>
#include <QFileDialog>
#include <QFileInfo>
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    QByteArray buffer;
    QString fileName;
    QString filePath;
    quint16 nextBlockSize = 0;
    void SendToServer(const QString&, const QByteArray& = QByteArray());
    void saveReceivedFiles(const QString&, const QByteArray&);
    void handleMessages(const QString&, QDataStream&);
private slots:
    void openNewClient();
    void updateInfoButton();
    void login();
    void chooseFile();
    void sendFile();
    void onSocketConnected();
    void onSocketError(QAbstractSocket::SocketError);
    void slotReadyRead();
private:
    QFile* sendingFile = nullptr;
    qint64 bytesSent = 0;
    qint64 totalBytes = 0;
    void sendNextChunk();
    struct FileReceiveState {
        QString fileName;
        qint64 totalBytes = 0;
        qint64 receivedBytes = 0;
        QByteArray fileData;
    };
    FileReceiveState receiveState;
};
#endif // MAINWINDOW_H
