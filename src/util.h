#pragma once
#include <QtWidgets/QMessageBox>

static QMessageBox* _OpenMessageBox_messageBox = nullptr;

// Rough equivalent of the QMessageBox::warning, etc. methods, which unfortunately don't generate modals on macOS.
static void OpenMessageBox (QWidget* parent, QMessageBox::Icon icon, const QString& title, const QString& text) {
    if (_OpenMessageBox_messageBox == nullptr) {
        _OpenMessageBox_messageBox = new QMessageBox();
    }
    _OpenMessageBox_messageBox->setParent(parent);
    _OpenMessageBox_messageBox->setWindowTitle(title);
    _OpenMessageBox_messageBox->setText(text);
    _OpenMessageBox_messageBox->setIcon(icon);
    _OpenMessageBox_messageBox->setWindowModality(Qt::WindowModal);
    _OpenMessageBox_messageBox->show();
}