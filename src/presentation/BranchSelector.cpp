#include "presentation/BranchSelector.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QSignalBlocker>
#include <QStyle>

namespace gitclone {

BranchSelector::BranchSelector(RemoteBranchService *branchService,
                               QWidget *parent,
                               int debounceMs)
    : QComboBox(parent)
    , m_branchService(branchService)
{
    setEditable(true);
    setInsertPolicy(QComboBox::NoInsert);
    setMaxVisibleItems(14);
    setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    setMinimumContentsLength(12);
    lineEdit()->setPlaceholderText(QStringLiteral("输入或选择远程分支"));

    QCompleter *branchCompleter = completer();
    branchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    branchCompleter->setCompletionMode(QCompleter::PopupCompletion);
    branchCompleter->setFilterMode(Qt::MatchContains);
    branchCompleter->setMaxVisibleItems(14);

    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(debounceMs);
    connect(&m_debounceTimer, &QTimer::timeout, this, &BranchSelector::beginQuery);
    connect(this, &QComboBox::editTextChanged, this, &BranchSelector::branchChanged);
    connect(lineEdit(), &QLineEdit::textEdited, this, [this](const QString &text) {
        if (suggestionCount() > 0) {
            completer()->setCompletionPrefix(text);
            completer()->complete();
        }
    });

    if (m_branchService != nullptr) {
        connect(m_branchService, &RemoteBranchService::branchesReady,
                this, &BranchSelector::applyCatalog);
        connect(m_branchService, &RemoteBranchService::branchQueryFailed,
                this, &BranchSelector::applyFailure);
    }
    setLookupState(QStringLiteral("idle"),
                   QStringLiteral("输入仓库 URL 后会自动读取远程分支，也可直接手工输入。"));
}

BranchSelector::~BranchSelector()
{
    if (m_branchService != nullptr && m_requestId != 0) {
        m_branchService->cancelRequest(m_requestId);
    }
}

QString BranchSelector::branchText() const
{
    return currentText();
}

void BranchSelector::setBranchText(const QString &branch)
{
    setEditText(branch);
}

void BranchSelector::setRepositoryUrl(const QString &repositoryUrl)
{
    const QString url = repositoryUrl.trimmed();
    if (m_repositoryUrl == url) {
        return;
    }

    m_debounceTimer.stop();
    if (m_branchService != nullptr && m_requestId != 0) {
        m_branchService->cancelRequest(m_requestId);
    }
    m_requestId = 0;
    m_repositoryUrl = url;
    const QString typedBranch = currentText();
    {
        const QSignalBlocker blocker(this);
        clear();
        setEditText(typedBranch);
    }

    if (url.isEmpty() || m_branchService == nullptr) {
        setLookupState(QStringLiteral("idle"),
                       QStringLiteral("输入仓库 URL 后会自动读取远程分支，也可直接手工输入。"));
        return;
    }
    setLookupState(QStringLiteral("waiting"), QStringLiteral("等待 URL 输入完成…"));
    m_debounceTimer.start();
}

void BranchSelector::setEditorObjectName(const QString &objectName)
{
    lineEdit()->setObjectName(objectName);
}

int BranchSelector::suggestionCount() const
{
    return count();
}

bool BranchSelector::isPopupIndicatorExpanded() const
{
    return m_popupVisible;
}

void BranchSelector::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const bool focused = hasFocus() || lineEdit()->hasFocus();
    const bool hovered = underMouse();
    const QString lookupState = property("lookupState").toString();

    QColor background(QStringLiteral("#FBFBFC"));
    QColor border(QStringLiteral("#C9CED6"));
    QColor separator(QStringLiteral("#E2E5E9"));
    QColor arrow(QStringLiteral("#646B75"));
    qreal borderWidth = 1.0;

