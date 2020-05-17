#ifndef BFXTTOKENLISTFILTERPROXY_H
#define BFXTTOKENLISTFILTERPROXY_H

#include <QSortFilterProxyModel>
#include <QLineEdit>

class BFXTTokenListFilterProxy : public QSortFilterProxyModel
{
    QLineEdit* filterLineEdit;
public:
    BFXTTokenListFilterProxy(QLineEdit *FilterLineEdit = NULL);

    // QSortFilterProxyModel interface
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

#endif // BFXTTOKENLISTFILTERPROXY_H
