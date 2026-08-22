#include "qt_message_box.h"

#include <QMessageBox>
#include <QString>

void show_critical_error(
    std::string_view title,
    std::string_view message
) {
    const QString qtitle = QString::fromUtf8(
        title.data(),
        static_cast<qsizetype>(title.size())
    );

    const QString qmessage = QString::fromUtf8(
        message.data(),
        static_cast<qsizetype>(message.size())
    );

    QMessageBox::critical(nullptr, qtitle, qmessage);
}
