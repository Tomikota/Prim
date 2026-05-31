#include "prim.h"
#include <QApplication>
#include <QMessageBox>
#include <QIcon>
#include "DatabaseManager.h"
#include "LoginDialog.h" 

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    a.setWindowIcon(QIcon(":/Prim/Prim.png"));

    if (!DatabaseManager::instance().init()) {
        QMessageBox::critical(nullptr, "Ошибка", "Не удалось запустить локальную базу данных SQLite.");
        return -1;
    }

    LoginDialog loginDlg;
    if (loginDlg.exec() == QDialog::Accepted) {
        Prim w;
        w.setUserName(loginDlg.getUsername());
        w.show();
        return a.exec();
    }

    return 0;
}