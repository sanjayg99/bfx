#include "bfxtmetadatapairwidget.h"

BFXTMetadataPairWidget::BFXTMetadataPairWidget(QWidget* parent) : QWidget(parent)
{
    mainLayout = new QGridLayout(this);

    this->setLayout(mainLayout);

    keyLineEdit = new QLineEdit(this);
    valLineEdit = new QLineEdit(this);
    closeButton = new QPushButton("", this);

    QIcon removeIcon;
    removeIcon.addFile(QStringLiteral(":/icons/remove"), QSize(), QIcon::Normal, QIcon::Off);
    closeButton->setIcon(removeIcon);

    keyLineEdit->setPlaceholderText("Key");
    valLineEdit->setPlaceholderText("Value");

    mainLayout->addWidget(keyLineEdit, 0, 0, 1, 1, Qt::AlignTop);
    mainLayout->addWidget(valLineEdit, 0, 1, 1, 1, Qt::AlignTop);
    mainLayout->addWidget(closeButton, 0, 2, 1, 1, Qt::AlignTop);

    mainLayout->setMargin(0);

    QSizePolicy keySp = keyLineEdit->sizePolicy();
    keySp.setHorizontalStretch(1);
    keySp.setVerticalPolicy(QSizePolicy::Expanding);
    keyLineEdit->setSizePolicy(keySp);

    QSizePolicy valSp = valLineEdit->sizePolicy();
    valSp.setHorizontalStretch(1);
    valSp.setVerticalPolicy(QSizePolicy::Expanding);
    valLineEdit->setSizePolicy(valSp);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    connect(closeButton, &QPushButton::clicked, this, &BFXTMetadataPairWidget::slot_closeThis);
}

QPair<QString, QString> BFXTMetadataPairWidget::getKeyValuePair() const
{
    return qMakePair(keyLineEdit->text(), valLineEdit->text());
}

void BFXTMetadataPairWidget::clearData()
{
    keyLineEdit->setText("");
    valLineEdit->setText("");
}

void BFXTMetadataPairWidget::slot_hideClose() { closeButton->hide(); }

void BFXTMetadataPairWidget::slot_showClose() { closeButton->show(); }

bool BFXTMetadataPairWidget::isEmpty() const
{
    return keyLineEdit->text().trimmed().isEmpty() && valLineEdit->text().trimmed().isEmpty();
}

json_spirit::Object BFXTMetadataPairWidget::getAsJsonObject() const
{
    json_spirit::Object result;
    if (!keyLineEdit->text().trimmed().isEmpty()) {
        std::string key = keyLineEdit->text().toUtf8().toStdString();
        std::string val = valLineEdit->text().toUtf8().toStdString();
        result.push_back(json_spirit::Pair(key, val));
    }
    return result;
}

void BFXTMetadataPairWidget::slot_closeThis() { emit signal_closeThis(this); }
