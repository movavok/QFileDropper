#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("QFileDropper");

    ui->sb_port->setRange(1000, 65535);
    ui->le_ip->setInputMask("000.000.000.000;_");
    ui->statusbar->setStyleSheet("color: red;");
    ui->te_receivedFiles->setOpenLinks(false);

    socket = new QTcpSocket(this);
    dbManager = new DbManager(this);

    initConnections();
    initSounds();
    initCheckBoxes();
    loadFields();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::initConnections()
{
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    connect(socket, &QTcpSocket::connected, this, &MainWindow::onSocketConnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onSocketError);

    connect(ui->actionOpen_new, &QAction::triggered, this, &MainWindow::openNewClient);
    connect(ui->b_connect,      &QPushButton::clicked, this, &MainWindow::login);
    connect(ui->b_chooseFile,   &QPushButton::clicked, this, &MainWindow::chooseFile);
    connect(ui->b_sendFile,     &QPushButton::clicked, this, &MainWindow::sendFile);

    connect(ui->cb_ip,    &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);
    connect(ui->cb_port,  &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);
    connect(ui->cb_login, &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);

    connect(ui->te_receivedFiles, &QTextBrowser::anchorClicked, this, [](const QUrl& url){
        QDesktopServices::openUrl(url);
    });
}

void MainWindow::initSounds()
{
    successSound = new QSoundEffect(this);
    successSound->setSource(QUrl("qrc:/sounds/success.wav"));
    successSound->setVolume(1);

    cancelSound = new QSoundEffect(this);
    cancelSound->setSource(QUrl("qrc:/sounds/cancel.wav"));
    cancelSound->setVolume(0.8);
}

void MainWindow::initCheckBoxes()
{
    ui->cb_ip->setChecked(dbManager->isFieldSet("ip"));
    ui->cb_port->setChecked(dbManager->isFieldSet("port"));
    ui->cb_login->setChecked(dbManager->isFieldSet("login"));
}

void MainWindow::loadFields()
{
    ui->le_ip->setText(dbManager->loadField("ip"));
    ui->sb_port->setValue(dbManager->loadField("port").toInt());
    ui->le_login->setText(dbManager->loadField("login"));
}

void MainWindow::saveFields()
{
    ui->cb_ip->isChecked()    ? dbManager->saveField("ip", ui->le_ip->text())      : dbManager->deleteField("ip");
    ui->cb_port->isChecked()  ? dbManager->saveField("port", QString::number(ui->sb_port->value())) : dbManager->deleteField("port");
    ui->cb_login->isChecked() ? dbManager->saveField("login", ui->le_login->text()) : dbManager->deleteField("login");
}

void MainWindow::updateInfoButton()
{
    bool enabled = ui->cb_ip->isChecked() || ui->cb_port->isChecked() || ui->cb_login->isChecked();
    ui->b_saveInfo->setEnabled(enabled);

    ui->b_saveInfo->setToolTip(enabled
                                   ? "<p><b>Note:</b> Checkboxes save the user's input in the fields.</p>"
                                   : "");
}

