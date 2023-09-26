#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

/**
 * @class MainWindow
 * 
 * The MainWindow class represents the main window of the application.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * Constructs a MainWindow object.
     * 
     * @param parent The parent QWidget object.
     */
    MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

private slots:
    /**
     * This function is called when the new project button is clicked.
     * It handles the logic for creating a new project.
     */
    void on_newProjectButton_clicked();

    /**
     * This function is called when the upload button is clicked.
     */
    void on_uploadButton_clicked();

private:
    Ui::MainWindow *ui;
    QMainWindow *mSecondWindow;
};
#endif // MAINWINDOW_H
