#include "miscsettingswidget.h"
#include "ui_miscsettingswidget.h"
#include "globals.h"
#include <QColorDialog>


MiscSettingsWidget::MiscSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MiscSettingsWidget)
{
    ui->setupUi(this);

    ui->spinCameraSpeed->setValue(DEFAULT_CAMERA_SPEED);
    ui->spinFovY->setValue(DEFAULT_CAMERA_FOVY);

    connect(ui->spinCameraSpeed, SIGNAL(valueChanged(double)), this, SLOT(onCameraSpeedChanged(double)));
    connect(ui->spinFovY, SIGNAL(valueChanged(double)), this, SLOT(onCameraFovYChanged(double)));
    connect(ui->buttonBackgroundColor, SIGNAL(clicked()), this, SLOT(onBackgroundColorClicked()));
    connect(ui->checkBoxGrid, SIGNAL(clicked()), this, SLOT(onGridCheckBoxClicked()));
    connect(ui->checkBoxLightSources, SIGNAL(clicked()), this, SLOT(onVisualHintChanged()));
    connect(ui->checkBoxSelectionOutline, SIGNAL(clicked()), this, SLOT(onSelectionOutlineClicked()));
    connect(ui->checkBoxBloom, SIGNAL(clicked()), this, SLOT(onBloomClicked()));
    connect(ui->checkBoxSSAO, SIGNAL(clicked()), this, SLOT(onSSAOClicked()));

}

MiscSettingsWidget::~MiscSettingsWidget()
{
    delete ui;
}

void MiscSettingsWidget::onCameraSpeedChanged(double speed)
{
    camera->speed = speed;
}

void MiscSettingsWidget::onCameraFovYChanged(double fovy)
{
    camera->fovy = fovy;
    emit settingsChanged();
}

int g_MaxSubmeshes = 100;

void MiscSettingsWidget::onMaxSubmeshesChanged(int n)
{
    g_MaxSubmeshes = n;
    emit settingsChanged();
}

void MiscSettingsWidget::onBackgroundColorClicked()
{
    QColor color = QColorDialog::getColor(miscSettings->backgroundColor, this, "Background color");
    if (color.isValid())
    {
        QString colorName = color.name();
        ui->buttonBackgroundColor->setStyleSheet(QString::fromLatin1("background-color: %0").arg(colorName));
        miscSettings->backgroundColor = color;
        emit settingsChanged();
    }
}

void MiscSettingsWidget::onGridCheckBoxClicked(){
    miscSettings->show_grid = ui->checkBoxGrid->isChecked();
    emit settingsChanged();
}

void MiscSettingsWidget::onSelectionOutlineClicked(){
    miscSettings->show_selection_outline = ui->checkBoxSelectionOutline->isChecked();
    emit settingsChanged();
}

void MiscSettingsWidget::onBloomClicked(){
    miscSettings->use_bloom = ui->checkBoxBloom->isChecked();
    emit settingsChanged();
}

void MiscSettingsWidget::onSSAOClicked(){
    miscSettings->use_ssao = ui->checkBoxSSAO->isChecked();
    emit settingsChanged();
}

void MiscSettingsWidget::onVisualHintChanged()
{
    miscSettings->renderLightSources = ui->checkBoxLightSources->isChecked();
    emit settingsChanged();
}
