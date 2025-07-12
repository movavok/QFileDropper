#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("QFileDropper");
    ui->sb_port->setRange(1000, 65535);
    ui->le_ip->setInputMask("000.000.000.000;_");
    ui->statusbar->setStyleSheet("color: red;");

    ui->te_receivedFiles->setOpenLinks(false);

    socket = new QTcpSocket(this);
    dbManager = new DbManager(this);

    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);

    connect(ui->actionOpen_new, &QAction::triggered, this, &MainWindow::openNewClient);

    connect(ui->cb_ip,    &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);
    connect(ui->cb_port,  &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);
    connect(ui->cb_login, &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);

    connect(ui->b_chooseFile, &QPushButton::clicked, this, &MainWindow::chooseFile);
    connect(ui->b_sendFile,   &QPushButton::clicked, this, &MainWindow::sendFile);

    connect(socket, &QTcpSocket::connected, this, &MainWindow::onSocketConnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onSocketError);

    connect(ui->b_connect, &QPushButton::clicked, this, &MainWindow::login);

    connect(ui->te_receivedFiles, &QTextBrowser::anchorClicked, this, [](const QUrl& url){
        QDesktopServices::openUrl(url);
    });

    initSounds();
    initCheckBoxes();
    loadFilds();
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

void MainWindow::initSounds()
{
    successSound = new QSoundEffect(this);
    successSound->setSource(QUrl("qrc:/sounds/success.wav"));
    successSound->setVolume(0.8);

    cancelSound = new QSoundEffect(this);
    cancelSound->setSource(QUrl("qrc:/sounds/cancel.wav"));
    cancelSound->setVolume(0.8);
}

void MainWindow::initCheckBoxes()
{
    if (dbManager->isFieldSet("ip")) ui->cb_ip->setChecked(true);
    if (dbManager->isFieldSet("port")) ui->cb_port->setChecked(true);
    if (dbManager->isFieldSet("login")) ui->cb_login->setChecked(true);
}

void MainWindow::loadFilds()
{
    ui->le_ip->setText(dbManager->loadField("ip"));
    ui->sb_port->setValue(dbManager->loadField("port").toInt());
    ui->le_login->setText(dbManager->loadField("login"));
}

void MainWindow::saveFilds()
{
    if (ui->cb_ip->isChecked()) dbManager->saveField("ip", ui->le_ip->text());
    else dbManager->deleteField("ip");
    if (ui->cb_port->isChecked()) dbManager->saveField("port", QString::number(ui->sb_port->value()));
    else dbManager->deleteField("port");
    if (ui->cb_login->isChecked()) dbManager->saveField("login", ui->le_login->text());
    else dbManager->deleteField("login");
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
    } else {
        qDebug() << "No files selected";
        ui->le_fileName->setText("Empty");
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
    
    QTimer::singleShot(0, this, &MainWindow::sendNextChunk);
}

void MainWindow::saveReceivedFiles(const QString& fileName, const QByteArray& fileData)
{
    if (fileName.trimmed().isEmpty() || fileData.isEmpty()) return;
    QDir dir("../../Received files");
    if (!dir.exists()) dir.mkpath(".");
    QString savePath = dir.filePath(fileName);
    QFile file(savePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(fileData);
        file.close();
        QString fileUrl = QUrl::fromLocalFile(savePath).toString();
        ui->te_receivedFiles->append(QString("<a href=\"%1\">%2</a>").arg(fileUrl, fileName));
        successSound->stop();
        successSound->play();
    } else {
        ui->statusbar->showMessage("Failed to save file: " + file.errorString());
    }
}

bool MainWindow::isValidReceivedFile() const {
    return !receiveState.fileName.trimmed().isEmpty()
        && receiveState.fileName.contains('@')
        && !receiveState.fileName.endsWith('@')
        && receiveState.fileData.size() == receiveState.totalBytes
        && receiveState.receivedBytes >= receiveState.totalBytes;
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
        cancelSound->stop();
        cancelSound->play();
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
        qint64 total = parts[3].toLongLong();
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
        return;
    }
}

void MainWindow::resetTransferStates() {
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
    saveFilds();
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

void MainWindow::updateInfoButton()
{
    bool enabled = (ui->cb_ip->isChecked() ||
                    ui->cb_port->isChecked() ||
                    ui->cb_login->isChecked());
    ui->b_saveInfo->setEnabled(enabled);

    if (enabled) ui->b_saveInfo->setToolTip("<p><b>Note:</b> Checkboxes save the user's input in the fields.</p>");
    else ui->b_saveInfo->setToolTip("");
}

