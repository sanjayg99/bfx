include(bfx-wallet.pri)

TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += wallet

#BFX_CONFIG += Tests

contains( BFX_CONFIG, Tests ) {
    SUBDIRS += wallet/test
}

contains( BFX_CONFIG, NoWallet ) {
    SUBDIRS += wallet/test
    SUBDIRS -= wallet
}
