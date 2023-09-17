#ifndef CREATEPROJECT_H
#define CREATEPROJECT_H

#include <QMainWindow>
#include "projectsettingwindow.h"

namespace Ui {
class CreateProjectWindow;
}

class CreateProjectWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CreateProjectWindow(QWidget *parent = nullptr);
    ~CreateProjectWindow();


private slots:
    void on_okButton_clicked();

private:
    Ui::CreateProjectWindow *ui;
    QMainWindow* mSecondWindow;
};

#endif // CREATEPROJECT_H
