#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "animation.h"
#include <QFileDialog>
#include <exception>
#include "createprojectwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mSecondWindow(nullptr)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_newProjectButton_clicked()
{
    mSecondWindow = new CreateProjectWindow;
    close();
    mSecondWindow->show();
}

void MainWindow::on_uploadButton_clicked()
{
    Animation * animation;
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Save"), "",
        tr("CSV File (*.csv);;All Files (*)"));
    if (fileName.isEmpty())
    {
        return;
    }
    else
    {
        animation = new Animation(fileName);
        if (animation->getError())
        {
            qDebug() << "the animation did not work";
        }
        else
        {
            mSecondWindow = new ProjectSettingWindow(animation, this);
            close();
            mSecondWindow->show();
        }
    }
}
