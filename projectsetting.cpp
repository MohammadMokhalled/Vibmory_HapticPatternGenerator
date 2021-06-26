#include "projectsetting.h"
#include "ui_projectsetting.h"
#include "qtoolbutton.h"
#include "qtabbar.h"
#include <QThreadPool>
#include <QGraphicsScene>
#include "playinganimation.h"
#include <QFileDialog>
#include <QTimer>
#include <QMessageBox>
#include <animationaudio.h>
#include <createproject.h>
#include <QAudioOutput>
#include <QFileInfo>

QThreadPool *thread_pool = QThreadPool::globalInstance();

ProjectSetting::ProjectSetting(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::projectsetting)
{
    ui->setupUi(this);
}


ProjectSetting::ProjectSetting(bool test, int rows, int columns, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::projectsetting)
{
    ui->setupUi(this);
    //ui->messageLabel->hide();
    this->rows = rows;
    this->columns = columns;

    this->animation = new Animation(this->rows, this->columns);

    initializeUI();

    this->help = new Helper(animation, rows, columns);
    this->pw = new PaintingWidget(help, this);
    this->animation->addFrame();

    this->ui->gridLayout->addWidget(pw,0,0);
    this->pw->drawBackground();

    stopTimer = new QTimer(this);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, pw, &PaintingWidget::animate);
    timer->start(15);

    connect(pw, &PaintingWidget::selectedSignal, this, &ProjectSetting::enableGroupBox);
    connect(ui->actionSave, &QAction::triggered, this, &ProjectSetting::on_saveAction_triggered);
    connect(ui->actionImport, &QAction::triggered, this, &ProjectSetting::on_importAction_triggered);
}

ProjectSetting::ProjectSetting(Animation * anim, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::projectsetting)
{
    ui->setupUi(this);
    //ui->messageLabel->hide();
    initialize(anim);
    stopTimer = new QTimer(this);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, pw, &PaintingWidget::animate);
    timer->start(50);
    connect(pw, &PaintingWidget::selectedSignal, this, &ProjectSetting::enableGroupBox);
    connect(ui->actionSave, &QAction::triggered, this, &ProjectSetting::on_saveAction_triggered);
    connect(ui->actionImport, &QAction::triggered, this, &ProjectSetting::on_importAction_triggered);
}

void ProjectSetting::initializeUI()
{
    this->ui->tabWidget->clear();
    this->ui->tabWidget->insertTab(this->ui->tabWidget->count() - 1,new QLabel(), QString("frame 1"));
    QToolButton *tb = this->ui->addFrameToolButton;
    tb->setText("+");
    // Add empty, not enabled tab to tabWidget
    this->ui->tabWidget->addTab(new QLabel(), QString());
    this->ui->tabWidget->setTabEnabled(this->ui->tabWidget->count() - 1, false);
    this->ui->tabWidget->tabBar()->setTabButton(this->ui->tabWidget->count() - 1, QTabBar::LeftSide, tb);

    this->ui->frameRateComboBox->clear();
    this->ui->frameRateComboBox->addItem(QString("1"));
    this->ui->frameRateComboBox->addItem(QString("2"));
    this->ui->frameRateComboBox->addItem(QString("5"));
    this->ui->frameRateComboBox->addItem(QString("10"));
//    this->ui->frameRateComboBox->addItem(QString("15"));
    this->ui->frameRateComboBox->addItem(QString("20"));
    this->ui->frameRateComboBox->addItem(QString("25"));
    this->ui->frameRateComboBox->addItem(QString("40"));
    this->ui->frameRateComboBox->addItem(QString("50"));
//    this->ui->frameRateComboBox->addItem(QString("35"));

    this->ui->tabWidget->setCurrentIndex(0);

}

