#include "projectsettingwindow.h"
#include "ui_projectsettingwindow.h"
#include "qtoolbutton.h"
#include "qtabbar.h"
#include <QGraphicsScene>
#include <QFileDialog>
#include <QMessageBox>
#include <createprojectwindow.h>
#include <QFileInfo>

ProjectSettingWindow::ProjectSettingWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::projectsettingwindow), mCurrentFrame(0),
      mCurrentCell(QPoint(0, 0)), mTabChangeTries(0), mEnableUnselect(0)
{
    ui->setupUi(this);
}

ProjectSettingWindow::ProjectSettingWindow(const QSize &size, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::projectsettingwindow), mSize(size),
      mAnimationController(new AnimationController(mSize)), mCurrentFrame(0),
      mCurrentCell(QPoint(0, 0)), mEnableUnselect(0)
{
    ui->setupUi(this);

    initialize();
}

ProjectSettingWindow::ProjectSettingWindow(const QString &fileName, QWidget *parent)
    : mAnimationController(new AnimationController(fileName)),
      QMainWindow(parent), ui(new Ui::projectsettingwindow),
      mSize(mAnimationController->getSize()), mCurrentFrame(0),
      mCurrentCell(QPoint(0, 0)), mTabChangeTries(0), mEnableUnselect(0)
{
    ui->setupUi(this);
    initialize();
}

void ProjectSettingWindow::initializeUI()
{
    ui->tabWidget->clear();
    ui->tabWidget->insertTab(ui->tabWidget->count() - 1, new QLabel(),
                             QString("frame 1"));
    QToolButton *tb = ui->addFrameToolButton;
    tb->setText("+");
    // Add empty, not enabled tab to tabWidget
    ui->tabWidget->addTab(new QLabel(), QString());
    ui->tabWidget->setTabEnabled(ui->tabWidget->count() - 1, false);
    ui->tabWidget->tabBar()->setTabButton(ui->tabWidget->count() - 1,
                                          QTabBar::LeftSide, tb);

    ui->frameRateComboBox->clear();
    ui->frameRateComboBox->addItem(QString("1"));
    ui->frameRateComboBox->addItem(QString("2"));
    ui->frameRateComboBox->addItem(QString("5"));
    ui->frameRateComboBox->addItem(QString("10"));
    ui->frameRateComboBox->addItem(QString("20"));
    ui->frameRateComboBox->addItem(QString("25"));
    ui->frameRateComboBox->addItem(QString("40"));
    ui->frameRateComboBox->addItem(QString("50"));

    ui->tabWidget->setCurrentIndex(0);

    // The first frame tab is already created up here this is why mAnimationController->getLen()
    for(qint32 i = 0; i < mAnimationController->getLen() - 1; i++)
    {
        ui->tabWidget->insertTab(ui->tabWidget->count() - 1, new QLabel(),
                                 QString("frame ") +
                                     QString::number(ui->tabWidget->count()));
    }

    ui->gridLayout->addWidget(mAnimationController->getPaintingWidget(), 0, 0);
    mAnimationController->drawBackground();
}

void ProjectSettingWindow::initializeSlots()
{
    connect(mAnimationController, &AnimationController::selectedSignal, this,
            &ProjectSettingWindow::enableGroupBox);
    connect(ui->actionSave, &QAction::triggered, this,
            &ProjectSettingWindow::on_saveAction_triggered);
    connect(ui->actionImport, &QAction::triggered, this,
            &ProjectSettingWindow::on_importAction_triggered);
    connect(mAnimationController, &AnimationController::stopAnimationSignal, this,
        			&ProjectSettingWindow::stopPlay);
}

void ProjectSettingWindow::initialize()
{
    initializeUI();
    initializeSlots();
}

void ProjectSettingWindow::enableGroupBox()
{
    ui->groupBox->setEnabled(true);
    ui->amplitudeSpinBox->setValue(
        mAnimationController->getAmplitude());
    ui->frequencySpinBox->setValue( 
        mAnimationController->getFrequency());
}

void ProjectSettingWindow::addFrame()
{
    ui->tabWidget->addTab(nullptr,
                          QString("frame ") +
                              QString::number(ui->tabWidget->count() - 1));
}

ProjectSettingWindow::~ProjectSettingWindow()
{
    delete ui;
}

void ProjectSettingWindow::on_addFrameToolButton_clicked()
{
    ui->tabWidget->insertTab(ui->tabWidget->count() - 1, new QLabel(),
                             QString("frame ") +
                                 QString::number(ui->tabWidget->count()));
    mAnimationController->addFrame();
}

void ProjectSettingWindow::on_playPushButton_clicked()
{
    if(!QFileInfo::exists("file.wav") && !QFileInfo("file.wav").isFile())
    {
        QMessageBox messageBox;
        messageBox.warning(0, "Error", "please generate an audio file first.");
        return;
    }

    if(ui->frameRateComboBox->currentIndex() >= 0)
    {

        if(ui->playPushButton->text() != QString("Stop"))
        {
            startPlay();
        }
        else
        {
            mAnimationController->stopAnimation();
            stopPlay();
        }
    }
    else
    {
        QMessageBox messageBox;
        messageBox.warning(0, "Error", "please select a correct frame rate");
    }
}

void ProjectSettingWindow::startPlay()
{
    ui->playPushButton->setText("Stop");
    ui->frameRateComboBox->setDisabled(true);
    ui->loopSlider->setDisabled(true);
    ui->loopSpinBox->setDisabled(true);
    mAnimationController->playAnimation(
        ui->frameRateComboBox->currentText().toInt(), ui->loopSpinBox->value());
}

