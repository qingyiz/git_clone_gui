#pragma once

#include "application/CloneController.h"
#include "application/ConfigurationStore.h"
#include "core/CloneRequest.h"

#include <QList>
#include <QMainWindow>

class QCloseEvent;
class QFrame;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QTimer;
class QVBoxLayout;
class QWidget;

namespace gitclone {

class ChildRepositoryCard;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(CloneController *controller,
                        ConfigurationStore *configurationStore,
                        QWidget *parent = nullptr);

    int childCardCount() const;

protected:
    void closeEvent(QCloseEvent *event) override;

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

    CloneController *m_controller;
    ConfigurationStore *m_configurationStore;
    QLineEdit *m_parentRepositoryEdit = nullptr;
    QLineEdit *m_parentBranchEdit = nullptr;
    QLineEdit *m_parentDirectoryEdit = nullptr;
    QLineEdit *m_destinationRootEdit = nullptr;
    QPushButton *m_browseButton = nullptr;
    QPushButton *m_addChildButton = nullptr;
    QLabel *m_childCountLabel = nullptr;
    QVBoxLayout *m_childCardsLayout = nullptr;
    QList<ChildRepositoryCard *> m_childCards;
    QPlainTextEdit *m_previewEdit = nullptr;
    QLabel *m_validationLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_saveStatusLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QPlainTextEdit *m_logEdit = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QTimer *m_saveTimer = nullptr;
    bool m_loadingConfiguration = false;
    bool m_running = false;
    bool m_configurationValid = false;
    bool m_closeAfterCancel = false;
};

} // namespace gitclone
