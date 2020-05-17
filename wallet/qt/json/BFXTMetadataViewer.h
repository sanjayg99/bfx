#ifndef BFXTMETADATAVIEWER_H
#define BFXTMETADATAVIEWER_H

#include "qt/json/JsonTreeModel.h"
#include "qt/json/JsonTreeView.h"
#include <QDialog>
#include <QGridLayout>
#include <QPlainTextEdit>
#include <QPushButton>

class BFXTMetadataViewer : public QDialog
{
    Q_OBJECT

    QGridLayout* mainLayout;
    QPushButton* switchTreeTextViewButton;

    JsonTreeView*   treeView;
    QPlainTextEdit* textView;
    JsonTreeModel   treeModel;

    void setRoot(std::shared_ptr<JsonTreeNode> root);

public:
    BFXTMetadataViewer(QWidget* parent = 0);

    void setJsonStr(const std::string& jsonStr);

private slots:
    void slot_switchTreeTextView();
};

#endif // BFXTMETADATAVIEWER_H
