#ifndef CREATEPROJECT_H
#define CREATEPROJECT_H

#include <QMainWindow>
#include "projectsetting.h"

namespace Ui {
class CreateProject;
}

class CreateProject : public QMainWindow
{
    Q_OBJECT

public:
    explicit CreateProject(QWidget *parent = nullptr);
    ~CreateProject();


private slots:
    void on_okButton_clicked();

private:
    Ui::CreateProject *ui;
};

#endif // CREATEPROJECT_H
