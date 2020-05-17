#include "bfxtsummary.h"
#include "ui_bfxtsummary.h"

#include "bitcoinunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "bfxt/bfxttokenlistitemdelegate.h"
#include "optionsmodel.h"
#include "qt/bfxt/bfxttokenlistmodel.h"
#include "main.h"
#include "walletmodel.h"

#include <QAction>
#include <QClipboard>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QMenu>
#include <QUrl>

#include "txdb.h"

const QString BFXTSummary::copyTokenIdText          = "Copy Token ID";
const QString BFXTSummary::copyTokenSymbolText      = "Copy Token Symbol";
const QString BFXTSummary::copyTokenNameText        = "Copy Token Name";
const QString BFXTSummary::copyIssuanceTxid         = "Copy issuance transaction ID";
const QString BFXTSummary::viewInBlockExplorerText  = "Show in block explorer";
const QString BFXTSummary::viewIssuanceMetadataText = "Show issuance metadata";

void BFXTSummary::GetAlreadyIssuedBFXTTokens(boost::promise<std::unordered_set<std::string>>& promise)
{
    try {
        LOCK(cs_main);
        std::unordered_set<std::string> result;
        CTxDB                           txdb;
        std::vector<uint256>            txs;
        // retrieve all issuance transactions hashes from db
        if (txdb.ReadAllIssuanceTxs(txs)) {
            for (const uint256& hash : txs) {
                // get every tx from db
                BFXTTransaction bfxttx;
                if (txdb.ReadBFXTTx(hash, bfxttx)) {
                    // make sure that the transaction is in the main chain
                    CTransaction tx;
                    uint256      blockHash;
                    if (!GetTransaction(hash, tx, blockHash)) {
                        throw std::runtime_error(
                            "Failed to find the block that belongs to transaction: " + hash.ToString());
                    }
                    auto it = mapBlockIndex.find(blockHash);
                    if (it != mapBlockIndex.cend() && it->second->IsInMainChain()) {
                        std::string tokenSymbol = bfxttx.getTokenSymbolIfIssuance();
                        // symbols should be in upper case for the comparison to work
                        std::transform(tokenSymbol.begin(), tokenSymbol.end(), tokenSymbol.begin(),
                                       ::toupper);
                        result.insert(tokenSymbol);
                    }
                } else {
                    throw std::runtime_error("Failed to read transaction " + hash.ToString() +
                                             " from blockchain database");
                }
            }
            promise.set_value(result);
        } else {
            throw std::runtime_error(
                "Failed to retrieve the list of already issued token symbols. This is "
                "necessary to avoid having duplicate token names");
        }
    } catch (std::exception& ex) {
        promise.set_exception(boost::current_exception());
    }
}

BFXTSummary::BFXTSummary(QWidget* parent)
    : QWidget(parent), ui(new Ui_BFXTSummary), currentBalance(-1), currentStake(0),
      currentUnconfirmedBalance(-1), currentImmatureBalance(-1), model(0), filter(0),
      tokenDelegate(new BFXTTokenListItemDelegate)
{
    model          = new BFXTTokenListModel;
    metadataViewer = new BFXTMetadataViewer;
    metadataViewer->setModal(true);
    ui->setupUi(this);

    bfxtLoaderConcluderTimer = new QTimer(this);

    ui->listTokens->setItemDelegate(tokenDelegate);
    ui->listTokens->setIconSize(
        QSize(BFXTTokenListItemDelegate::DECORATION_SIZE, BFXTTokenListItemDelegate::DECORATION_SIZE));
    ui->listTokens->setMinimumHeight(BFXTTokenListItemDelegate::NUM_ITEMS *
                                     (BFXTTokenListItemDelegate::DECORATION_SIZE + 2));
    ui->listTokens->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTokens, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTokenClicked(QModelIndex)));

    //    ui->showSendDialogButton->setText(sendDialogHiddenStr);
    //    connect(ui->showSendDialogButton, &QPushButton::clicked, this,
    //            &BFXTSummary::slot_actToShowSendTokensView);

    // init "out of sync" warning labels
    ui->labelBlockchainSyncStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    setupContextMenu();

    filter = new BFXTTokenListFilterProxy(ui->filter_lineEdit);
    setModel(model);

    connect(model, &BFXTTokenListModel::signal_walletUpdateRunning, ui->upper_table_loading_label,
            &QLabel::setVisible);
    connect(ui->filter_lineEdit, &QLineEdit::textChanged, filter,
            &BFXTTokenListFilterProxy::setFilterWildcard);
    connect(ui->issueNewBFXTTokenButton, &QPushButton::clicked, this,
            &BFXTSummary::slot_showIssueNewTokenDialog);
}

