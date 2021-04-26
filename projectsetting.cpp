#include "projectsetting.h"
#include "ui_projectsetting.h"
#include "qtoolbutton.h"
#include "qtabbar.h"
#include <QThreadPool>
#include <QGraphicsScene>
#include "playinganimation.h"
#include <QFileDialog>

#include <QTimer>

QThreadPool *thread_pool = QThreadPool::globalInstance();

projectsetting::projectsetting(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::projectsetting)
{
    ui->setupUi(this);
}

projectsetting::projectsetting(int rows, int columns, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::projectsetting)
{
    ui->setupUi(this);

    this->rows = rows;
    this->columns = columns;

    this->graphicViews = new QGraphicsView[this->rows * this->columns];


    for (int i = 0; i < this->rows; i++)
    {
        for (int j = 0; j < this->columns; j++)
        {
            this->ui->gridLayout->addWidget(dynamic_cast<QWidget *>(&this->graphicViews[(i* this->columns) + j]), i, j);

            this->graphicViews[(i* this->columns) + j].setScene(
                        new QGraphicsScene(0,0,1, 1));
            this->graphicViews[(i* this->columns) + j].setBackgroundBrush(QBrush(QColor(255,255,255),Qt::SolidPattern));

        }
    }

    this->ui->tabWidget->clear();
    this->ui->tabWidget->insertTab(this->ui->tabWidget->count() - 1,new QLabel(), QString("frame 1"));
    QToolButton *tb = this->ui->addFrameToolButton;
    tb->setText("+");
    // Add empty, not enabled tab to tabWidget
    this->ui->tabWidget->addTab(new QLabel(), QString());
    this->ui->tabWidget->setTabEnabled(this->ui->tabWidget->count() - 1, false);
    this->ui->tabWidget->tabBar()->setTabButton(this->ui->tabWidget->count() - 1, QTabBar::LeftSide, tb);

    this->ui->frameRateComboBox->addItem(QString("10"));
    this->ui->frameRateComboBox->addItem(QString("15"));
    this->ui->frameRateComboBox->addItem(QString("20"));
    this->ui->frameRateComboBox->addItem(QString("25"));
    this->ui->frameRateComboBox->addItem(QString("30"));
    this->ui->frameRateComboBox->addItem(QString("35"));
}

projectsetting::projectsetting(bool test, int rows, int columns, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::projectsetting)
{
    ui->setupUi(this);

    this->rows = rows;
    this->columns = columns;

    this->animation = new Animation(this->rows, this->columns);
    this->animation->addFrame();

    this->ui->tabWidget->clear();
    this->ui->tabWidget->insertTab(this->ui->tabWidget->count() - 1,new QLabel(), QString("frame 1"));
    QToolButton *tb = this->ui->addFrameToolButton;
    tb->setText("+");
    // Add empty, not enabled tab to tabWidget
    this->ui->tabWidget->addTab(new QLabel(), QString());
    this->ui->tabWidget->setTabEnabled(this->ui->tabWidget->count() - 1, false);
    this->ui->tabWidget->tabBar()->setTabButton(this->ui->tabWidget->count() - 1, QTabBar::LeftSide, tb);

    this->ui->frameRateComboBox->addItem(QString("10"));
    this->ui->frameRateComboBox->addItem(QString("15"));
    this->ui->frameRateComboBox->addItem(QString("20"));
    this->ui->frameRateComboBox->addItem(QString("25"));
    this->ui->frameRateComboBox->addItem(QString("30"));
    this->ui->frameRateComboBox->addItem(QString("35"));



    this->help = new Helper(animation, rows, columns);
    this->pw = new PaintingWidget(help, this);

    this->ui->gridLayout->addWidget(pw,0,0);
    this->pw->drawBackground();
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, pw, &PaintingWidget::animate);
    timer->start(15);

    connect(pw, &PaintingWidget::selectedSignal, this, &projectsetting::enableGroupBox);
    //connect(help, SIGNAL(Helper::selectedSignal()), this, SLOT(projectsetting::enableGroupBox()));

    //help->setCurrentFrame(&frames[0]);
    //help->currentFrame = &frames[0];
}

projectsetting::projectsetting(Animation * anim, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::projectsetting)
{
    ui->setupUi(this);

    this->rows = anim->getRows();
    this->columns = anim->getColumns();

    this->animation = anim;

    this->ui->tabWidget->clear();
    this->ui->tabWidget->insertTab(this->ui->tabWidget->count() - 1,new QLabel(), QString("frame 1"));

    QToolButton *tb = this->ui->addFrameToolButton;
    tb->setText("+");
    // Add empty, not enabled tab to tabWidget
    this->ui->tabWidget->addTab(new QLabel(), QString());
    this->ui->tabWidget->setTabEnabled(this->ui->tabWidget->count() - 1, false);
    this->ui->tabWidget->tabBar()->setTabButton(this->ui->tabWidget->count() - 1, QTabBar::LeftSide, tb);

    this->ui->frameRateComboBox->addItem(QString("10"));
    this->ui->frameRateComboBox->addItem(QString("15"));
    this->ui->frameRateComboBox->addItem(QString("20"));
    this->ui->frameRateComboBox->addItem(QString("25"));
    this->ui->frameRateComboBox->addItem(QString("30"));
    this->ui->frameRateComboBox->addItem(QString("35"));

    for (int i = 0; i < animation->getLen() - 1; i++)
    {
        this->ui->tabWidget->insertTab(this->ui->tabWidget->count() - 1,new QLabel(), QString("frame ") + QString::number(this->ui->tabWidget->count()));
    }



    this->help = new Helper(animation, rows, columns);
    this->pw = new PaintingWidget(help, this);

    this->ui->gridLayout->addWidget(pw,0,0);
    this->pw->drawBackground();
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, pw, &PaintingWidget::animate);
    timer->start(50);

    connect(pw, &PaintingWidget::selectedSignal, this, &projectsetting::enableGroupBox);
    this->ui->tabWidget->setCurrentIndex(0);

    connect(ui->actionSave, &QAction::triggered, this, &projectsetting::on_saveAction_triggered);

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
    //PlayingAnimation * play  = new PlayingAnimation(this);
    //play->run();
    //qDebug() << animation->getFrameString();
    //animation->writeInFile("C:/TEMP/test.csv");
    if (this->ui->playPushButton->text() != QString("Stop"))
    {
        this->ui->playPushButton->setText("Stop");
        this->help->startPlay();
    }
    else
    {
        this->ui->playPushButton->setText("Play");
        this->help->stopPlay();
    }


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
    Frame * a = new Frame("18672-47, 0-0, 0-0, 2000-80, \n0-0, 0-0, 0-0, 0-0, \n0-0, 0-0, 0-0, 12941-249, \n0-0, 0-0, 0-0, 0-0, \n0-0, 0-0, 0-0, 0-0, \n", 4, 5);
    qDebug() << a->toString();

}

void projectsetting::on_nextPushButton_clicked()
{
    animation->writeInFile("C:/TEMP/test123.csv");
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
