#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->sb_port->setRange(1000, 65535);
    ui->le_ip->setInputMask("000.000.000.000;0");
    ui->statusbar->setStyleSheet("color: red;");
    ui->b_saveInfo->setToolTip("<p><b>Note:</b> Checkboxes save the user's input in the fields.</p>");

    connect(ui->cb_ip,    &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);
    connect(ui->cb_port,  &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);
    connect(ui->cb_login, &QCheckBox::stateChanged, this, &MainWindow::updateInfoButton);

    connect(ui->b_connect, &QPushButton::clicked, this, &MainWindow::login);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::login()
{
    if (ui->le_login->text().isEmpty()) {
        ui->statusbar->showMessage("You have not entered the login!");
        return;
    }

    QString ip = ui->le_ip->text();
    QHostAddress addr;
    if (!addr.setAddress(ip)) {
        ui->statusbar->showMessage("Invalid IP address format!");
        return;
    }


}

void MainWindow::updateInfoButton()
{
    bool enabled = (ui->cb_ip->isChecked() ||
                    ui->cb_port->isChecked() ||
                    ui->cb_login->isChecked());
    ui->b_saveInfo->setEnabled(enabled);
}
