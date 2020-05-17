#ifndef BFXTSUMMARY_H
#define BFXTSUMMARY_H

#include "bfxt/bfxttokenlistfilterproxy.h"
#include "bfxt/bfxttokenlistitemdelegate.h"
#include "bfxt/bfxttokenlistmodel.h"
#include "ui_bfxtsummary.h"
#include "json/BFXTMetadataViewer.h"
#include <QTimer>
#include <QWidget>

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class Ui_BFXTSummary;

class WalletModel;

/** BFXTSummary page widget */
class BFXTSummary : public QWidget
{
    Q_OBJECT

    // since loading the list of BFXT tokens is async, this is will change accordingly
    bool    isBFXTTokensLoadRunning = false;
    QTimer* bfxtLoaderConcluderTimer;
    int     bfxtLoaderConcluderTimerTimeout = 100;

    boost::promise<std::unordered_set<std::string>>       alreadyIssuedBFXTSymbolsPromise;
    boost::unique_future<std::unordered_set<std::string>> alreadyIssuedBFXTSymbolsFuture;

    static void GetAlreadyIssuedBFXTTokens(boost::promise<std::unordered_set<std::string>>& promise);

public:
    explicit BFXTSummary(QWidget* parent = 0);
    ~BFXTSummary();

    void setModel(BFXTTokenListModel* model);
    void showOutOfSyncWarning(bool fShow);

public slots:

signals:
    void tokenClicked(const QModelIndex& index);

public:
    Ui_BFXTSummary*     ui;
    BFXTTokenListModel* getTokenListModel() const;

private:
    static const QString copyTokenIdText;
    static const QString copyTokenSymbolText;
    static const QString copyTokenNameText;
    static const QString copyIssuanceTxid;
    static const QString viewInBlockExplorerText;
    static const QString viewIssuanceMetadataText;

    qint64                    currentBalance;
    qint64                    currentStake;
    qint64                    currentUnconfirmedBalance;
    qint64                    currentImmatureBalance;
    BFXTTokenListModel*       model;
    BFXTTokenListFilterProxy* filter;

    BFXTTokenListItemDelegate* tokenDelegate;

    QMenu*   contextMenu               = Q_NULLPTR;
    QAction* copyTokenIdAction         = Q_NULLPTR;
    QAction* copyTokenSymbolAction     = Q_NULLPTR;
    QAction* copyTokenNameAction       = Q_NULLPTR;
    QAction* copyIssuanceTxidAction    = Q_NULLPTR;
    QAction* viewInBlockExplorerAction = Q_NULLPTR;
    QAction* showMetadataAction        = Q_NULLPTR;

    BFXTMetadataViewer* metadataViewer;

    // caching mechanism for the metadata to avoid accessing the db repeatedly4
    std::unordered_map<uint256, json_spirit::Value> issuanceTxidVsMetadata;

private slots:
    void handleTokenClicked(const QModelIndex& index);
    void slot_contextMenuRequested(QPoint pos);
    void slot_copyTokenIdAction();
    void slot_copyTokenSymbolAction();
    void slot_copyTokenNameAction();
    void slot_copyIssuanceTxidAction();
    void slot_visitInBlockExplorerAction();
    void slot_showMetadataAction();
    void slot_showIssueNewTokenDialog();
    void slot_concludeLoadBFXTTokens();

    // QWidget interface
protected:
    void keyPressEvent(QKeyEvent* event);
    void setupContextMenu();
};

#endif // BFXTSUMMARY_H
