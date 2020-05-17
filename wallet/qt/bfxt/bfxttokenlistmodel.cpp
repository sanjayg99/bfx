#include "bfxttokenlistmodel.h"
#include "boost/thread/future.hpp"
#include <boost/atomic/atomic.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <QIcon>
#include <QImage>

std::atomic<BFXTTokenListModel*> bfxtTokenListModelInstance{nullptr};

QString BFXTTokenListModel::__getTokenName(int index, boost::shared_ptr<BFXTWallet> theWallet)
{
    return QString::fromStdString(theWallet->getTokenName(index));
}

QString BFXTTokenListModel::__getTokenId(int index, boost::shared_ptr<BFXTWallet> theWallet)
{
    return QString::fromStdString(theWallet->getTokenId(index));
}

QString BFXTTokenListModel::__getTokenDescription(int index, boost::shared_ptr<BFXTWallet> theWallet)
{
    return QString::fromStdString(theWallet->getTokenDescription(index));
}

QString BFXTTokenListModel::__getTokenBalance(int index, boost::shared_ptr<BFXTWallet> theWallet)
{
    return QString::fromStdString(ToString(theWallet->getTokenBalance(index)));
}

QString BFXTTokenListModel::__getIssuanceTxid(int index, boost::shared_ptr<BFXTWallet> theWallet)
{
    return QString::fromStdString(theWallet->getTokenIssuanceTxid(index));
}

QIcon BFXTTokenListModel::__getTokenIcon(int index, boost::shared_ptr<BFXTWallet> theWallet)
{
    const std::string& iconData = theWallet->getTokenIcon(index);
    if (iconData.empty() || BFXTWallet::IconHasErrorContent(iconData)) {
        return QIcon();
    }
    QImage iconImage;
    iconImage.loadFromData((const uchar*)iconData.c_str(), iconData.size());
    return QIcon(QPixmap::fromImage(iconImage));
}

void BFXTTokenListModel::clearBFXTWallet()
{
    if (bfxtwallet) {
        beginResetModel();
        bfxtwallet->clear();
        endResetModel();
    }
}

void BFXTTokenListModel::refreshBFXTWallet()
{
    if (bfxtwallet) {
        beginResetModel();
        bfxtwallet->update();
        endResetModel();
    }
}

void BFXTTokenListModel::UpdateWalletBalances(boost::shared_ptr<BFXTWallet>                  wallet,
                                              boost::promise<boost::shared_ptr<BFXTWallet>>& promise)
{
    try {
        boost::atomic_load(&wallet)->update();
        promise.set_value(wallet);
    } catch (...) {
        try {
            promise.set_exception(boost::current_exception());
        } catch (std::exception& ex) {
            printf("Error: Setting promise exception failed for BFXTTokenListModel wallet: %s",
                   ex.what());
        } catch (...) {
            printf(
                "Error: Setting promise exception failed for BFXTTokenListModel wallet (Unknown error)");
        }
    }
}

BFXTTokenListModel::BFXTTokenListModel()
    : bfxtWalletTxUpdater(boost::make_shared<BFXTWalletTxUpdater>(this))
{
    updateWalletAgain.store(false);
    walletUpdateLockFlag.clear();
    bfxtwallet             = boost::make_shared<BFXTWallet>();
    walletLocked           = false;
    walletUpdateRunning    = false;
    walletUpdateEnderTimer = new QTimer(this);
    loadWalletFromFile();
    connect(walletUpdateEnderTimer, &QTimer::timeout, this, &BFXTTokenListModel::endWalletUpdate);
    walletUpdateEnderTimer->start(1000);
    reloadBalances();
    boost::thread t(&BFXTTokenListModel::SetupBFXTWalletTxUpdaterToWallet, this);
    t.detach();
    bfxtTokenListModelInstance.store(this);
}

BFXTTokenListModel::~BFXTTokenListModel()
{
    bfxtTokenListModelInstance.store(nullptr);
    bfxtWalletTxUpdater.reset();
}

void BFXTTokenListModel::reloadBalances()
{
    // the following mechanism is to ensure that if this function is called too often, it won't
    // indefinitely block, but will simply queue requests through the boolean `updateWalletAgain`

    bool dummy       = false;
    auto restoreFunc = [this](bool*) { walletUpdateLockFlag.clear(); };
    std::unique_ptr<bool, decltype(restoreFunc)> lg(&dummy, restoreFunc); // RAII, acts like a lock_guard
    do {
        // only one thread can do this, if another thread tries, it'll just schedule another update
        if (!walletUpdateLockFlag.test_and_set(boost::memory_order_seq_cst)) {
            beginWalletUpdate();
        } else {
            // if locking failed, that means something else triggered the scan, so we just schedule an
            // update and exit when this scan is done
            updateWalletAgain.store(true, boost::memory_order_seq_cst);
            break;
        }
        // continue if a rescan is scheduled
    } while (updateWalletAgain.exchange(false, boost::memory_order_seq_cst));
}

