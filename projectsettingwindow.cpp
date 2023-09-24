#include "projectsettingwindow.h"
#include "ui_projectsettingwindow.h"
#include "qtoolbutton.h"
#include "qtabbar.h"
#include <QThreadPool>
#include <QGraphicsScene>

#include <QFileDialog>
#include <QTimer>
#include <QMessageBox>
#include <animationaudio.h>
#include <createprojectwindow.h>
#include <QAudioOutput>
#include <QFileInfo>

QThreadPool *thread_pool = QThreadPool::globalInstance();

ProjectSettingWindow::ProjectSettingWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::projectsettingwindow),
    mCurrentFrame(0),
    mCurrentRow(0),
    mCurrentColumn(0),
    mTabChangeTries(0),
    mEnableUnselect(0)
{
    ui->setupUi(this);
}

ProjectSettingWindow::ProjectSettingWindow(qint32 rows, qint32 columns, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::projectsettingwindow),
    mRows(rows),
    mColumns(columns),
    mAnimation(new Animation(QSize(rows, columns))),
    mCurrentFrame(0),
    mCurrentRow(0),
    mCurrentColumn(0),
    mTabChangeTries(0),
    mEnableUnselect(0)
{
    ui->setupUi(this);

    initializeUI();
    mHelper = new Helper(mAnimation, rows, columns);
    mPaintingWidget = new PaintingWidget(mHelper, this);
    mAnimation->addFrame();

    ui->gridLayout->addWidget(mPaintingWidget,0,0);
    mPaintingWidget->drawBackground();

    mStopTimer = new QTimer(this);
    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, mPaintingWidget, &PaintingWidget::animate);
    mTimer->start(15);

    connect(mPaintingWidget, &PaintingWidget::selectedSignal, this, &ProjectSettingWindow::enableGroupBox);
    connect(ui->actionSave, &QAction::triggered, this, &ProjectSettingWindow::on_saveAction_triggered);
    connect(ui->actionImport, &QAction::triggered, this, &ProjectSettingWindow::on_importAction_triggered);
}

ProjectSettingWindow::ProjectSettingWindow(Animation * anim, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::projectsettingwindow),
    mCurrentFrame(0),
    mCurrentRow(0),
    mCurrentColumn(0),
    mTabChangeTries(0),
    mEnableUnselect(0)
{
    ui->setupUi(this);
    initialize(anim);
    mStopTimer = new QTimer(this);
    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, mPaintingWidget, &PaintingWidget::animate);
    mTimer->start(50);
    connect(mPaintingWidget, &PaintingWidget::selectedSignal, this, &ProjectSettingWindow::enableGroupBox);
    connect(ui->actionSave, &QAction::triggered, this, &ProjectSettingWindow::on_saveAction_triggered);
    connect(ui->actionImport, &QAction::triggered, this, &ProjectSettingWindow::on_importAction_triggered);
}

void ProjectSettingWindow::initializeUI()
{
    ui->tabWidget->clear();
    ui->tabWidget->insertTab(ui->tabWidget->count() - 1,new QLabel(), QString("frame 1"));
    QToolButton *tb = ui->addFrameToolButton;
    tb->setText("+");
    // Add empty, not enabled tab to tabWidget
    ui->tabWidget->addTab(new QLabel(), QString());
    ui->tabWidget->setTabEnabled(ui->tabWidget->count() - 1, false);
    ui->tabWidget->tabBar()->setTabButton(ui->tabWidget->count() - 1, QTabBar::LeftSide, tb);

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

}

void ProjectSettingWindow::initialize(Animation* animation)
{
    mRows = animation->getSize().width();
    mColumns = animation->getSize().height();

    mAnimation = animation;

    initializeUI();

    mHelper = new Helper(mAnimation, mRows, mColumns);
    mPaintingWidget = new PaintingWidget(mHelper, this);
    for (qint32 i = 0; i < mAnimation->getLen() - 1; i++)
    {
        ui->tabWidget->insertTab(ui->tabWidget->count() - 1,new QLabel(), QString("frame ") + QString::number(ui->tabWidget->count()));
    }

    ui->gridLayout->addWidget(mPaintingWidget,0,0);
    mPaintingWidget->drawBackground();
}

void ProjectSettingWindow::enableGroupBox()
{
    ui->groupBox->setEnabled(true);
    ui->amplitudeSpinBox->setValue
        (mAnimation->getAmplitude
                                   (QPoint(mHelper->getSelectedRow(), mHelper->getSelectedColumn())));
    ui->frequencySpinBox->setValue
        (mAnimation->getFrequency
                                   (QPoint(mHelper->getSelectedRow(), mHelper->getSelectedColumn())));
}

void ProjectSettingWindow::addFrame()
{
    ui->tabWidget->addTab(nullptr, QString("frame ") + QString::number(ui->tabWidget->count() - 1));
}

ProjectSettingWindow::~ProjectSettingWindow()
{
    delete ui;
}

