#ifndef BFXTLISTELEMENTTOKENDATA_H
#define BFXTLISTELEMENTTOKENDATA_H

#include <QString>
#include <QIcon>
#include "bfxt/bfxttokenlistmodel.h"

struct BFXTListElementTokenData
{
    QString name;
    QString tokenId;
    qint64  amount;
    QIcon   icon;
    void    fill(int index, boost::shared_ptr<BFXTWallet> wallet)
    {
        name = BFXTTokenListModel::__getTokenName(index, wallet);
        icon = BFXTTokenListModel::__getTokenIcon(index, wallet);
        if (icon.isNull()) {
            icon = QIcon(":/images/orion");
        }
        tokenId = BFXTTokenListModel::__getTokenId(index, wallet);
        amount  = FromString<qint64>(BFXTTokenListModel::__getTokenBalance(index, wallet).toStdString());
    }
};

#endif // BFXTLISTELEMENTTOKENDATA_H