void ProjectSetting::initialize(Animation * anim)
{
    this->rows = anim->getRows();
    this->columns = anim->getColumns();

    this->animation = anim;

    initializeUI();

    this->help = new Helper(animation, rows, columns);
    this->pw = new PaintingWidget(help, this);
    for (int i = 0; i < animation->getLen() - 1; i++)
    {
        this->ui->tabWidget->insertTab(this->ui->tabWidget->count() - 1,new QLabel(), QString("frame ") + QString::number(this->ui->tabWidget->count()));
    }

    this->ui->gridLayout->addWidget(pw,0,0);
    this->pw->drawBackground();
}

void ProjectSetting::enableGroupBox()
{
    this->ui->groupBox->setEnabled(true);
    this->ui->amplitudeSpinBox->setValue
            (this->animation->getAmplitude
             (this->help->getSelectedRow(), this->help->getSelectedColumn()));
    this->ui->frequencySpinBox->setValue
            (this->animation->getFrequency
             (this->help->getSelectedRow(), this->help->getSelectedColumn()));
}

void ProjectSetting::addFrame()
{
    this->ui->tabWidget->addTab(nullptr, QString("frame ") + QString::number(this->ui->tabWidget->count() - 1));

}

ProjectSetting::~ProjectSetting()
{
    delete ui;
}


void ProjectSetting::on_addFrameToolButton_clicked()
{
    this->ui->tabWidget->insertTab(this->ui->tabWidget->count() - 1,new QLabel(), QString("frame ") + QString::number(this->ui->tabWidget->count()));
//    this->frames.append(Frame(columns, rows));
    this->animation->addFrame();
}

void ProjectSetting::on_playPushButton_clicked()
{
    //qDebug() << "index = " << ui->frameRateComboBox->currentIndex();

    if (!QFileInfo::exists("file.wav") && !QFileInfo("file.wav").isFile())
    {
        QMessageBox messageBox;
        messageBox.warning(0,"Error","please generate an audio file first.");
        return;
    }

    if (ui->frameRateComboBox->currentIndex() >= 0)
    {

        if (this->ui->playPushButton->text() != QString("Stop"))
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

void ProjectSetting::startPlay()
{


    this->ui->playPushButton->setText("Stop");

    this->timer->stop();
    this->timer->start(1000/ui->frameRateComboBox->currentText().toInt());
    this->ui->frameRateComboBox->setDisabled(true);

    connect(stopTimer, &QTimer::timeout, this, &ProjectSetting::stopPlay);
    stopTimer->start(1000*ui->loopSlider->value());
    this->ui->loopSlider->setDisabled(true);
    this->ui->loopSpinBox->setDisabled(true);
    this->help->startPlay();

}

void ProjectSetting::stopPlay()
{
    this->ui->playPushButton->setText("Play");
    this->help->stopPlay();
    this->timer->stop();
    this->timer->start(50);

    this->ui->frameRateComboBox->setEnabled(true);
    disconnect(stopTimer, &QTimer::timeout, this, &ProjectSetting::stopPlay);
    stopTimer->stop();
    this->ui->loopSlider->setEnabled(true);
    this->ui->loopSpinBox->setEnabled(true);

//    player->stop();

}

void ProjectSetting::on_frequencySlider_valueChanged(int value)
{
    this->ui->frequencySpinBox->setValue(value);
}

void ProjectSetting::on_frequencySpinBox_valueChanged(int arg1)
{
    this->ui->frequencySlider->setValue(arg1);
    if (help->isSelected())
    {
        this->animation->setFrequency(help->getSelectedRow(), help->getSelectedColumn(), arg1);
    }
//    this->frames[currentFrame].setFrequency(help->selectedRow, help->selectedColumn, arg1);
}


void ProjectSetting::on_amplitudeSlider_valueChanged(int value)
{
    this->ui->amplitudeSpinBox->setValue(value);
}

void ProjectSetting::on_amplitudeSpinBox_valueChanged(int arg1)
{
    this->ui->amplitudeSlider->setValue(arg1);
    //this->frames[currentFrame].setAmplitude(help->selectedRow, help->selectedColumn, arg1);
    if (help->isSelected())
    {
        this->animation->setAmplitude(help->getSelectedRow(), help->getSelectedColumn(), arg1);
    }
}

void ProjectSetting::on_loopSlider_valueChanged(int value)
{
    this->ui->loopSpinBox->setValue(value);
}

void ProjectSetting::on_loopSpinBox_valueChanged(int arg1)
{
    this->ui->loopSlider->setValue(arg1);
}


void ProjectSetting::on_tabWidget_currentChanged(int index)
{
    qDebug() << "len" << this->animation->getLen() << "  " << index << "\n";

    if (index < this->animation->getLen() && index >= 0)
    {
        //help->setCurrentFrame(&frames[index]);
        currentFrame = index;
        this->animation->selectFrame(index);
        qDebug() << "current " << index << "\n";
        tabChangeTries++;
        if (tabChangeTries > 2)
            enableUnselect = true;

        if (enableUnselect)
        {
            help->unselectCell();
            this->ui->groupBox->setDisabled(true);
        }

    }

}

void ProjectSetting::on_previousPushButton_clicked()
{
    if (ui->tabWidget->currentIndex() > 0)
    {
        ui->tabWidget->setCurrentIndex(ui->tabWidget->currentIndex() - 1);
    }

}

void ProjectSetting::on_nextPushButton_clicked()
{
    if (ui->tabWidget->currentIndex() < ui->tabWidget->count() - 2)
    {
        ui->tabWidget->setCurrentIndex(ui->tabWidget->currentIndex() + 1);
    }
}

void ProjectSetting::on_saveAction_triggered(bool action)
{
    saveToFile();
}

void ProjectSetting::saveToFile()
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
        animation->writeInFile(fileName);
    }
}

