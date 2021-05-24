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

QThreadPool *thread_pool = QThreadPool::globalInstance();

projectsetting::projectsetting(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::projectsetting)
{
    ui->setupUi(this);
}


projectsetting::projectsetting(bool test, int rows, int columns, QWidget *parent) :
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

    connect(pw, &PaintingWidget::selectedSignal, this, &projectsetting::enableGroupBox);
    connect(ui->actionSave, &QAction::triggered, this, &projectsetting::on_saveAction_triggered);
    connect(ui->actionImport, &QAction::triggered, this, &projectsetting::on_importAction_triggered);
}

projectsetting::projectsetting(Animation * anim, QWidget *parent) :
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
    connect(pw, &PaintingWidget::selectedSignal, this, &projectsetting::enableGroupBox);
    connect(ui->actionSave, &QAction::triggered, this, &projectsetting::on_saveAction_triggered);
    connect(ui->actionImport, &QAction::triggered, this, &projectsetting::on_importAction_triggered);
}

void projectsetting::initializeUI()
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
    this->ui->frameRateComboBox->addItem(QString("10"));
    this->ui->frameRateComboBox->addItem(QString("15"));
    this->ui->frameRateComboBox->addItem(QString("20"));
    this->ui->frameRateComboBox->addItem(QString("25"));
    this->ui->frameRateComboBox->addItem(QString("30"));
    this->ui->frameRateComboBox->addItem(QString("35"));

    this->ui->tabWidget->setCurrentIndex(0);

}

void projectsetting::initialize(Animation * anim)
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

void projectsetting::enableGroupBox()
{
    this->ui->groupBox->setEnabled(true);
    this->ui->amplitudeSpinBox->setValue
            (this->animation->getAmplitude
             (this->help->getSelectedRow(), this->help->getSelectedColumn()));
    this->ui->frequencySpinBox->setValue
            (this->animation->getFrequency
             (this->help->getSelectedRow(), this->help->getSelectedColumn()));
}

void projectsetting::addFrame()
{
    this->ui->tabWidget->addTab(nullptr, QString("frame ") + QString::number(this->ui->tabWidget->count() - 1));

}

projectsetting::~projectsetting()
{
    delete ui;
}


void projectsetting::on_addFrameToolButton_clicked()
{
    this->ui->tabWidget->insertTab(this->ui->tabWidget->count() - 1,new QLabel(), QString("frame ") + QString::number(this->ui->tabWidget->count()));
//    this->frames.append(Frame(columns, rows));
    this->animation->addFrame();
}

void projectsetting::on_playPushButton_clicked()
{
    qDebug() << "index = " << ui->frameRateComboBox->currentIndex();
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

void projectsetting::startPlay()
{
    this->ui->playPushButton->setText("Stop");

    this->timer->stop();
    this->timer->start(1000/ui->frameRateComboBox->currentText().toInt());
    this->ui->frameRateComboBox->setDisabled(true);

    connect(stopTimer, &QTimer::timeout, this, &projectsetting::stopPlay);
    stopTimer->start(1000*ui->loopSlider->value());
    this->ui->loopSlider->setDisabled(true);
    this->ui->loopSpinBox->setDisabled(true);
    this->help->startPlay();
}

void projectsetting::stopPlay()
{
    this->ui->playPushButton->setText("Play");
    this->help->stopPlay();
    this->timer->stop();
    this->timer->start(50);

    this->ui->frameRateComboBox->setEnabled(true);
    disconnect(stopTimer, &QTimer::timeout, this, &projectsetting::stopPlay);
    stopTimer->stop();
    this->ui->loopSlider->setEnabled(true);
    this->ui->loopSpinBox->setEnabled(true);

}

void projectsetting::on_frequencySlider_valueChanged(int value)
{
    this->ui->frequencySpinBox->setValue(value);
}

void projectsetting::on_frequencySpinBox_valueChanged(int arg1)
{
    this->ui->frequencySlider->setValue(arg1);
    if (help->isSelected())
    {
        this->animation->setFrequency(help->getSelectedRow(), help->getSelectedColumn(), arg1);
    }
//    this->frames[currentFrame].setFrequency(help->selectedRow, help->selectedColumn, arg1);
}


void projectsetting::on_amplitudeSlider_valueChanged(int value)
{
    this->ui->amplitudeSpinBox->setValue(value);
}

void projectsetting::on_amplitudeSpinBox_valueChanged(int arg1)
{
    this->ui->amplitudeSlider->setValue(arg1);
    //this->frames[currentFrame].setAmplitude(help->selectedRow, help->selectedColumn, arg1);
    if (help->isSelected())
    {
        this->animation->setAmplitude(help->getSelectedRow(), help->getSelectedColumn(), arg1);
    }
}

void projectsetting::on_loopSlider_valueChanged(int value)
{
    this->ui->loopSpinBox->setValue(value);
}

void projectsetting::on_loopSpinBox_valueChanged(int arg1)
{
    this->ui->loopSlider->setValue(arg1);
}


void projectsetting::on_tabWidget_currentChanged(int index)
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

void projectsetting::on_previousPushButton_clicked()
{
    if (ui->tabWidget->currentIndex() > 0)
    {
        ui->tabWidget->setCurrentIndex(ui->tabWidget->currentIndex() - 1);
    }

}

void projectsetting::on_nextPushButton_clicked()
{
    if (ui->tabWidget->currentIndex() < ui->tabWidget->count() - 2)
    {
        ui->tabWidget->setCurrentIndex(ui->tabWidget->currentIndex() + 1);
    }
}

void projectsetting::on_saveAction_triggered(bool action)
{
    saveToFile();
}

void projectsetting::saveToFile()
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

void projectsetting::on_importAction_triggered(bool action)
{
    importFromFile();
}

void projectsetting::importFromFile()
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
               projectsetting * newPS = new projectsetting(newPrj);
               newPS->show();
               this->close();
            }



    }
}

void projectsetting::on_duplicatePushButton_clicked()
{
    this->ui->tabWidget->insertTab(this->ui->tabWidget->count() - 1,new QLabel(), QString("frame ") + QString::number(this->ui->tabWidget->count()));
    animation->duplicateCurrentFrame();
}

void projectsetting::on_deletePushButton_clicked()
{
    if (ui->tabWidget->count() > 2)
    {
        ui->tabWidget->removeTab(ui->tabWidget->currentIndex());
        animation->removeCurrentFrame();
    }
}

void projectsetting::on_onPushButton_clicked()
{

}

void projectsetting::on_generateSounFileButton_clicked()
{
    if (ui->frameRateComboBox->currentIndex() < 0)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Please select frame rate value.");
        return;
    }

    ui->messageLabel->show();
    while(ui->messageLabel->isHidden());
    AnimationAudio *audio = new AnimationAudio(animation, ui->frameRateComboBox->currentText().toInt(), ui->loopSpinBox->value());
    audio->generateFile("file.wav");
    ui->messageLabel->hide();
}
