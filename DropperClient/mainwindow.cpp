#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::slotReadyRead);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);

    connect(ui->actionOpen_new, &QAction::triggered, this, &MainWindow::openNewClient);

    ui->sb_port->setRange(1000, 65535);
    ui->le_ip->setInputMask("000.000.000.000;_");
    ui->statusbar->setStyleSheet("color: red;");

    connect(ui->cb_ip,    &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);
    connect(ui->cb_port,  &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);
    connect(ui->cb_login, &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);

    connect(ui->b_chooseFile, &QPushButton::clicked, this, &MainWindow::chooseFile);
    connect(ui->b_sendFile, &QPushButton::clicked, this, &MainWindow::sendFile);

    connect(socket, &QTcpSocket::connected, this, [this]() {
        qDebug() << "Connected to server!";
        SendToServer(ui->le_login->text());
        ui->stackedWidget->setCurrentIndex(1);
    });

    connect(socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError socketError){
        Q_UNUSED(socketError);
        ui->statusbar->showMessage(socket->errorString());
    });

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

void MainWindow::SendToServer(QString str)
{
    Data.clear();
    QDataStream out(&Data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_7);
    out << str;
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
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        ui->statusbar->showMessage("Failed to open file.");
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_7);

    out << QString("FILE:" + fileName);
    out << fileData;

    socket->write(packet);
}

void MainWindow::slotReadyRead()
{
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_6_7);

    QString header;
    in >> header;

    if (header.startsWith("FILE:")) {
        QString fileName = header.mid(5);

        QByteArray fileData;
        in >> fileData;

        QDir dir("Received files");
        if (!dir.exists()) dir.mkpath("Received files");

        QString savePath = dir.filePath(fileName);
        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(fileData);
            file.close();
            ui->te_receivedFiles->append(fileName);
        } else {
            ui->statusbar->showMessage("Failed to save file: " + file.errorString());
        }

        return;
    }
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