void MainWindow::openNewClient()
{
    auto* newClient = new MainWindow();
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

void MainWindow::login()
{
    if (ui->le_login->text().isEmpty()) {
        ui->statusbar->showMessage("You have not entered the login!");
        return;
    }

    socket->abort();
    QString ip = ui->le_ip->text();
    int port = ui->sb_port->value();

    qDebug() << "Connecting to" << ip << ":" << port;
    socket->connectToHost(ip, port);
}

void MainWindow::onSocketConnected()
{
    qDebug() << "Connected to server!";
    saveFields();
    SendToServer(ui->le_login->text());
    setWindowTitle("QFileDropper — " + ui->le_login->text());
    ui->stackedWidget->setCurrentIndex(1);
    ui->statusbar->clearMessage();
}

void MainWindow::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    ui->statusbar->showMessage(socket->errorString());
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

void MainWindow::chooseFile()
{
    filePath = QFileDialog::getOpenFileName(this, "Select a file", "", "All Files (*.*)");
    QFileInfo fileInfo(filePath);
    fileName = fileInfo.fileName();

    ui->le_fileName->setText(filePath.isEmpty() ? "Empty" : fileName);
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

    const qint64 chunkSize = 32 * 1024;
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
    out << quint16(0) << QString("FILE:%1:%2:%3").arg(fileName).arg(bytesSent).arg(totalBytes) << chunk;

    out.device()->seek(0);
    out << quint16(Data.size() - sizeof(quint16));
    socket->write(Data);

    bytesSent += chunk.size();
    ui->pb_sending->setValue(bytesSent);

    QTimer::singleShot(0, this, &MainWindow::sendNextChunk);
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

void MainWindow::handleMessages(const QString& header, QDataStream& in)
{
    if (header.startsWith("INFO:")) {
        ui->statusbar->showMessage(header.mid(5));

        if (header.contains("No other clients connected")) {
            if (sendingFile) {
                sendingFile->close();
                sendingFile->deleteLater();
                sendingFile = nullptr;
            }
            bytesSent = 0;
            ui->pb_sending->setValue(0);
        }
        return;
    }

    if (header == "RESET_PROGRESS") {
        ui->pb_sending->setValue(0);
        cancelSound->stop(); cancelSound->play();
        return;
    }

    if (header.startsWith("SENT:")) {
        ui->te_sentFiles->append(header.mid(5));
        ui->statusbar->clearMessage();
        return;
    }

    if (header.startsWith("FILE:")) {
        QStringList parts = header.split(":");
        if (parts.size() != 4) return;

        QString fname = parts[1];
        qint64 offset = parts[2].toLongLong();
        qint64 total  = parts[3].toLongLong();
        QByteArray chunk;
        in >> chunk;

        if (offset == 0) {
            receiveState = FileReceiveState();
            receiveState.fileName = fname;
            receiveState.totalBytes = total;
            ui->pb_receiving->setMaximum(total);
            ui->pb_receiving->setValue(0);
        }

        receiveState.fileData.append(chunk);
        receiveState.receivedBytes += chunk.size();
        ui->pb_receiving->setValue(receiveState.receivedBytes);

        if (isValidReceivedFile()) {
            saveReceivedFiles(receiveState.fileName, receiveState.fileData);
            ui->statusbar->clearMessage();
            receiveState = FileReceiveState();
        }
    }
}

bool MainWindow::isValidReceivedFile() const {
    return !receiveState.fileName.trimmed().isEmpty()
    && receiveState.fileName.contains('@')
        && !receiveState.fileName.endsWith('@')
        && receiveState.fileData.size() == receiveState.totalBytes
        && receiveState.receivedBytes >= receiveState.totalBytes;
}

void MainWindow::saveReceivedFiles(const QString& fileName, const QByteArray& fileData)
{
    if (fileName.trimmed().isEmpty() || fileData.isEmpty()) return;

    QDir dir("../Received files");
    if (!dir.exists()) dir.mkpath(".");

    QString savePath = dir.filePath(fileName);
    QFile file(savePath);

    if (file.open(QIODevice::WriteOnly)) {
        file.write(fileData);
        file.close();

        QString fileUrl = QUrl::fromLocalFile(savePath).toString();
        ui->te_receivedFiles->append(QString("<a href=\"%1\">%2</a>").arg(fileUrl, fileName));
        successSound->stop(); successSound->play();
    } else {
        ui->statusbar->showMessage("Failed to save file: " + file.errorString());
    }
}

void MainWindow::resetTransferStates()
{
    ui->pb_sending->setValue(0);
    ui->pb_receiving->setValue(0);
    receiveState = FileReceiveState();

    if (sendingFile) {
        sendingFile->close();
        sendingFile->deleteLater();
        sendingFile = nullptr;
    }

    bytesSent = 0;
    totalBytes = 0;
    fileName.clear();
}



