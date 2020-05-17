#include "bfxtmetadatapairswidget.h"

BFXTMetadataPairsWidget::BFXTMetadataPairsWidget(QWidget* parent) : QWidget(parent)
{
    mainLayout              = new QGridLayout(this);
    addFieldPairButton      = new QPushButton("Add value pair", this);
    metadataPairsScrollArea = new QScrollArea(this);
    metadataPairsLayout     = new QVBoxLayout(metadataPairsScrollArea);
    pairsWidgetsWidget      = new QWidget(this);

    okButton    = new QPushButton("OK", this);
    clearButton = new QPushButton("Clear all", this);

    metadataPairsScrollArea->setLayout(metadataPairsLayout);
    metadataPairsScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    metadataPairsScrollArea->setWidgetResizable(true);
    metadataPairsScrollArea->setFrameStyle(QFrame::NoFrame);
    metadataPairsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    metadataPairsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    pairsWidgetsWidget->setLayout(metadataPairsLayout);
    metadataPairsScrollArea->setWidget(pairsWidgetsWidget);

    pairsWidgetsWidget->setStyleSheet("background-color: white;font-family:'Open "
                                      "Sans,sans-serif';");

    mainLayout->addWidget(okButton, 0, 0, 1, 1);
    mainLayout->addWidget(clearButton, 0, 1, 1, 1);
    mainLayout->addWidget(addFieldPairButton, 0, 2, 1, 1);
    mainLayout->addWidget(metadataPairsScrollArea, 1, 0, 1, 3);

    connect(addFieldPairButton, &QPushButton::clicked, this,
            &BFXTMetadataPairsWidget::slot_addKeyValuePair);
    connect(clearButton, &QPushButton::clicked, this, &BFXTMetadataPairsWidget::slot_clearPressed);
    connect(okButton, &QPushButton::clicked, this, &BFXTMetadataPairsWidget::slot_okPressed);

    metadataPairsLayout->setContentsMargins(0, 0, 0, 0);
    metadataPairsLayout->setSpacing(0);
    metadataPairsLayout->setMargin(5);
    metadataPairsLayout->setAlignment(Qt::AlignTop);

    slot_addKeyValuePair();
}

void BFXTMetadataPairsWidget::clearData()
{
    while (metadataPairsWidgets.size() > 1) {
        slot_removePairWidget(metadataPairsWidgets.front());
    }
    Q_ASSERT(metadataPairsWidgets.size() == 1);
    metadataPairsWidgets.front()->clearData();
}

void BFXTMetadataPairsWidget::slot_addKeyValuePair()
{
    BFXTMetadataPairWidget* widget = new BFXTMetadataPairWidget;
    metadataPairsWidgets.push_back(widget);
    metadataPairsLayout->addWidget(widget, 0, Qt::AlignTop);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    connect(widget, &BFXTMetadataPairWidget::signal_closeThis, this,
            &BFXTMetadataPairsWidget::slot_removePairWidget);
    slot_actToShowOrHideCloseButtons();
}

void BFXTMetadataPairsWidget::slot_actToShowOrHideCloseButtons()
{
    if (metadataPairsWidgets.size() > 1) {
        showAllCloseButtons();
    } else {
        hideAllCloseButtons();
    }
}

void BFXTMetadataPairsWidget::showAllCloseButtons()
{
    for (BFXTMetadataPairWidget* w : metadataPairsWidgets) {
        w->slot_showClose();
    }
}

void BFXTMetadataPairsWidget::hideAllCloseButtons()
{
    for (BFXTMetadataPairWidget* w : metadataPairsWidgets) {
        w->slot_hideClose();
    }
}

void BFXTMetadataPairsWidget::slot_removePairWidget(QWidget* widget)
{
    if (metadataPairsWidgets.size() <= 1) {
        return;
    }
    metadataPairsLayout->removeWidget(widget);
    delete widget;
    auto it = std::find(metadataPairsWidgets.begin(), metadataPairsWidgets.end(), widget);
    if (it != metadataPairsWidgets.end()) {
        metadataPairsWidgets.erase(it);
    } else {
        printf("Unable to find the metadata pair widget to erase");
    }
    slot_actToShowOrHideCloseButtons();
}

void BFXTMetadataPairsWidget::slot_okPressed() { emit sig_okPressed(); }

void BFXTMetadataPairsWidget::slot_clearPressed() { clearData(); }

json_spirit::Object BFXTMetadataPairsWidget::getJsonObject() const
{
    // {"userData":{"meta":[{"Hi":"there"},{"I am":"here!"}]}}
    json_spirit::Object resultObj;
    json_spirit::Array  metaArray;
    for (const auto& w : metadataPairsWidgets) {
        json_spirit::Object obj = w->getAsJsonObject();
        if (!obj.empty()) {
            metaArray.push_back(obj);
        }
    }
    resultObj.push_back(json_spirit::Pair("meta", metaArray));
    json_spirit::Pair p("userData", resultObj);
    return json_spirit::Object({p});
}

bool BFXTMetadataPairsWidget::isJsonEmpty() const
{
    bool allEmpty = true;
    for (const auto& w : metadataPairsWidgets) {
        allEmpty = allEmpty && w->isEmpty();
    }
    return allEmpty;
}
