#include "presentation/AppStyle.h"

namespace gitclone {

QString applicationStyleSheet()
{
    return QStringLiteral(R"(
QMainWindow, QWidget#appRoot, QWidget#workspacePage {
    background: #F4F5F7;
    color: #20242A;
    font-size: 13px;
}
QWidget#windowRoot, QStackedWidget#mainPageStack {
    background: #F4F5F7;
}
QFrame#navigationSidebar {
    background: #ECEEF1;
    border: none;
    border-right: 1px solid #D6D9DE;
}
QLabel#navigationBrand {
    color: #20242A;
    font-size: 15px;
    font-weight: 650;
}
QFrame#navigationSidebar QLabel[role="muted"] {
    color: #777D87;
}
QLabel#navigationVersionLabel {
    background: transparent;
    border: none;
    border-radius: 0px;
    color: #7C828C;
    padding: 2px 3px;
    font-size: 11px;
}
QPushButton[buttonRole="navigation"] {
    min-height: 28px;
    text-align: left;
    padding: 7px 10px 7px 11px;
    border: 1px solid transparent;
    border-left: 3px solid transparent;
    border-radius: 6px;
    background: transparent;
    color: #555C66;
    font-weight: 500;
}
QPushButton[buttonRole="navigation"]:hover {
    background: #E5E7EA;
    color: #20242A;
}
QPushButton[buttonRole="navigation"]:checked {
    background: #E1E4E8;
    color: #20242A;
    border-color: transparent;
    border-left-color: #2F6FEB;
    font-weight: 650;
}
QFrame[role="panel"] {
    background: #F8F9FA;
    border: 1px solid #D9DDE3;
    border-radius: 8px;
}
QFrame[role="card"] {
    background: #FFFFFF;
    border: 1px solid #DDE1E6;
    border-radius: 7px;
}
QFrame[role="statusCard"] {
    background: #F7F8FA;
    border: 1px solid #D9DDE3;
    border-radius: 7px;
}
QFrame#statusCard[statusState="success"] {
    background: #F0F8F4;
    border-color: #BFDCCB;
}
QFrame#statusCard[statusState="error"] {
    background: #FCF3F3;
    border-color: #E8C5C5;
}
QLabel#appTitle {
    color: #20242A;
    font-size: 20px;
    font-weight: 650;
}
QLabel#sectionTitle, QLabel#cardTitle {
    color: #292D33;
    font-size: 14px;
    font-weight: 650;
}
QLabel[role="muted"] {
    color: #747B86;
    font-size: 12px;
}
QLabel[role="fieldLabel"] {
    color: #5E6570;
    font-size: 12px;
    font-weight: 550;
}
QLabel#cardNumberBadge {
    background: #F0F2F5;
    color: #59616D;
    border: 1px solid #D9DDE3;
    border-radius: 5px;
    font-size: 11px;
    font-weight: 650;
}
QLabel#saveStatusLabel, QLabel#childCountLabel {
    background: transparent;
    color: #777D87;
    border: none;
    border-radius: 0px;
    padding: 2px 3px;
    font-size: 11px;
    font-weight: 500;
}
QLabel#workspaceRepositoryCount {
    background: #F0F2F5;
    color: #646B75;
    border: 1px solid #DDE1E6;
    border-radius: 5px;
    padding: 3px 7px;
    font-size: 11px;
    font-weight: 600;
}
QLabel[role="branchBadge"] {
    background: #EDF2FA;
    color: #285EBD;
    border: 1px solid #D5DFEF;
    border-radius: 5px;
    padding: 4px 8px;
    font-weight: 600;
}
QLabel#workspaceStatusLabel {
    padding: 7px 10px;
    border-radius: 6px;
    background: #ECEFF2;
    color: #626A75;
}
QLabel#workspaceStatusLabel[statusState="success"] {
    background: #EEF7F2;
    color: #28724D;
}
QLabel#workspaceStatusLabel[statusState="error"] {
    background: #FAF0F0;
    color: #A33A3A;
}
QLabel#workspaceStatusLabel[statusState="loading"] {
    background: #EEF3FA;
    color: #285EBD;
}
QFrame#workspaceWorktreeStatusCard {
    background: #F7F8FA;
    border: 1px solid #DDE1E6;
    border-left: 3px solid #8B929C;
    border-radius: 6px;
}
QFrame#workspaceWorktreeStatusCard[worktreeState="clean"] {
    background: #F0F7F3;
    border-color: #C9DED1;
    border-left-color: #3F8A61;
}
QFrame#workspaceWorktreeStatusCard[worktreeState="dirty"] {
    background: #FBF6EE;
    border-color: #E7D3B6;
    border-left-color: #B87828;
}
QLabel#workspaceWorktreeStatusTitle {
    color: #555D68;
    font-weight: 650;
}
QFrame#workspaceWorktreeStatusCard[worktreeState="clean"] QLabel#workspaceWorktreeStatusTitle {
    color: #2E714E;
}
QFrame#workspaceWorktreeStatusCard[worktreeState="dirty"] QLabel#workspaceWorktreeStatusTitle {
    color: #8D581B;
    font-size: 13px;
}
QLabel#workspaceWorktreeStatusDetails {
    color: #717985;
    font-size: 12px;
}
QFrame#workspaceWorktreeStatusCard[worktreeState="dirty"] QLabel#workspaceWorktreeStatusDetails {
    color: #7C5B31;
}
QLabel#validationSummary[validationState="ready"] {
    color: #2D7651;
}
QLabel#validationSummary[validationState="error"] {
    color: #A63D3D;
}
QLineEdit, QComboBox {
    min-height: 22px;
    padding: 7px 9px;
    background: #FBFBFC;
    color: #20242A;
    border: 1px solid #C9CED6;
    border-radius: 8px;
    selection-background-color: #DCE7F8;
}
QLineEdit:hover, QComboBox:hover {
    background: #FFFFFF;
    border-color: #AAB1BB;
}
QLineEdit:focus, QComboBox:focus {
    background: #FFFFFF;
    border-color: #2F6FEB;
}
QLineEdit:disabled, QComboBox:disabled {
    background: #EFF1F3;
    color: #969CA5;
    border-color: #DDE1E6;
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
    color: #20242A;
    border: 1px solid #C9CED6;
    border-radius: 8px;
    padding: 3px;
    selection-background-color: #E8EEF8;
    selection-color: #20242A;
    outline: none;
}
QComboBox[lookupState="loading"] {
    border-color: #6F96D8;
}
QComboBox[lookupState="error"] {
    border-color: #B9823E;
}
QPushButton {
    min-height: 22px;
    padding: 7px 12px;
    border-radius: 6px;
    border: 1px solid #C8CDD5;
    background: #FFFFFF;
    color: #3E454F;
    font-weight: 550;
}
QPushButton:hover {
    background: #F6F7F8;
    border-color: #A9B0BA;
}
QPushButton:pressed {
    background: #E9EBEE;
}
QPushButton:disabled {
    background: #EFF1F3;
    color: #9BA1AA;
    border-color: #DDE1E6;
}
QPushButton[buttonRole="primary"] {
    background: #2F6FEB;
    color: #FFFFFF;
    border-color: #2F6FEB;
    padding-left: 17px;
    padding-right: 17px;
}
QPushButton[buttonRole="primary"]:hover {
    background: #285FCA;
    border-color: #285FCA;
}
QPushButton[buttonRole="primary"]:disabled {
    background: #C9D7EF;
    color: #F8FAFD;
    border-color: #C9D7EF;
}
QPushButton[buttonRole="accentGhost"] {
    color: #285EBD;
    border-color: transparent;
    background: transparent;
}
QPushButton[buttonRole="accentGhost"]:hover {
    background: #E9EEF7;
    border-color: #D5DFEF;
}
QPushButton[buttonRole="dangerGhost"] {
    color: #A84545;
    border-color: transparent;
    background: transparent;
    padding: 4px 7px;
}
QPushButton[buttonRole="dangerGhost"]:hover {
    background: #F8EDED;
    border-color: #E8CACA;
}
QPlainTextEdit {
    background: #151A22;
    color: #D7DCE5;
    border: 1px solid #252C36;
    border-radius: 6px;
    padding: 9px;
    selection-background-color: #315FAD;
    font-family: "Menlo";
    font-size: 11px;
}
QListWidget, QTabWidget::pane {
    background: #FFFFFF;
    color: #20242A;
    border: 1px solid #D5D9DF;
    border-radius: 6px;
    outline: none;
}
QListWidget::item {
    min-height: 25px;
    padding: 3px 7px;
    border-radius: 4px;
}
QListWidget::item:selected {
    background: #E8EEF8;
    color: #20242A;
}
QTreeWidget#workspaceRepositoryTree {
    background: #FFFFFF;
    color: #20242A;
    border: 1px solid #D5D9DF;
    border-radius: 6px;
    padding: 3px 2px 3px 3px;
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
    margin: 4px 2px 4px 0px;
    background: transparent;
}
QTreeWidget#workspaceRepositoryTree QScrollBar::handle:vertical {
    min-height: 36px;
    background: #C8CDD4;
    border-radius: 3px;
}
QTreeWidget#workspaceRepositoryTree QScrollBar::handle:vertical:hover {
    background: #A9B0BA;
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
    padding: 7px 11px;
    color: #717782;
    border-bottom: 2px solid transparent;
}
QTabBar::tab:hover {
    color: #3E454F;
}
QTabBar::tab:selected {
    color: #285EBD;
    border-bottom-color: #2F6FEB;
}
QProgressBar {
    min-height: 5px;
    max-height: 5px;
    background: #DDE4EE;
    border: none;
    border-radius: 2px;
    text-align: center;
}
QProgressBar::chunk {
    background: #4C79C7;
    border-radius: 2px;
}
QScrollArea {
    border: none;
    background: transparent;
}
QSplitter::handle:vertical {
    height: 7px;
    margin: 2px 24px;
    background: #D6DAE0;
    border-radius: 2px;
}
QSplitter::handle:vertical:hover {
    background: #AEB7C4;
}
QWidget#executionSummaryContainer, QWidget#configurationScrollContent {
    background: transparent;
}
QScrollBar:vertical {
    width: 8px;
    margin: 2px;
    background: transparent;
}
QScrollBar::handle:vertical {
    min-height: 30px;
    background: #C5CAD2;
    border-radius: 3px;
}
QScrollBar::handle:vertical:hover {
    background: #A8AFB9;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QToolTip {
    background: #252A31;
    color: #FFFFFF;
    border: none;
    padding: 5px;
}
)");
}

} // namespace gitclone
