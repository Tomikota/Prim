#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr);
    QString getUsername() const;

private slots:
    void onActionButtonClicked();
    void onSwitchModeClicked();

private:
    QLabel* m_titleLabel;
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QPushButton* m_actionButton;
    QPushButton* m_switchModeButton;

    bool m_isLoginMode;
    void updateUI();
};