void ProjectSettingWindow::stopPlay()
{
    ui->playPushButton->setText("Play");
    ui->frameRateComboBox->setEnabled(true);
    ui->loopSlider->setEnabled(true);
    ui->loopSpinBox->setEnabled(true);
}

void ProjectSettingWindow::on_frequencySlider_valueChanged(qint32 value)
{
    ui->frequencySpinBox->setValue(value);
}

void ProjectSettingWindow::on_frequencySpinBox_valueChanged(qint32 arg1)
{
    ui->frequencySlider->setValue(arg1);
    if(mAnimationController->isSelected())
    {
        mAnimationController->setFrequency(arg1);
    }
}

void ProjectSettingWindow::on_amplitudeSlider_valueChanged(qint32 value)
{
    ui->amplitudeSpinBox->setValue(value);
}

void ProjectSettingWindow::on_amplitudeSpinBox_valueChanged(qint32 arg1)
{
    ui->amplitudeSlider->setValue(arg1);
    if(mAnimationController->isSelected())
    {
        mAnimationController->setAmplitude(
            arg1);
    }
}

void ProjectSettingWindow::on_loopSlider_valueChanged(qint32 value)
{
    ui->loopSpinBox->setValue(value);
}

void ProjectSettingWindow::on_loopSpinBox_valueChanged(qint32 arg1)
{
    ui->loopSlider->setValue(arg1);
}

void ProjectSettingWindow::on_tabWidget_currentChanged(qint32 index)
{
    if(index < mAnimationController->getLen() && index >= 0)
    {
        mCurrentFrame = index;
        mAnimationController->selectFrame(index);
        mTabChangeTries++;
        if(mTabChangeTries > 2)
        {
            mEnableUnselect = true;
        }

        if(mEnableUnselect)
        {
            mAnimationController->unselect();
            ui->groupBox->setDisabled(true);
        }
    }
}

void ProjectSettingWindow::on_previousPushButton_clicked()
{
    if(ui->tabWidget->currentIndex() > 0)
    {
        ui->tabWidget->setCurrentIndex(ui->tabWidget->currentIndex() - 1);
    }
}

void ProjectSettingWindow::on_nextPushButton_clicked()
{
    if(ui->tabWidget->currentIndex() < ui->tabWidget->count() - 2)
    {
        ui->tabWidget->setCurrentIndex(ui->tabWidget->currentIndex() + 1);
    }
}

void ProjectSettingWindow::on_saveAction_triggered(bool action)
{
    saveToFile();
}

void ProjectSettingWindow::saveToFile()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save"), "", tr("CSV File (*.csv);;All Files (*)"));
    if(fileName.isEmpty())
    {
        return;
    }
    else
    {
        mAnimationController->saveToFile(fileName);
    }
}

void ProjectSettingWindow::on_importAction_triggered(bool action)
{
    importFromFile();
}

void ProjectSettingWindow::importFromFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Import"), "", tr("CSV File (*.csv);;All Files (*)"));
    if(fileName.isEmpty())
    {
        return;
    }
    else
    {
        try
        {
            ProjectSettingWindow *newPS = new ProjectSettingWindow(fileName);
            newPS->show();
            close();
        }
        catch(std::exception &e)
        {
            QMessageBox::critical(nullptr, "Error", e.what());
        }
    }
}

void ProjectSettingWindow::on_duplicatePushButton_clicked()
{
    ui->tabWidget->insertTab(ui->tabWidget->count() - 1, new QLabel(),
                             QString("frame ") +
                                 QString::number(ui->tabWidget->count()));
    mAnimationController->duplicateCurrentFrame();
}

void ProjectSettingWindow::on_deletePushButton_clicked()
{
    if(ui->tabWidget->count() > 2)
    {
        ui->tabWidget->removeTab(ui->tabWidget->currentIndex());
        mAnimationController->removeFrame();
    }
}

void ProjectSettingWindow::on_generateSounFileButton_clicked()
{
    if(ui->frameRateComboBox->currentIndex() < 0)
    {
        QMessageBox messageBox;
        messageBox.critical(0, "Error", "Please select frame rate value.");
        return;
    }

    mAnimationController->setFrameRate(
        ui->frameRateComboBox->currentText().toInt());
    mAnimationController->setDuration(ui->loopSpinBox->value());
    auto filename = QString("file.wav");
    mAnimationController->prepareActuator(filename);
}

void ProjectSettingWindow::on_actionNew_Project_triggered()
{
    CreateProjectWindow *win = new CreateProjectWindow;
    win->show();
    close();
}

void ProjectSettingWindow::on_maxFrequencyPushButton_clicked()
{
    ui->frequencySpinBox->setValue(ui->frequencySpinBox->maximum());
}

void ProjectSettingWindow::on_maxAmplitudePushButton_clicked()
{
    ui->amplitudeSpinBox->setValue(ui->amplitudeSpinBox->maximum());
}

void ProjectSettingWindow::on_minFrequencyPushButton_clicked()
{
    ui->frequencySpinBox->setValue(ui->frequencySpinBox->minimum());
}

void ProjectSettingWindow::on_minAmplitudePushButton_clicked()
{
    ui->amplitudeSpinBox->setValue(ui->amplitudeSpinBox->minimum());
}