void BFXTTokenListModel::beginWalletUpdate()
{
    if (!walletUpdateRunning) {
        walletUpdateRunning = true;
        emit                          signal_walletUpdateRunning(true);
        boost::shared_ptr<BFXTWallet> wallet = boost::make_shared<BFXTWallet>(*bfxtwallet);
        updateWalletPromise                  = boost::promise<boost::shared_ptr<BFXTWallet>>();
        {
            boost::lock_guard<boost::mutex> lg(walletFutureCheckMutex);
            updateWalletFuture = updateWalletPromise.get_future();
        }
        boost::thread t(boost::bind(&BFXTTokenListModel::UpdateWalletBalances, wallet,
                                    boost::ref(updateWalletPromise)));
        t.detach();
    }
}

void BFXTTokenListModel::endWalletUpdate()
{
    bool is_ready = false;
    {
        boost::lock_guard<boost::mutex> lg(walletFutureCheckMutex);
        is_ready = updateWalletFuture.is_ready();
    }
    if (walletUpdateRunning && is_ready) {
        try {
            boost::shared_ptr<BFXTWallet> wallet = updateWalletFuture.get();
            // the nullptr check is done after having a nullptr once happen.
            // Although this should never happen, having it doesn't hurt and is safer
            if ((wallet.get() != nullptr) && !(*wallet == *bfxtwallet)) {
                beginResetModel();
                boost::atomic_store(&bfxtwallet, wallet);
                saveWalletToFile();
                endResetModel();
            }
        } catch (std::exception& ex) {
            printf("Error while updating BFXT balances: %s", ex.what());
        }
        walletUpdateRunning = false;
        emit signal_walletUpdateRunning(false);
    }
}

int BFXTTokenListModel::rowCount(const QModelIndex& /*parent*/) const
{
    return bfxtwallet->getNumberOfTokens();
}

int BFXTTokenListModel::columnCount(const QModelIndex& /*parent*/) const { return 3; }

QVariant BFXTTokenListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= static_cast<int>(bfxtwallet->getNumberOfTokens()))
        return QVariant();

    if (role == BFXTTokenListModel::AmountRole) {
        return __getTokenBalance(index.row(), bfxtwallet);
    }
    if (role == BFXTTokenListModel::TokenDescriptionRole) {
        return __getTokenDescription(index.row(), bfxtwallet);
    }
    if (role == BFXTTokenListModel::TokenIdRole) {
        return __getTokenId(index.row(), bfxtwallet);
    }
    if (role == BFXTTokenListModel::TokenNameRole) {
        return __getTokenName(index.row(), bfxtwallet);
    }
    if (role == BFXTTokenListModel::IssuanceTxidRole) {
        return __getIssuanceTxid(index.row(), bfxtwallet);
    }
    if (role == Qt::DecorationRole) {
        return QVariant(__getTokenIcon(index.row(), bfxtwallet));
    }
    return QVariant();
}

void BFXTTokenListModel::saveWalletToFile()
{
    try {
        if (!bfxtwallet) {
            throw std::runtime_error("Error: BFXT wallet is a null pointer!");
        }

        // The temp file here ensures that the target file is not tampered with before a successful
        // writing happens This is helpful to avoid corrupt files in cases such as diskspace issues
        srand(time(NULL));
        boost::filesystem::path tempFile = GetDataDir() / BFXTWalletCacheFileName;
        tempFile.replace_extension(".json." + ToString(rand()));
        boost::filesystem::path permFile = GetDataDir() / BFXTWalletCacheFileName;
        bfxtwallet->exportToFile(tempFile);
        if (boost::filesystem::exists(permFile))
            boost::filesystem::remove(permFile);
        boost::filesystem::rename(tempFile, permFile);
    } catch (std::exception& ex) {
        printf("Failed at exporting wallet data. Error says %s", ex.what());
    }
}

void BFXTTokenListModel::loadWalletFromFile()
{
    try {
        if (!bfxtwallet) {
            throw std::runtime_error("Error: BFXT wallet is a null pointer!");
        }
        boost::filesystem::path file = GetDataDir() / BFXTWalletCacheFileName;
        if (boost::filesystem::exists(file)) {
            bfxtwallet->importFromFile(file);
        } else {
            printf("BFXT wallet not found. One will be created soon.");
        }
    } catch (std::exception& ex) {
        bfxtwallet->clear();
        printf("Failed at exporting wallet data. Error says %s", ex.what());
    }
}

boost::shared_ptr<BFXTWallet> BFXTTokenListModel::getCurrentWallet() const
{
    return boost::atomic_load(&bfxtwallet);
}
