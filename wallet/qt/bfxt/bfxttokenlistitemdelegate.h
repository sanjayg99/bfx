#ifndef BFXTTOKENLISTITEMDELEGATE_H
#define BFXTTOKENLISTITEMDELEGATE_H

#include <QAbstractItemDelegate>
#include <QPainter>

class BFXTTokenListItemDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    static const int DECORATION_SIZE = 64;
    static const int NUM_ITEMS = 3;

    BFXTTokenListItemDelegate();

    virtual ~BFXTTokenListItemDelegate();

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const Q_DECL_OVERRIDE;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
};

#endif // BFXTTOKENLISTITEMDELEGATE_H
