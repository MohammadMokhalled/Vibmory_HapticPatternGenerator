#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <exception>
#include "createprojectwindow.h"
#include <QDebug>
#include <QMessageBox>

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
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Save"), "",
        tr("CSV File (*.csv);;All Files (*)"));
    if (fileName.isEmpty())
    {
        return;
    }
    else
    {
        try
        {
            mSecondWindow = new ProjectSettingWindow(fileName, this);
            close();
            mSecondWindow->show();
        }
        catch (std::exception& e)
        {
            QMessageBox::critical(nullptr, "Error", e.what());
            return;
        }
    }
}
