#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class Animation;
class AnimationPlayer;
class SidPlayer;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    SidPlayer *sidPlayer;
    Animation *animation;
    AnimationPlayer *animationPlayer;
};

#endif // MAINWINDOW_H
