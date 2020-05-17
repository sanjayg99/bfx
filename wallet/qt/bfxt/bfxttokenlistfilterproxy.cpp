#include "bfxttokenlistfilterproxy.h"
#include "bfxttokenlistmodel.h"

BFXTTokenListFilterProxy::BFXTTokenListFilterProxy(QLineEdit* FilterLineEdit)
{
    filterLineEdit = FilterLineEdit;
}

bool BFXTTokenListFilterProxy::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (filterLineEdit == NULL) {
        return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
    }

    if (filterLineEdit->text().isEmpty()) {
        return true;
    }

    QModelIndex columnsIndex = sourceModel()->index(source_row, 0, source_parent);
    if (sourceModel()
            ->data(columnsIndex, BFXTTokenListModel::AmountRole)
            .toString()
            .contains(filterLineEdit->text(), Qt::CaseInsensitive) ||
        sourceModel()
            ->data(columnsIndex, BFXTTokenListModel::TokenDescriptionRole)
            .toString()
            .contains(filterLineEdit->text(), Qt::CaseInsensitive) ||
        sourceModel()
            ->data(columnsIndex, BFXTTokenListModel::TokenIdRole)
            .toString()
            .contains(filterLineEdit->text(), Qt::CaseInsensitive) ||
        sourceModel()
            ->data(columnsIndex, BFXTTokenListModel::TokenNameRole)
            .toString()
            .contains(filterLineEdit->text(), Qt::CaseInsensitive)) {
        return true;
    } else {
        return false;
    }
}
