#include "LoginDialog.h"
#include "DatabaseManager.h"
#include <QVBoxLayout>
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent), m_isLoginMode(true) {

    setFixedSize(320, 360);
    setWindowTitle("Авторизация");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(30, 40, 30, 40);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("Номер телефона (например, +7...)");
    m_usernameEdit->setFixedHeight(35);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("Пароль");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setFixedHeight(35);

    m_actionButton = new QPushButton(this);
    m_actionButton->setObjectName("actionButton");
    m_actionButton->setFixedHeight(40);

    m_switchModeButton = new QPushButton(this);
    m_switchModeButton->setObjectName("switchButton");
    m_switchModeButton->setFlat(true);
    m_switchModeButton->setFixedHeight(30);

    layout->addWidget(m_titleLabel);
    layout->addSpacing(10);
    layout->addWidget(m_usernameEdit);
    layout->addWidget(m_passwordEdit);
    layout->addWidget(m_actionButton);
    layout->addWidget(m_switchModeButton);
    layout->addStretch();

    setStyleSheet(
        "QDialog { background-color: #0e1621; }"
        "QLabel { color: white; }"
        "QLineEdit { background-color: #17212b; border: 1px solid #24303f; color: white; border-radius: 5px; padding-left: 10px; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #5288c1; }"
        "QPushButton#actionButton { background-color: #5288c1; color: white; border-radius: 5px; font-weight: bold; font-size: 14px; border: none; }"
        "QPushButton#actionButton:hover { background-color: #659bdf; }"
        "QPushButton#switchButton { color: #5288c1; font-size: 12px; border: none; background: transparent; }"
        "QPushButton#switchButton:hover { color: #659bdf; text-decoration: underline; }"
    );

    updateUI();

    connect(m_actionButton, &QPushButton::clicked, this, &LoginDialog::onActionButtonClicked);
    connect(m_switchModeButton, &QPushButton::clicked, this, &LoginDialog::onSwitchModeClicked);
}

QString LoginDialog::getUsername() const {
    return m_usernameEdit->text();
}

void LoginDialog::updateUI() {
    if (m_isLoginMode) {
        m_titleLabel->setText("Вход");
        m_actionButton->setText("Войти");
        m_switchModeButton->setText("Нет аккаунта? Зарегистрироваться");
    }
    else {
        m_titleLabel->setText("Регистрация");
        m_actionButton->setText("Создать аккаунт");
        m_switchModeButton->setText("Уже есть аккаунт? Войти");
    }
}

void LoginDialog::onSwitchModeClicked() {
    m_isLoginMode = !m_isLoginMode;
    m_usernameEdit->clear();
    m_passwordEdit->clear();
    updateUI();
}

void LoginDialog::onActionButtonClicked() {
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Заполните все поля!");
        return;
    }

    if (m_isLoginMode) {
        if (DatabaseManager::instance().loginUser(username, password)) {
            accept();
        }
        else {
            QMessageBox::critical(this, "Ошибка", "Неверное имя пользователя или пароль.");
        }
    }
    else {
        if (DatabaseManager::instance().registerUser(username, password)) {
            QMessageBox::information(this, "Успех", "Регистрация завершена! Теперь вы можете войти.");
            onSwitchModeClicked();
        }
        else {
            QMessageBox::critical(this, "Ошибка", "Пользователь с таким именем уже существует.");
        }
    }
}