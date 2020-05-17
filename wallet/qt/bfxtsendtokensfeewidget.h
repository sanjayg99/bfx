#ifndef BFXTSENDTOKENSFEEWIDGET_H
#define BFXTSENDTOKENSFEEWIDGET_H

#include <QCheckBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QWidget>

class BFXTSendTokensFeeWidget : public QWidget
{
    Q_OBJECT

    QGridLayout* mainLayout;
    QCheckBox*   autoCalculateFeeCheckbox;
    QLineEdit*   feeAmountLineEdit;

    void createWidgets();

public:
    explicit BFXTSendTokensFeeWidget(QWidget* parent = nullptr);
    bool        isAutoCalcFeeSelected() const;
    std::string getEnteredFee() const;
    void        resetAllFields();

signals:

public slots:
    void slot_autoCalculateFeeStatusChanged(bool selected);
};

#endif // BFXTSENDTOKENSFEEWIDGET_H
