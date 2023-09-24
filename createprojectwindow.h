#ifndef CREATEPROJECT_H
#define CREATEPROJECT_H

#include <QMainWindow>
#include "projectsettingwindow.h"

namespace Ui
{
    class CreateProjectWindow;
}


/**
 * @class CreateProjectWindow
 * 
 * @brief A class representing the Create Project Window.
 *
 * This class inherits from QMainWindow and provides functionality for creating a new project.
 */
class CreateProjectWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for the CreateProjectWindow class.
     *
     * @param parent The parent widget.
     */
    explicit CreateProjectWindow(QWidget *parent = nullptr);

    ~CreateProjectWindow();

private slots:
    /**
     * @brief This function is called when the okButton is clicked.
     *
     * @details This function handles the click event of the okButton.
     * It performs the necessary actions when the button is clicked.
     */
    void on_okButton_clicked();

private:
    Ui::CreateProjectWindow *ui;
    QMainWindow *mSecondWindow;
};

#endif // CREATEPROJECT_H