void ProjectSetting::on_importAction_triggered(bool action)
{
    importFromFile();
}

void ProjectSetting::importFromFile()
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
           ProjectSetting * newPS = new ProjectSetting(newPrj);
           newPS->show();
           this->close();
        }
    }
}

void ProjectSetting::on_duplicatePushButton_clicked()
{
    this->ui->tabWidget->insertTab(this->ui->tabWidget->count() - 1,new QLabel(), QString("frame ") + QString::number(this->ui->tabWidget->count()));
    animation->duplicateCurrentFrame();
}

void ProjectSetting::on_deletePushButton_clicked()
{
    if (ui->tabWidget->count() > 2)
    {
        ui->tabWidget->removeTab(ui->tabWidget->currentIndex());
        animation->removeCurrentFrame();
    }
}


void ProjectSetting::on_generateSounFileButton_clicked()
{
    if (ui->frameRateComboBox->currentIndex() < 0)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Please select frame rate value.");
        return;
    }

    //ui->messageLabel->show();
    //while(ui->messageLabel->isHidden());
    AnimationAudio *audio = new AnimationAudio(animation, ui->frameRateComboBox->currentText().toInt(), ui->loopSpinBox->value());
    audio->generateFile("file.wav");
   // ui->messageLabel->hide();
}

void ProjectSetting::on_actionNew_Project_triggered()
{
    CreateProject *win = new CreateProject;
    win->show();
    this->close();

}



void ProjectSetting::on_maxFrequencyPushButton_clicked()
{

    ui->frequencySpinBox->setValue(ui->frequencySpinBox->maximum());

}

void ProjectSetting::on_maxAmplitudePushButton_clicked()
{

    ui->amplitudeSpinBox->setValue(ui->amplitudeSpinBox->maximum());
}

void ProjectSetting::on_minFrequencyPushButton_clicked()
{
    ui->frequencySpinBox->setValue(ui->frequencySpinBox->minimum());
}


void ProjectSetting::on_minAmplitudePushButton_clicked()
{
    ui->amplitudeSpinBox->setValue(ui->amplitudeSpinBox->minimum());
}
