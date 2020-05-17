#include "bfxtcreatemetadatadialog.h"

#include <QDesktopWidget>
#include <QMessageBox>

BFXTCreateMetadataDialog::BFXTCreateMetadataDialog(QWidget* parent) : QDialog(parent)
{
    mainLayout = new QGridLayout(this);
    setLayout(mainLayout);
    setWindowTitle("Edit BFXT metadata");

    resize(QDesktopWidget().availableGeometry(this).size() * 0.5);

    editModeCheckbox    = new QCheckBox("Input custom json data", this);
    encryptDataCheckbox = new QCheckBox("Encrypt data (possible for only one recipient)", this);
    metadataPairsWidget = new BFXTMetadataPairsWidget(this);
    customDataWidget    = new BFXTCustomMetadataWidget(this);
    customDataWidget->setVisible(false);

    mainLayout->addWidget(editModeCheckbox, 0, 0, 1, 1);
    mainLayout->addWidget(encryptDataCheckbox, 1, 0, 1, 1);
    mainLayout->addWidget(metadataPairsWidget, 2, 0, 1, 1);
    mainLayout->addWidget(customDataWidget, 2, 0, 1, 1);

    connect(metadataPairsWidget, &BFXTMetadataPairsWidget::sig_okPressed, this, &QWidget::hide);
    connect(customDataWidget, &BFXTCustomMetadataWidget::sig_okPressed, this, &QWidget::hide);
    connect(this->editModeCheckbox, &QCheckBox::toggled, this,
            &BFXTCreateMetadataDialog::slot_customJsonDataSwitched);
}

json_spirit::Object BFXTCreateMetadataDialog::getJsonData() const
{
    try {
        if (editModeCheckbox->isChecked()) {
            return customDataWidget->getJsonObject();
        } else {
            return metadataPairsWidget->getJsonObject();
        }
    } catch (...) {
        QMessageBox::warning(
            this->customDataWidget, "Error retrieving json data",
            "An error happened while attempting to retrieve json data. This should not happen.");
        return json_spirit::Object({json_spirit::Pair("Error", "")});
    }
}

bool BFXTCreateMetadataDialog::encryptData() const { return encryptDataCheckbox->isChecked(); }

bool BFXTCreateMetadataDialog::jsonDataExists() const
{
    if (editModeCheckbox->isChecked()) {
        return !customDataWidget->isJsonEmpty();
    } else {
        return !metadataPairsWidget->isJsonEmpty();
    }
}

bool BFXTCreateMetadataDialog::jsonDataValid() const
{
    if (editModeCheckbox->isChecked()) {
        return !customDataWidget->isJsonEmpty() && customDataWidget->isJsonValid();
    } else {
        return !metadataPairsWidget->isJsonEmpty();
    }
}

void BFXTCreateMetadataDialog::clearData()
{
    customDataWidget->clearData();
    metadataPairsWidget->clearData();
    editModeCheckbox->setChecked(false);
}

void BFXTCreateMetadataDialog::slot_customJsonDataSwitched()
{
    metadataPairsWidget->setVisible(!editModeCheckbox->isChecked());
    customDataWidget->setVisible(editModeCheckbox->isChecked());
}

void BFXTCreateMetadataDialog::closeEvent(QCloseEvent* /*event*/)
{
    QMessageBox::information(this, "Changes saved", "Your changes (if any) have been saved");
    hide();
}

void BFXTCreateMetadataDialog::reject()
{
    QMessageBox::information(this, "Changes saved", "Your changes (if any) have been saved");
    hide();
}
