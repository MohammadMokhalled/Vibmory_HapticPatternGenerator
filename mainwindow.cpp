#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "animation.h"
#include <QFileDialog>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_newProjectButton_clicked()
{
    CreateProject *win = new CreateProject;
    this->hide();
    win->show();
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
        projectsetting *prj = new projectsetting(animation);
        this->hide();
        prj->show();
    }
//    Animation * anim = new Animation("C:/TEMP/test123.csv");
}
