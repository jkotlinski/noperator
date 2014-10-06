#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "sidplayer.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sidPlayer(new SidPlayer)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
