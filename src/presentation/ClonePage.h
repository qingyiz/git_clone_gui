#pragma once

#include "application/CloneController.h"
#include "application/ConfigurationStore.h"
#include "core/CloneRequest.h"
#include "presentation/NotificationSeverity.h"

#include <QList>
#include <QWidget>

class QFrame;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSplitter;
class QTimer;
class QVBoxLayout;

namespace gitclone {

class ChildRepositoryCard;
class BranchSelector;
class RemoteBranchService;

class ClonePage final : public QWidget {
    Q_OBJECT

public:
    explicit ClonePage(CloneController *controller,
                        ConfigurationStore *configurationStore,
                        QWidget *parent = nullptr);
    ClonePage(CloneController *controller,
               ConfigurationStore *configurationStore,
               RemoteBranchService *branchService,
               QWidget *parent);

    int childCardCount() const;

signals:
    void taskResultNotificationRequested(const QString &title,
                                         const QString &message,
                                         gitclone::NotificationSeverity severity);
    void closeReady();

public:
    bool requestClose();

private slots:
    void chooseDestinationRoot();
    void addEmptyChildCard();
    void updatePreview();
    void startClone();
    void setRunning(bool running);
    void appendLog(const QString &text);
    void showValidationErrors(const QStringList &errors);
    void handleStateChanged(CloneController::State state);
    void handleFinished(CloneController::Outcome outcome,
                        const QString &message,
                        const QString &parentTargetPath);
    void saveConfigurationNow();

private:
    CloneRequest currentRequest() const;
    void createUi();
    QFrame *createParentCard(QWidget *parent);
    QFrame *createDestinationCard(QWidget *parent);
    QFrame *createPreviewCard(QWidget *parent);
    QFrame *createStatusCard(QWidget *parent);
    QFrame *createLogCard(QWidget *parent);
    void connectUi();
    void restoreConfiguration();
    ChildRepositoryCard *addChildCard(const ChildRepositoryRequest &configuration = {});
    void removeChildCard(ChildRepositoryCard *card);
    void renumberChildCards();
    void scheduleConfigurationSave();
    void setConfigurationEnabled(bool enabled);
    void setValidationSummary(const QStringList &errors);
    void setTaskResult(CloneController::Outcome outcome, const QString &message);

    CloneController *m_controller;
    ConfigurationStore *m_configurationStore;
    RemoteBranchService *m_branchService;
    QLineEdit *m_parentRepositoryEdit = nullptr;
    BranchSelector *m_parentBranchSelector = nullptr;
    QLineEdit *m_parentDirectoryEdit = nullptr;
    QLineEdit *m_destinationRootEdit = nullptr;
    QPushButton *m_browseButton = nullptr;
    QPushButton *m_addChildButton = nullptr;
    QLabel *m_childCountLabel = nullptr;
    QVBoxLayout *m_childCardsLayout = nullptr;
    QList<ChildRepositoryCard *> m_childCards;
    QPlainTextEdit *m_previewEdit = nullptr;
    QLabel *m_validationLabel = nullptr;
    QFrame *m_statusCard = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_saveStatusLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QPlainTextEdit *m_logEdit = nullptr;
    QSplitter *m_executionSplitter = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QTimer *m_saveTimer = nullptr;
    bool m_loadingConfiguration = false;
    bool m_running = false;
    bool m_configurationValid = false;
    bool m_closeAfterCancel = false;
};

} // namespace gitclone
