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
    ProjectSetting *prj = new ProjectSetting(true, this->ui->rowsSpinBox->value(), this->ui->columnsSpinBox->value());
    this->hide();
    prj->show();
}
