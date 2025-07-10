#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->sb_port->setRange(1000, 65535);
    ui->le_ip->setInputMask("000.000.000.000;_");
    ui->statusbar->setStyleSheet("color: red;");

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);

    connect(ui->actionOpen_new, &QAction::triggered, this, &MainWindow::openNewClient);

    connect(ui->cb_ip,    &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);
    connect(ui->cb_port,  &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);
    connect(ui->cb_login, &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);

    connect(ui->b_chooseFile, &QPushButton::clicked, this, &MainWindow::chooseFile);
    connect(ui->b_sendFile, &QPushButton::clicked, this, &MainWindow::sendFile);

    connect(socket, &QTcpSocket::connected, this, &MainWindow::onSocketConnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onSocketError);

    connect(ui->b_connect, &QPushButton::clicked, this, &MainWindow::login);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openNewClient()
{
    MainWindow* newClient = new MainWindow();
    newClient->show();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        SendToServer("DISCONNECT:" + ui->le_login->text());
        socket->disconnectFromHost();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::SendToServer(const QString& header, const QByteArray& fileData)
{
    QByteArray Data;
    QDataStream out(&Data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_7);
    out << quint16(0) << header;

    if (!fileData.isEmpty()) out << fileData;

    out.device()->seek(0);
    out << quint16(Data.size() - sizeof(quint16));
    socket->write(Data);
}

void MainWindow::login()
{
    if (ui->le_login->text().isEmpty()) {
        ui->statusbar->showMessage("You have not entered the login!");
        return;
    }

    QString ip = ui->le_ip->text();
    int port = ui->sb_port->value();

    socket->abort();
    qDebug() << "Connecting to" << ip << ":" << port;
    socket->connectToHost(ip, port);
}

void MainWindow::chooseFile()
{
    filePath = QFileDialog::getOpenFileName(this, "Select a file", "", "All Files (*.*)");

    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        fileName = fileInfo.fileName();
        qDebug() << "Selected file:" << filePath << "(" + fileName + ")";
        ui->le_fileName->setText(fileName);
    }
}

void MainWindow::sendFile()
{
    if (filePath.isEmpty()) {
        ui->statusbar->showMessage("No file selected.");
        return;
    }
    sendingFile = new QFile(filePath, this);
    if (!sendingFile->open(QIODevice::ReadOnly)) {
        ui->statusbar->showMessage("Failed to open file.");
        delete sendingFile;
        sendingFile = nullptr;
        return;
    }
    bytesSent = 0;
    totalBytes = sendingFile->size();
    ui->pb_sending->setMaximum(totalBytes);
    ui->pb_sending->setValue(0);
    sendNextChunk();
}

void MainWindow::sendNextChunk()
{
    if (!sendingFile) return;
    const qint64 chunkSize = 32 * 1024; // 32KB
    QByteArray chunk = sendingFile->read(chunkSize);
    if (chunk.isEmpty()) {
        sendingFile->close();
        sendingFile->deleteLater();
        sendingFile = nullptr;
        return;
    }
    QByteArray Data;
    QDataStream out(&Data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_7);
    out << quint16(0) << QString("FILE:%1:%2:%3").arg(fileName).arg(bytesSent).arg(totalBytes);
    out << chunk;
    out.device()->seek(0);
    out << quint16(Data.size() - sizeof(quint16));
    socket->write(Data);
    bytesSent += chunk.size();
    ui->pb_sending->setValue(bytesSent);
    // Надсилаємо наступний chunk після підтвердження від сервера або одразу:
    QTimer::singleShot(0, this, &MainWindow::sendNextChunk);
}

void MainWindow::saveReceivedFiles(const QString& fileName, const QByteArray& fileData)
{
    QDir dir("../../Received files");
    if (!dir.exists()) dir.mkpath(".");

    QString savePath = dir.filePath(fileName);
    QFile file(savePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(fileData);
        file.close();
        ui->te_receivedFiles->append(fileName);
    } else {
        ui->statusbar->showMessage("Failed to save file: " + file.errorString());
    }
}

void MainWindow::handleMessages(const QString& header, QDataStream& in)
{
    if (header.startsWith("INFO:")) {
        ui->statusbar->showMessage(header.mid(5));
        return;
    } else if (header.startsWith("SENT:")) {
        ui->te_sentFiles->append(header.mid(5));
        return;
    } else if (header.startsWith("FILE:")) {
        // Новий формат: FILE:filename:offset:total
        QStringList parts = header.split(":");
        if (parts.size() == 4) {
            QString fname = parts[1];
            qint64 offset = parts[2].toLongLong();
            qint64 total = parts[3].toLongLong();
            QByteArray chunk;
            in >> chunk;
            if (offset == 0) {
                receiveState.fileName = fname;
                receiveState.totalBytes = total;
                receiveState.receivedBytes = 0;
                receiveState.fileData.clear();
                ui->pb_receiving->setMaximum(total);
                ui->pb_receiving->setValue(0);
            }
            receiveState.fileData.append(chunk);
            receiveState.receivedBytes += chunk.size();
            ui->pb_receiving->setValue(receiveState.receivedBytes);
            if (receiveState.receivedBytes >= receiveState.totalBytes) {
                saveReceivedFiles(receiveState.fileName, receiveState.fileData);
                receiveState = FileReceiveState();
            }
        }
        return;
    }
}

void MainWindow::slotReadyRead()
{
    buffer.append(socket->readAll());

    while (true) {
        if (buffer.size() < 2) break;

        QDataStream sizeStream(buffer);
        sizeStream.setVersion(QDataStream::Qt_6_7);
        quint16 blockSize = 0;
        sizeStream >> blockSize;

        if (buffer.size() < blockSize + 2) break;

        QByteArray fullBlock = buffer.mid(2, blockSize);
        QDataStream in(&fullBlock, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_6_7);

        QString header;
        in >> header;

        handleMessages(header, in);

        buffer.remove(0, blockSize + 2);
    }
}

void MainWindow::onSocketConnected()
{
    qDebug() << "Connected to server!";
    SendToServer(ui->le_login->text());
    ui->stackedWidget->setCurrentIndex(1);
    ui->statusbar->clearMessage();
}

void MainWindow::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    ui->statusbar->showMessage(socket->errorString());
}

void MainWindow::updateInfoButton()
{
    bool enabled = (ui->cb_ip->isChecked() ||
                    ui->cb_port->isChecked() ||
                    ui->cb_login->isChecked());
    ui->b_saveInfo->setEnabled(enabled);

    if (enabled) ui->b_saveInfo->setToolTip("<p><b>Note:</b> Checkboxes save the user's input in the fields.</p>");
    else ui->b_saveInfo->setToolTip("");
}