void BFXTSummary::handleTokenClicked(const QModelIndex& index)
{
    if (filter)
        emit tokenClicked(filter->mapToSource(index));
}

void BFXTSummary::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) {
        ui->filter_lineEdit->setFocus();
    }
}

void BFXTSummary::setupContextMenu()
{
    ui->listTokens->setContextMenuPolicy(Qt::CustomContextMenu);
    contextMenu               = new QMenu(this);
    copyTokenIdAction         = new QAction(copyTokenIdText, this);
    copyTokenSymbolAction     = new QAction(copyTokenSymbolText, this);
    copyTokenNameAction       = new QAction(copyTokenNameText, this);
    copyIssuanceTxidAction    = new QAction(copyIssuanceTxid, this);
    viewInBlockExplorerAction = new QAction(viewInBlockExplorerText, this);
    showMetadataAction        = new QAction(viewIssuanceMetadataText, this);
    contextMenu->addAction(copyTokenIdAction);
    contextMenu->addAction(copyTokenSymbolAction);
    contextMenu->addAction(copyTokenNameAction);
    contextMenu->addSeparator();
    contextMenu->addAction(copyIssuanceTxidAction);
    contextMenu->addAction(viewInBlockExplorerAction);
    contextMenu->addSeparator();
    contextMenu->addAction(showMetadataAction);
    connect(ui->listTokens, &TokensListView::customContextMenuRequested, this,
            &BFXTSummary::slot_contextMenuRequested);

    connect(copyTokenIdAction, &QAction::triggered, this, &BFXTSummary::slot_copyTokenIdAction);
    connect(copyTokenSymbolAction, &QAction::triggered, this, &BFXTSummary::slot_copyTokenSymbolAction);
    connect(copyTokenNameAction, &QAction::triggered, this, &BFXTSummary::slot_copyTokenNameAction);
    connect(copyIssuanceTxidAction, &QAction::triggered, this,
            &BFXTSummary::slot_copyIssuanceTxidAction);
    connect(viewInBlockExplorerAction, &QAction::triggered, this,
            &BFXTSummary::slot_visitInBlockExplorerAction);
    connect(showMetadataAction, &QAction::triggered, this, &BFXTSummary::slot_showMetadataAction);
    connect(bfxtLoaderConcluderTimer, &QTimer::timeout, this, &BFXTSummary::slot_concludeLoadBFXTTokens);
}

void BFXTSummary::slot_copyTokenIdAction()
{
    QModelIndexList selected = ui->listTokens->selectedIndexesP();
    std::set<int>   rows;
    for (long i = 0; i < selected.size(); i++) {
        QModelIndex index = selected.at(i);
        int         row   = index.row();
        rows.insert(row);
    }
    if (rows.size() != 1) {
        QMessageBox::warning(this, "Failed to copy",
                             "Failed to copy Token ID; selected items size is not equal to one");
        return;
    }
    QModelIndex idx   = ui->listTokens->model()->index(*rows.begin(), 0);
    QString resultStr = ui->listTokens->model()->data(idx, BFXTTokenListModel::TokenIdRole).toString();
    if (!resultStr.isEmpty()) {
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(resultStr);
    } else {
        QMessageBox::warning(this, "Failed to copy", "No information to include in the clipboard");
    }
}

void BFXTSummary::slot_copyTokenSymbolAction()
{
    QModelIndexList selected = ui->listTokens->selectedIndexesP();
    std::set<int>   rows;
    for (long i = 0; i < selected.size(); i++) {
        QModelIndex index = selected.at(i);
        int         row   = index.row();
        rows.insert(row);
    }
    if (rows.size() != 1) {
        QMessageBox::warning(this, "Failed to copy",
                             "Failed to copy Token Symbol; selected items size is not equal to one");
        return;
    }
    QModelIndex idx   = ui->listTokens->model()->index(*rows.begin(), 0);
    QString resultStr = ui->listTokens->model()->data(idx, BFXTTokenListModel::TokenNameRole).toString();
    if (!resultStr.isEmpty()) {
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(resultStr);
    } else {
        QMessageBox::warning(this, "Failed to copy", "No information to include in the clipboard");
    }
}

