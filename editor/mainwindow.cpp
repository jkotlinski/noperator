#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "animation.h"
#include "animationplayer.h"
#include "sidplayer.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sidPlayer(new SidPlayer),
    animation(new Animation)
{
    ui->setupUi(this);
    animationPlayer = new AnimationPlayer(animation, ui->centralWidget);
    animationPlayer->start();
}

MainWindow::~MainWindow()
{
    delete animationPlayer;
    delete animation;
    delete sidPlayer;
    delete ui;
}
