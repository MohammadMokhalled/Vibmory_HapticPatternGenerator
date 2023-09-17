#include "createprojectwindow.h"
#include "ui_createproject.h"

CreateProjectWindow::CreateProjectWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CreateProject)
{
    ui->setupUi(this);
}

CreateProjectWindow::~CreateProjectWindow()
{
    delete ui;
}

void CreateProjectWindow::on_okButton_clicked()
{
    mSecondWindow  = new ProjectSettingWindow(ui->rowsSpinBox->value(), ui->columnsSpinBox->value());
    close();
    mSecondWindow->show();
}