    if (!isEnabled()) {
        background = QColor(QStringLiteral("#EFF1F3"));
        border = QColor(QStringLiteral("#DDE1E6"));
        separator = QColor(QStringLiteral("#DDE1E6"));
        arrow = QColor(QStringLiteral("#969CA5"));
    } else if (focused || m_popupVisible) {
        background = QColor(QStringLiteral("#FFFFFF"));
        border = QColor(QStringLiteral("#2F6FEB"));
        separator = QColor(QStringLiteral("#DCE4F1"));
        arrow = QColor(QStringLiteral("#285EBD"));
        borderWidth = 1.5;
    } else if (lookupState == QStringLiteral("error")) {
        border = QColor(QStringLiteral("#B9823E"));
        arrow = QColor(QStringLiteral("#8D5D22"));
    } else if (lookupState == QStringLiteral("loading")) {
        border = QColor(QStringLiteral("#6F96D8"));
        arrow = QColor(QStringLiteral("#285EBD"));
    } else if (hovered) {
        background = QColor(QStringLiteral("#FFFFFF"));
        border = QColor(QStringLiteral("#AAB1BB"));
        arrow = QColor(QStringLiteral("#3E454F"));
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal inset = borderWidth / 2.0 + 0.25;
    const QRectF frameRect = QRectF(rect()).adjusted(inset, inset, -inset, -inset);
    QPainterPath framePath;
    framePath.addRoundedRect(frameRect, 8.0, 8.0);
    painter.fillPath(framePath, background);
    painter.setPen(QPen(border, borderWidth));
    painter.drawPath(framePath);

    const qreal separatorX = width() - 34.0;
    painter.setPen(QPen(separator, 1.0));
    painter.drawLine(QPointF(separatorX, 9.0), QPointF(separatorX, height() - 9.0));

    const QPointF center(width() - 17.0, height() / 2.0);
    QPainterPath chevron;
    if (m_popupVisible) {
        chevron.moveTo(center + QPointF(-4.5, 2.0));
        chevron.lineTo(center + QPointF(0.0, -2.5));
        chevron.lineTo(center + QPointF(4.5, 2.0));
    } else {
        chevron.moveTo(center + QPointF(-4.5, -2.0));
        chevron.lineTo(center + QPointF(0.0, 2.5));
        chevron.lineTo(center + QPointF(4.5, -2.0));
    }
    QPen arrowPen(arrow, 1.8);
    arrowPen.setCapStyle(Qt::RoundCap);
    arrowPen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(arrowPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(chevron);
}

void BranchSelector::showPopup()
{
    m_popupVisible = true;
    update();
    QComboBox::showPopup();
}

void BranchSelector::hidePopup()
{
    QComboBox::hidePopup();
    m_popupVisible = false;
    update();
}

void BranchSelector::beginQuery()
{
    if (m_branchService == nullptr || m_repositoryUrl.isEmpty()) {
        return;
    }
    setLookupState(QStringLiteral("loading"), QStringLiteral("正在读取远程分支…"));
    m_requestId = m_branchService->requestBranches(m_repositoryUrl);
}

void BranchSelector::applyCatalog(RemoteBranchService::RequestId requestId,
                                  const RemoteBranchCatalog &catalog)
{
    if (requestId == 0 || requestId != m_requestId) {
        return;
    }
    m_requestId = 0;
    const QString typedBranch = currentText();
    const bool chooseDefault = typedBranch.trimmed().isEmpty()
        && !catalog.defaultBranch.isEmpty();
    {
        const QSignalBlocker blocker(this);
        clear();
        addItems(catalog.branches);
        setEditText(chooseDefault ? catalog.defaultBranch : typedBranch);
    }
    if (chooseDefault) {
        emit branchChanged(catalog.defaultBranch);
    }
    setLookupState(QStringLiteral("ready"),
                   catalog.branches.isEmpty()
                       ? QStringLiteral("远端未返回分支；仍可手工输入。")
                       : QStringLiteral("已读取 %1 个远程分支；默认与常用分支优先，可输入关键词搜索。")
                             .arg(catalog.branches.size()));
}

void BranchSelector::applyFailure(RemoteBranchService::RequestId requestId,
                                  const QString &message)
{
    if (requestId == 0 || requestId != m_requestId) {
        return;
    }
    m_requestId = 0;
    setLookupState(QStringLiteral("error"), message + QStringLiteral(" 仍可手工输入分支。"));
}

void BranchSelector::setLookupState(const QString &state, const QString &toolTipText)
{
    setProperty("lookupState", state);
    setToolTip(toolTipText);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

} // namespace gitclone
