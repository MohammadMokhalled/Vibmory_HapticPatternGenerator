#include "createproject.h"
#include "ui_createproject.h"

CreateProject::CreateProject(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CreateProject)
{
    ui->setupUi(this);
}

CreateProject::~CreateProject()
{
    delete ui;
}



void CreateProject::on_okButton_clicked()
{
    ProjectSetting *prj = new ProjectSetting(true, this->ui->rowsSpinBox->value(), this->ui->columnsSpinBox->value());
    this->hide();
    prj->show();
}
