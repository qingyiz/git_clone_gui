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
QLabel#validationSummary[validationState="ready"] {
    color: #047857;
}
QLabel#validationSummary[validationState="error"] {
    color: #B91C1C;
}
QLineEdit {
    min-height: 22px;
    padding: 8px 10px;
    background: #F8FAFC;
    color: #0F172A;
    border: 1px solid #CBD5E1;
    border-radius: 8px;
    selection-background-color: #BFDBFE;
}
QLineEdit:hover {
    border-color: #94A3B8;
}
QLineEdit:focus {
    background: #FFFFFF;
    border: 2px solid #3B82F6;
    padding: 7px 9px;
}
QLineEdit:disabled {
    background: #F1F5F9;
    color: #94A3B8;
    border-color: #E2E8F0;
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
