#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    QByteArray Data;
    QString fileName;
    QString filePath;
    void SendToServer(QString);
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
};
#endif // MAINWINDOW_H