void ProjectSettingWindow::on_addFrameToolButton_clicked()
{
    ui->tabWidget->insertTab(ui->tabWidget->count() - 1,new QLabel(), QString("frame ") + QString::number(ui->tabWidget->count()));
    mAnimation->addFrame();
}

void ProjectSettingWindow::on_playPushButton_clicked()
{
    if (!QFileInfo::exists("file.wav") && !QFileInfo("file.wav").isFile())
    {
        QMessageBox messageBox;
        messageBox.warning(0,"Error","please generate an audio file first.");
        return;
    }

    if (ui->frameRateComboBox->currentIndex() >= 0)
    {

        if (ui->playPushButton->text() != QString("Stop"))
        {
            startPlay();
        }
        else
        {
            stopPlay();
        }
    }
    else
    {
        QMessageBox messageBox;
        messageBox.warning(0,"Error","please select a correct frame rate");
    }
}

void ProjectSettingWindow::startPlay()
{
    ui->playPushButton->setText("Stop");

    mTimer->stop();
    mTimer->start(1000/ui->frameRateComboBox->currentText().toInt());
    ui->frameRateComboBox->setDisabled(true);

    connect(mStopTimer, &QTimer::timeout, this, &ProjectSettingWindow::stopPlay);
    mStopTimer->start(1000*ui->loopSlider->value());
    ui->loopSlider->setDisabled(true);
    ui->loopSpinBox->setDisabled(true);
    mHelper->startPlay();
}

void ProjectSettingWindow::stopPlay()
{
    ui->playPushButton->setText("Play");
    mHelper->stopPlay();
    mTimer->stop();
    mTimer->start(50);

    ui->frameRateComboBox->setEnabled(true);
    disconnect(mStopTimer, &QTimer::timeout, this, &ProjectSettingWindow::stopPlay);
    mStopTimer->stop();
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
    if (mHelper->isSelected())
    {
        mAnimation->setFrequency(QPoint(mHelper->getSelectedRow(), mHelper->getSelectedColumn()), arg1);
    }
//    frames[currentFrame].setFrequency(help->selectedRow, help->selectedColumn, arg1);
}

void ProjectSettingWindow::on_amplitudeSlider_valueChanged(qint32 value)
{
    ui->amplitudeSpinBox->setValue(value);
}

void ProjectSettingWindow::on_amplitudeSpinBox_valueChanged(qint32 arg1)
{
    ui->amplitudeSlider->setValue(arg1);
    if (mHelper->isSelected())
    {
        mAnimation->setAmplitude(QPoint(mHelper->getSelectedRow(), mHelper->getSelectedColumn()), arg1);
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
    if (index < mAnimation->getLen() && index >= 0)
    {
        mCurrentFrame = index;
        mAnimation->selectFrame(index);
        mTabChangeTries++;
        if (mTabChangeTries > 2)
        {
            mEnableUnselect = true;
        }

        if (mEnableUnselect)
        {
            mHelper->unselectCell();
            ui->groupBox->setDisabled(true);
        }
    }
}

void ProjectSettingWindow::on_previousPushButton_clicked()
{
    if (ui->tabWidget->currentIndex() > 0)
    {
        ui->tabWidget->setCurrentIndex(ui->tabWidget->currentIndex() - 1);
    }

}

void ProjectSettingWindow::on_nextPushButton_clicked()
{
    if (ui->tabWidget->currentIndex() < ui->tabWidget->count() - 2)
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
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save"), "",
        tr("CSV File (*.csv);;All Files (*)"));
    if (fileName.isEmpty())
    {
        return;
    }
    else
    {
        mAnimation->writeInFile(fileName);
    }
}

void ProjectSettingWindow::on_importAction_triggered(bool action)
{
    importFromFile();
}

void ProjectSettingWindow::importFromFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Import"), "",
        tr("CSV File (*.csv);;All Files (*)"));
    if (fileName.isEmpty())
    {
        return;
    }
    else
    {
        Animation * newPrj = new Animation(fileName);
        if (newPrj->getError())
        {
            qDebug() << "the animation did not work";
        }
        else
        {
           ProjectSettingWindow * newPS = new ProjectSettingWindow(newPrj);
           newPS->show();
           close();
        }
    }
}

void ProjectSettingWindow::on_duplicatePushButton_clicked()
{
    ui->tabWidget->insertTab(ui->tabWidget->count() - 1,new QLabel(), QString("frame ") + QString::number(ui->tabWidget->count()));
    mAnimation->duplicateCurrentFrame();
}

void ProjectSettingWindow::on_deletePushButton_clicked()
{
    if (ui->tabWidget->count() > 2)
    {
        ui->tabWidget->removeTab(ui->tabWidget->currentIndex());
        mAnimation->removeCurrentFrame();
    }
}

void ProjectSettingWindow::on_generateSounFileButton_clicked()
{
    if (ui->frameRateComboBox->currentIndex() < 0)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Please select frame rate value.");
        return;
    }

    AnimationAudio *audio = new AnimationAudio(mAnimation, ui->frameRateComboBox->currentText().toInt(), ui->loopSpinBox->value());
    QString fileName = "file.wav";
    audio->generateFile(fileName);
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
