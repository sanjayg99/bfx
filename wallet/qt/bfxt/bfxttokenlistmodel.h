#ifndef BFXTTOKENLISTMODEL_H
#define BFXTTOKENLISTMODEL_H

#include <QAbstractTableModel>
#include <QTimer>
#include <atomic>

#include "init.h"
#include "bfxt/bfxtwallet.h"
#include "wallet.h"

class BFXTTokenListModel : public QAbstractTableModel
{
    Q_OBJECT

    boost::shared_ptr<BFXTWallet> bfxtwallet;
    bool                          walletLocked;
    boost::atomic_bool            walletUpdateRunning;
    boost::atomic_flag            walletUpdateLockFlag = BOOST_ATOMIC_FLAG_INIT;
    boost::atomic_bool            updateWalletAgain;
    // this mutex should not be necessary, but future<shared_ptr> is indicating an issue with
    // thread-sanitizer
    boost::mutex walletFutureCheckMutex;

    QTimer* walletUpdateEnderTimer;

    boost::promise<boost::shared_ptr<BFXTWallet>>       updateWalletPromise;
    boost::unique_future<boost::shared_ptr<BFXTWallet>> updateWalletFuture;

    static void UpdateWalletBalances(boost::shared_ptr<BFXTWallet>                  wallet,
                                     boost::promise<boost::shared_ptr<BFXTWallet>>& promise);

    class BFXTWalletTxUpdater : public WalletNewTxUpdateFunctor
    {
        BFXTTokenListModel* model;
        int                 currentBlockHeight;

    public:
        BFXTWalletTxUpdater(BFXTTokenListModel* Model) : model(Model), currentBlockHeight(-1) {}

        void run(uint256, int currentHeight) Q_DECL_OVERRIDE
        {
            if (currentBlockHeight < 0) {
                setReferenceBlockHeight();
            }
            if (currentHeight <= currentBlockHeight + HEIGHT_OFFSET_TOLERANCE) {
                model->reloadBalances();
            }
        }

        virtual ~BFXTWalletTxUpdater() {}

        // WalletNewTxUpdateFunctor interface
    public:
        void setReferenceBlockHeight() Q_DECL_OVERRIDE { currentBlockHeight = nBestHeight; }
    };

    boost::shared_ptr<BFXTWalletTxUpdater> bfxtWalletTxUpdater;
    void                                   SetupBFXTWalletTxUpdaterToWallet()
    {
        while (!std::atomic_load(&pwalletMain).get()) {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }
        std::atomic_load(&pwalletMain)->setFunctorOnTxInsert(bfxtWalletTxUpdater);
    }

public:
    static QString __getTokenName(int index, boost::shared_ptr<BFXTWallet> theWallet);
    static QString __getTokenId(int index, boost::shared_ptr<BFXTWallet> theWallet);
    static QString __getTokenDescription(int index, boost::shared_ptr<BFXTWallet> theWallet);
    static QString __getTokenBalance(int index, boost::shared_ptr<BFXTWallet> theWallet);
    static QString __getIssuanceTxid(int index, boost::shared_ptr<BFXTWallet> theWallet);
    static QIcon   __getTokenIcon(int index, boost::shared_ptr<BFXTWallet> theWallet);

    void clearBFXTWallet();
    void refreshBFXTWallet();

    BFXTTokenListModel();
    virtual ~BFXTTokenListModel();
    void reloadBalances();

    int      rowCount(const QModelIndex& parent) const Q_DECL_OVERRIDE;
    int      columnCount(const QModelIndex& parent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;

    /** Roles to get specific information from a token row.
        These are independent of column.
    */
    enum RoleIndex
    {
        TokenNameRole = Qt::UserRole,
        TokenDescriptionRole,
        TokenIdRole,
        AmountRole,
        IssuanceTxidRole
    };

    void                          saveWalletToFile();
    void                          loadWalletFromFile();
    boost::shared_ptr<BFXTWallet> getCurrentWallet() const;
signals:
    void signal_walletUpdateRunning(bool running);

private slots:
    void beginWalletUpdate();
    void endWalletUpdate();
};

extern std::atomic<BFXTTokenListModel*> bfxtTokenListModelInstance;

#endif // BFXTTOKENLISTMODEL_H
