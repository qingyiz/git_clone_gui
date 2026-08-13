#include "presentation/AppStyle.h"

namespace gitclone {

QString applicationStyleSheet()
{
    return QStringLiteral(R"(
QMainWindow, QWidget#appRoot {
    background: #F4F7FB;
    color: #0F172A;
    font-size: 13px;
}
QWidget#windowRoot, QStackedWidget#mainPageStack {
    background: #F4F7FB;
}
QFrame#navigationSidebar {
    background: #0F172A;
    border: none;
}
QLabel#navigationBrand {
    color: #FFFFFF;
    font-size: 18px;
    font-weight: 700;
}
QFrame#navigationSidebar QLabel[role="muted"] {
    color: #94A3B8;
}
QLabel#navigationVersionLabel {
    background: #172033;
    border: 1px solid #26354F;
    border-radius: 8px;
    color: #CBD5E1;
    padding: 8px 10px;
    font-size: 11px;
}
QPushButton[buttonRole="navigation"] {
    min-height: 30px;
    text-align: left;
    padding: 10px 12px;
    border: none;
    border-radius: 9px;
    background: transparent;
    color: #CBD5E1;
}
QPushButton[buttonRole="navigation"]:hover {
    background: #1E293B;
    color: #FFFFFF;
}
QPushButton[buttonRole="navigation"]:checked {
    background: #2563EB;
    color: #FFFFFF;
}
QFrame[role="panel"] {
    background: #F8FAFC;
    border: 1px solid #DDE5EF;
    border-radius: 16px;
}
QFrame[role="card"] {
    background: #FFFFFF;
    border: 1px solid #E2E8F0;
    border-radius: 12px;
}
QFrame[role="statusCard"] {
    background: #EFF6FF;
    border: 1px solid #BFDBFE;
    border-radius: 12px;
}
QFrame#statusCard[statusState="success"] {
    background: #ECFDF5;
    border: 1px solid #A7F3D0;
}
QFrame#statusCard[statusState="error"] {
    background: #FEF2F2;
    border: 1px solid #FECACA;
}
QLabel#appIcon {
    background: #2563EB;
    color: #FFFFFF;
    border-radius: 12px;
    font-size: 20px;
    font-weight: 700;
}
QLabel#appTitle {
    color: #0F172A;
    font-size: 23px;
    font-weight: 700;
}
QLabel#sectionTitle, QLabel#cardTitle {
    color: #0F172A;
    font-size: 15px;
    font-weight: 700;
}
QLabel[role="muted"] {
    color: #64748B;
    font-size: 12px;
}
QLabel[role="fieldLabel"] {
    color: #475569;
    font-size: 12px;
    font-weight: 600;
}
QLabel#cardNumberBadge {
    background: #DBEAFE;
    color: #1D4ED8;
    border-radius: 14px;
    font-weight: 700;
}
QLabel#saveStatusLabel, QLabel#childCountLabel {
    background: #E2E8F0;
    color: #475569;
    border-radius: 10px;
    padding: 4px 9px;
    font-size: 11px;
    font-weight: 600;
}
QLabel#workspaceRepositoryCount, QLabel[role="branchBadge"] {
    background: #DBEAFE;
    color: #1D4ED8;
    border-radius: 10px;
    padding: 4px 9px;
    font-weight: 600;
}
QLabel#workspaceStatusLabel {
    padding: 9px 12px;
    border-radius: 8px;
    background: #E2E8F0;
    color: #475569;
}
QLabel#workspaceStatusLabel[statusState="success"] {
    background: #ECFDF5;
    color: #047857;
}
QLabel#workspaceStatusLabel[statusState="error"] {
    background: #FEF2F2;
    color: #B91C1C;
}
QLabel#workspaceStatusLabel[statusState="loading"] {
    background: #EFF6FF;
    color: #1D4ED8;
}
QFrame#workspaceWorktreeStatusCard {
    background: #F8FAFC;
    border: 1px solid #E2E8F0;
    border-left: 4px solid #94A3B8;
    border-radius: 9px;
}
QFrame#workspaceWorktreeStatusCard[worktreeState="clean"] {
    background: #ECFDF5;
    border-color: #A7F3D0;
    border-left-color: #10B981;
}
QFrame#workspaceWorktreeStatusCard[worktreeState="dirty"] {
    background: #FFF7ED;
    border-color: #FED7AA;
    border-left-color: #F97316;
}
QLabel#workspaceWorktreeStatusTitle {
    color: #475569;
    font-weight: 700;
}
QFrame#workspaceWorktreeStatusCard[worktreeState="clean"] QLabel#workspaceWorktreeStatusTitle {
    color: #047857;
}
QFrame#workspaceWorktreeStatusCard[worktreeState="dirty"] QLabel#workspaceWorktreeStatusTitle {
    color: #C2410C;
    font-size: 14px;
}
QLabel#workspaceWorktreeStatusDetails {
    color: #64748B;
    font-size: 12px;
}
QFrame#workspaceWorktreeStatusCard[worktreeState="dirty"] QLabel#workspaceWorktreeStatusDetails {
    color: #9A3412;
}
QLabel#validationSummary[validationState="ready"] {
    color: #047857;
}
QLabel#validationSummary[validationState="error"] {
    color: #B91C1C;
}
QLineEdit, QComboBox {
    min-height: 22px;
    padding: 8px 10px;
    background: #F8FAFC;
    color: #0F172A;
    border: 1px solid #CBD5E1;
    border-radius: 8px;
    selection-background-color: #BFDBFE;
}
QLineEdit:hover, QComboBox:hover {
    border-color: #94A3B8;
}
QLineEdit:focus, QComboBox:focus {
    background: #FFFFFF;
    border: 2px solid #3B82F6;
    padding: 7px 9px;
}
QLineEdit:disabled, QComboBox:disabled {
    background: #F1F5F9;
    color: #94A3B8;
    border-color: #E2E8F0;
}
QComboBox QLineEdit {
    min-height: 22px;
    padding: 0px;
    background: transparent;
    border: none;
}
QComboBox QLineEdit:focus {
    padding: 0px;
    border: none;
}
QComboBox QAbstractItemView {
    background: #FFFFFF;
    color: #0F172A;
    border: 1px solid #CBD5E1;
    border-radius: 8px;
    padding: 4px;
    selection-background-color: #DBEAFE;
    selection-color: #1D4ED8;
    outline: none;
}
QComboBox[lookupState="loading"] {
    border-color: #60A5FA;
}
QComboBox[lookupState="error"] {
    border-color: #F59E0B;
}
QPushButton {
    min-height: 22px;
    padding: 8px 14px;
    border-radius: 8px;
    border: 1px solid #CBD5E1;
    background: #FFFFFF;
    color: #334155;
    font-weight: 600;
}
QPushButton:hover {
    background: #F8FAFC;
    border-color: #94A3B8;
}
QPushButton:pressed {
    background: #E2E8F0;
}
QPushButton:disabled {
    background: #F1F5F9;
    color: #A3AFBF;
    border-color: #E2E8F0;
}
QPushButton[buttonRole="primary"] {
    background: #2563EB;
    color: #FFFFFF;
    border-color: #2563EB;
    padding-left: 20px;
    padding-right: 20px;
}
QPushButton[buttonRole="primary"]:hover {
    background: #1D4ED8;
    border-color: #1D4ED8;
}
QPushButton[buttonRole="primary"]:disabled {
    background: #BFDBFE;
    color: #EFF6FF;
    border-color: #BFDBFE;
}
QPushButton[buttonRole="accentGhost"] {
    color: #1D4ED8;
    border-color: #BFDBFE;
    background: #EFF6FF;
}
QPushButton[buttonRole="accentGhost"]:hover {
    background: #DBEAFE;
}
QPushButton[buttonRole="dangerGhost"] {
    color: #DC2626;
    border-color: transparent;
    background: transparent;
    padding: 5px 9px;
}
QPushButton[buttonRole="dangerGhost"]:hover {
    background: #FEF2F2;
    border-color: #FECACA;
}
QPlainTextEdit {
    background: #0F172A;
    color: #DDE7F4;
    border: 1px solid #1E293B;
    border-radius: 9px;
    padding: 10px;
    selection-background-color: #1D4ED8;
    font-family: "Menlo";
    font-size: 11px;
}
QListWidget, QTabWidget::pane {
    background: #F8FAFC;
    color: #0F172A;
    border: 1px solid #DDE5EF;
    border-radius: 8px;
    outline: none;
}
QListWidget::item {
    min-height: 26px;
    padding: 4px 7px;
    border-radius: 6px;
}
QListWidget::item:selected {
    background: #DBEAFE;
    color: #1D4ED8;
}
QTreeWidget#workspaceRepositoryTree {
    background: #F8FAFC;
    color: #172033;
    border: 1px solid #D8E2EF;
    border-radius: 10px;
    padding: 5px 3px 5px 5px;
    outline: none;
    selection-background-color: transparent;
}
QTreeWidget#workspaceRepositoryTree::item {
    background: transparent;
    border: none;
}
QTreeWidget#workspaceRepositoryTree::branch {
    background: transparent;
    border: none;
    image: none;
}
QTreeWidget#workspaceRepositoryTree QScrollBar:vertical {
    width: 7px;
    margin: 5px 2px 5px 0px;
    background: transparent;
}
QTreeWidget#workspaceRepositoryTree QScrollBar::handle:vertical {
    min-height: 36px;
    background: #C8D4E3;
    border-radius: 3px;
}
QTreeWidget#workspaceRepositoryTree QScrollBar::handle:vertical:hover {
    background: #AAB9CC;
}
QTreeWidget#workspaceRepositoryTree QScrollBar::add-line:vertical,
QTreeWidget#workspaceRepositoryTree QScrollBar::sub-line:vertical {
    height: 0px;
}
QTreeWidget#workspaceRepositoryTree QScrollBar::add-page:vertical,
QTreeWidget#workspaceRepositoryTree QScrollBar::sub-page:vertical {
    background: transparent;
}
QTabBar::tab {
    padding: 8px 12px;
    color: #64748B;
    border-bottom: 2px solid transparent;
}
QTabBar::tab:selected {
    color: #1D4ED8;
    border-bottom-color: #2563EB;
}
QProgressBar {
    min-height: 7px;
    max-height: 7px;
    background: #DBEAFE;
    border: none;
    border-radius: 3px;
    text-align: center;
}
QProgressBar::chunk {
    background: #2563EB;
    border-radius: 3px;
}
QScrollArea {
    border: none;
    background: transparent;
}
QSplitter::handle:vertical {
    height: 8px;
    margin: 2px 24px;
    background: #D7E0EC;
    border-radius: 3px;
}
QSplitter::handle:vertical:hover {
    background: #93C5FD;
}
QWidget#executionSummaryContainer {
    background: transparent;
}
QWidget#configurationScrollContent {
    background: transparent;
}
QScrollBar:vertical {
    width: 9px;
    margin: 2px;
    background: transparent;
}
QScrollBar::handle:vertical {
    min-height: 30px;
    background: #CBD5E1;
    border-radius: 4px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QToolTip {
    background: #0F172A;
    color: #FFFFFF;
    border: none;
    padding: 5px;
}
)");
}

} // namespace gitclone
