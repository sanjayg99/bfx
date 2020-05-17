#include "bfxtsendtokensfeewidget.h"

#include "util.h"
#include <QDoubleValidator>

void BFXTSendTokensFeeWidget::createWidgets()
{
    mainLayout               = new QGridLayout;
    autoCalculateFeeCheckbox = new QCheckBox(this);
    feeAmountLineEdit        = new QLineEdit(this);

    mainLayout->addWidget(autoCalculateFeeCheckbox, 0, 0, 1, 1);
    mainLayout->addWidget(feeAmountLineEdit, 0, 1, 1, 1);

    feeAmountLineEdit->setPlaceholderText("Choose your fee in nebl, e.g., 0.0001");

    feeAmountLineEdit->setValidator(new QDoubleValidator(0, 100000000, 8));
    autoCalculateFeeCheckbox->setText("Automatically calculate the fee");
    setLayout(mainLayout);

    connect(autoCalculateFeeCheckbox, &QCheckBox::toggled, this,
            &BFXTSendTokensFeeWidget::slot_autoCalculateFeeStatusChanged);

    autoCalculateFeeCheckbox->setChecked(true);
}

BFXTSendTokensFeeWidget::BFXTSendTokensFeeWidget(QWidget* parent) : QWidget(parent) { createWidgets(); }

bool BFXTSendTokensFeeWidget::isAutoCalcFeeSelected() const
{
    return autoCalculateFeeCheckbox->isChecked();
}

std::string BFXTSendTokensFeeWidget::getEnteredFee() const
{
    return feeAmountLineEdit->text().toStdString();
}

void BFXTSendTokensFeeWidget::resetAllFields()
{
    autoCalculateFeeCheckbox->setChecked(true);
    feeAmountLineEdit->clear();
}

void BFXTSendTokensFeeWidget::slot_autoCalculateFeeStatusChanged(bool selected)
{
    feeAmountLineEdit->setEnabled(!selected);
}
