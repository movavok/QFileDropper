#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHostAddress>
#include <QTcpSocket>
#include <QFileDialog>
#include <QFileInfo>
#include <QTimer>
#include <QDesktopServices>
#include <QSoundEffect>

#include <dbmanager.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow* ui;
    QTcpSocket* socket;
    DbManager* dbManager;

    QSoundEffect* successSound;
    QSoundEffect* cancelSound;

    QByteArray buffer;
    QString fileName;
    QString filePath;
    quint16 nextBlockSize = 0;

    QFile* sendingFile = nullptr;
    qint64 bytesSent = 0;
    qint64 totalBytes = 0;

    struct FileReceiveState {
        QString fileName;
        qint64 totalBytes = 0;
        qint64 receivedBytes = 0;
        QByteArray fileData;
    };
    FileReceiveState receiveState;

    void SendToServer(const QString& header, const QByteArray& fileData = QByteArray());
    void initSounds();
    void initCheckBoxes();
    void loadFilds();
    void saveFilds();
    void saveReceivedFiles(const QString& fileName, const QByteArray& fileData);
    void handleMessages(const QString& header, QDataStream& in);
    void sendNextChunk();
    void resetTransferStates();
    bool isValidReceivedFile() const;

private slots:
    void openNewClient();
    void updateInfoButton();
    void login();
    void chooseFile();
    void sendFile();
    void onSocketConnected();
    void onSocketError(QAbstractSocket::SocketError);
    void slotReadyRead();
};

#endif // MAINWINDOW_H

