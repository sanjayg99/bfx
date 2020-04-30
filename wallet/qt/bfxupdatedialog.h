#ifndef BFXUPDATEDIALOG_H
#define BFXUPDATEDIALOG_H

#include <QDialog>
#include <QTextBrowser>
#include <QGridLayout>
#include <QLabel>

#include "bfxupdater.h"

class BFXUpdateDialog : public QDialog
{
    Q_OBJECT

    QLabel* currentVersionTitleLabel;
    QLabel* currentVersionContentLabel;
    QLabel* remoteVesionTitleLabel;
    QLabel* remoteVesionContentLabel;
    QTextBrowser* updateInfoText;
    QGridLayout*  mainLayout;
    QLabel* downloadLinkLabel;

    void setRemoteVersion(const QString& version);
    void setCurrentVersion(const QString& version);
    void setBodyText(const QString& bodyText);
    void setDownloadLink(const QString& link);

public:
    explicit BFXUpdateDialog(QWidget *parent = 0);

    void setUpdateRelease(const BFXReleaseInfo &rel);

signals:

public slots:
};

#endif // BFXUPDATEDIALOG_H