void BFXTSummary::slot_copyTokenNameAction()
{
    QModelIndexList selected = ui->listTokens->selectedIndexesP();
    std::set<int>   rows;
    for (long i = 0; i < selected.size(); i++) {
        QModelIndex index = selected.at(i);
        int         row   = index.row();
        rows.insert(row);
    }
    if (rows.size() != 1) {
        QMessageBox::warning(this, "Failed to copy",
                             "Failed to copy Token Name; selected items size is not equal to one");
        return;
    }
    QString resultStr = ui->listTokens->model()
                            ->data(ui->listTokens->model()->index(*rows.begin(), 0),
                                   BFXTTokenListModel::TokenDescriptionRole)
                            .toString();
    if (!resultStr.isEmpty()) {
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(resultStr);
    } else {
        QMessageBox::warning(this, "Failed to copy", "No information to include in the clipboard");
    }
}

void BFXTSummary::slot_copyIssuanceTxidAction()
{
    QModelIndexList selected = ui->listTokens->selectedIndexesP();
    std::set<int>   rows;
    for (long i = 0; i < selected.size(); i++) {
        QModelIndex index = selected.at(i);
        int         row   = index.row();
        rows.insert(row);
    }
    if (rows.size() != 1) {
        QMessageBox::warning(this, "Failed get URL",
                             "Failed to get Token ID; selected items size is not equal to one");
        return;
    }
    QModelIndex idx = ui->listTokens->model()->index(*rows.begin(), 0);
    QString     resultStr =
        ui->listTokens->model()->data(idx, BFXTTokenListModel::IssuanceTxidRole).toString();
    if (!resultStr.isEmpty()) {
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(resultStr);
    } else {
        QMessageBox::warning(this, "Failed to copy", "No information to include in the clipboard");
    }
}

void BFXTSummary::slot_visitInBlockExplorerAction()
{
    QModelIndexList selected = ui->listTokens->selectedIndexesP();
    std::set<int>   rows;
    for (long i = 0; i < selected.size(); i++) {
        QModelIndex index = selected.at(i);
        int         row   = index.row();
        rows.insert(row);
    }
    if (rows.size() != 1) {
        QMessageBox::warning(this, "Failed get URL",
                             "Failed to get Token ID; selected items size is not equal to one");
        return;
    }
    QModelIndex idx   = ui->listTokens->model()->index(*rows.begin(), 0);
    QString resultStr = ui->listTokens->model()->data(idx, BFXTTokenListModel::TokenIdRole).toString();
    if (!resultStr.isEmpty()) {
        QString link = QString::fromStdString(
            BFXTTools::GetURL_ExplorerTokenInfo(resultStr.toStdString(), fTestNet));
        if (!QDesktopServices::openUrl(QUrl(link))) {
            QMessageBox::warning(
                this, "Failed to open browser",
                "Could not open the web browser to view token information in block explorer");
        }
    } else {
        QMessageBox::warning(this, "Failed to get token ID", "No information retrieved for token ID");
    }
}

void BFXTSummary::slot_showMetadataAction()
{
    QModelIndexList selected = ui->listTokens->selectedIndexesP();
    std::set<int>   rows;
    for (long i = 0; i < selected.size(); i++) {
        QModelIndex index = selected.at(i);
        int         row   = index.row();
        rows.insert(row);
    }
    if (rows.size() != 1) {
        QMessageBox::warning(this, "Failed get URL",
                             "Failed to get Token ID; selected items size is not equal to one");
        return;
    }
    QModelIndex                   idx       = ui->listTokens->model()->index(*rows.begin(), 0);
    BFXTTokenListModel::RoleIndex role      = BFXTTokenListModel::IssuanceTxidRole;
    QString                       resultStr = ui->listTokens->model()->data(idx, role).toString();
    if (!resultStr.isEmpty()) {
        uint256 issuanceTxid(resultStr.toStdString());
        try {
            auto it = issuanceTxidVsMetadata.find(issuanceTxid);
            if (it == issuanceTxidVsMetadata.cend()) {
                json_spirit::Value v;
                try {
                    v = BFXTTransaction::GetBFXTIssuanceMetadata(issuanceTxid);
                } catch(std::exception& ex) {
                    QMessageBox::warning(this, "Failed to get issuance transaction",
                                         "Failed to retrieve issuance transaction with error: " +
                                             QString(ex.what()));
                    return;
                }
                issuanceTxidVsMetadata[issuanceTxid] = v;
                std::stringstream ss;
                json_spirit::write_formatted(v, ss);
                metadataViewer->setJsonStr(ss.str());
            } else {
                std::stringstream ss;
                json_spirit::write_formatted(it->second, ss);
                metadataViewer->setJsonStr(ss.str());
            }
            metadataViewer->show();
            metadataViewer->raise();
        } catch (std::exception& ex) {
            QMessageBox::warning(this, "Failed to get issuance transaction",
                                 "Failed to retrieve issuance transaction with error: " +
                                     QString(ex.what()));
        } catch (...) {
            QMessageBox::warning(this, "Failed to get issuance transaction",
                                 "Failed to retrieve issuance transaction with an unknown error.");
        }
    } else {
        QMessageBox::warning(
            this, "Failed to get issuance txid",
            "Failed to retrieve issuance txid, which is required to retrieve the metadata");
    }
}

