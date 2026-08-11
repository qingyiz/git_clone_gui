#pragma once

#include "application/RemoteBranchService.h"

#include <QComboBox>
#include <QTimer>

class QPaintEvent;

namespace gitclone {

class BranchSelector final : public QComboBox {
    Q_OBJECT

public:
    explicit BranchSelector(RemoteBranchService *branchService,
                            QWidget *parent = nullptr,
                            int debounceMs = 450);
    ~BranchSelector() override;

    QString branchText() const;
    void setBranchText(const QString &branch);
    void setRepositoryUrl(const QString &repositoryUrl);
    void setEditorObjectName(const QString &objectName);
    int suggestionCount() const;
    bool isPopupIndicatorExpanded() const;
    void showPopup() override;
    void hidePopup() override;

signals:
    void branchChanged(const QString &branch);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void beginQuery();
    void applyCatalog(RemoteBranchService::RequestId requestId,
                      const RemoteBranchCatalog &catalog);
    void applyFailure(RemoteBranchService::RequestId requestId,
                      const QString &message);
    void setLookupState(const QString &state, const QString &toolTip);

    RemoteBranchService *m_branchService;
    QTimer m_debounceTimer;
    RemoteBranchService::RequestId m_requestId = 0;
    QString m_repositoryUrl;
    bool m_popupVisible = false;
};

} // namespace gitclone
