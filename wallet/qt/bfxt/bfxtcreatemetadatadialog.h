#ifndef BFXTCREATEMETADATADIALOG_H
#define BFXTCREATEMETADATADIALOG_H

#include "bfxtcustommetadatawidget.h"
#include "bfxtmetadatapairswidget.h"
#include <QCheckBox>
#include <QDialog>
#include <QGridLayout>
#include <QPushButton>

class BFXTCreateMetadataDialog : public QDialog
{
    Q_OBJECT

    QGridLayout* mainLayout;
    QCheckBox*   editModeCheckbox;
    QCheckBox*   encryptDataCheckbox;

    BFXTMetadataPairsWidget*  metadataPairsWidget;
    BFXTCustomMetadataWidget* customDataWidget;

public:
    BFXTCreateMetadataDialog(QWidget* parent = Q_NULLPTR);

    json_spirit::Object getJsonData() const;
    bool                encryptData() const;
    bool                jsonDataExists() const;
    bool                jsonDataValid() const;
    void                clearData();

public slots:
    void slot_customJsonDataSwitched();

    // QWidget interface
protected:
    // effect on clicking the X button of the window
    void closeEvent(QCloseEvent* event) Q_DECL_OVERRIDE;

    // QDialog interface
public slots:
    // effect on Esc keyboard press
    void reject() Q_DECL_OVERRIDE;
};

#endif // BFXTCREATEMETADATADIALOG_H