void BFXTSummary::slot_showIssueNewTokenDialog()
{
    if (!isBFXTTokensLoadRunning) {
        printf("Loading BFXT tokens list...\n");
        QSize iconSize(ui->issueNewBFXTTokenButton->height(), ui->issueNewBFXTTokenButton->height());
        ui->loadIssuedBFXTSpinnerMovie->setScaledSize(iconSize);

        ui->loadIssuedBFXTSpinnerLabel->setToolTip("Loading issued BFXT tokens...");
        ui->loadIssuedBFXTSpinnerLabel->setMovie(ui->loadIssuedBFXTSpinnerMovie);
        ui->loadIssuedBFXTSpinnerMovie->start();

        ui->loadIssuedBFXTSpinnerLabel->setVisible(true);
        ui->issueNewBFXTTokenButton->setVisible(false);

        alreadyIssuedBFXTSymbolsPromise = boost::promise<std::unordered_set<std::string>>();
        alreadyIssuedBFXTSymbolsFuture  = alreadyIssuedBFXTSymbolsPromise.get_future();
        boost::thread loadBFXTIssuedTokensThread(boost::bind(
            &BFXTSummary::GetAlreadyIssuedBFXTTokens, boost::ref(alreadyIssuedBFXTSymbolsPromise)));
        loadBFXTIssuedTokensThread.detach();
        bfxtLoaderConcluderTimer->start(bfxtLoaderConcluderTimerTimeout);
        isBFXTTokensLoadRunning = true;
    }
}

void BFXTSummary::slot_concludeLoadBFXTTokens()
{
    try {
        if (isBFXTTokensLoadRunning && alreadyIssuedBFXTSymbolsFuture.is_ready()) {
            printf("Concluding loading issued BFXT tokens...\n");

            bfxtLoaderConcluderTimer->stop();

            std::unordered_set<std::string> alreadyIssuedSymbols = alreadyIssuedBFXTSymbolsFuture.get();

            ui->loadIssuedBFXTSpinnerLabel->setVisible(false);
            ui->issueNewBFXTTokenButton->setVisible(true);
            ui->loadIssuedBFXTSpinnerMovie->stop();

            isBFXTTokensLoadRunning = false;

            ui->issueNewBFXTTokenDialog->setAlreadyIssuedTokensSymbols(alreadyIssuedSymbols);
            ui->issueNewBFXTTokenDialog->show();
        } else {
        }
    } catch (std::exception& ex) {
        QMessageBox::warning(this, "Failed to retrieve token names",
                             "Failed to retrieve the list of already issued token symbols. This is "
                             "necessary to avoid having duplicate token names. Error: " +
                                 QString(ex.what()));
    }
}

BFXTSummary::~BFXTSummary() { delete ui; }

BFXTTokenListModel* BFXTSummary::getTokenListModel() const { return model; }

void BFXTSummary::setModel(BFXTTokenListModel* model)
{
    if (model) {
        filter->setSourceModel(model);
        ui->listTokens->setModel(filter);
    }
}

void BFXTSummary::showOutOfSyncWarning(bool fShow) { ui->labelBlockchainSyncStatus->setVisible(fShow); }

void BFXTSummary::slot_contextMenuRequested(QPoint pos)
{
    QModelIndexList selected = ui->listTokens->selectedIndexesP();
    if (selected.size() != 1) {
        copyTokenIdAction->setDisabled(true);
        metadataViewer->setDisabled(true);
    } else {
        copyTokenIdAction->setDisabled(false);
        metadataViewer->setDisabled(false);
    }
    contextMenu->popup(ui->listTokens->viewport()->mapToGlobal(pos));
}
