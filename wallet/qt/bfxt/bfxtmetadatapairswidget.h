#ifndef BFXTMETADATAPAIRSWIDGET_H
#define BFXTMETADATAPAIRSWIDGET_H

#include "bfxtmetadatapairwidget.h"
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <json_spirit.h>

class BFXTMetadataPairsWidget : public QWidget
{
    Q_OBJECT

    QGridLayout*                     mainLayout;
    QPushButton*                     addFieldPairButton;
    QVBoxLayout*                     metadataPairsLayout;
    QScrollArea*                     metadataPairsScrollArea;
    QVector<BFXTMetadataPairWidget*> metadataPairsWidgets;
    QWidget*                         pairsWidgetsWidget;
    QPushButton*                     okButton;
    QPushButton*                     clearButton;

    void showAllCloseButtons();
    void hideAllCloseButtons();

public:
    BFXTMetadataPairsWidget(QWidget* parent = Q_NULLPTR);
    void clearData();

signals:
    void sig_okPressed();

public slots:
    void slot_addKeyValuePair();
    void slot_actToShowOrHideCloseButtons();
    void slot_removePairWidget(QWidget* widget);
    void slot_okPressed();
    void slot_clearPressed();

    json_spirit::Object getJsonObject() const;
    bool                isJsonEmpty() const;
};

#endif // BFXTMETADATAPAIRSWIDGET_H
