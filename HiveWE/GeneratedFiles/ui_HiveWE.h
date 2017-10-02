/********************************************************************************
** Form generated from reading UI file 'HiveWE.ui'
**
** Created by: Qt User Interface Compiler version 5.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_HIVEWE_H
#define UI_HIVEWE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "glwidget.h"

QT_BEGIN_NAMESPACE

class Ui_HiveWEClass
{
public:
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    GLWidget *widget;

    void setupUi(QMainWindow *HiveWEClass)
    {
        if (HiveWEClass->objectName().isEmpty())
            HiveWEClass->setObjectName(QStringLiteral("HiveWEClass"));
        HiveWEClass->resize(600, 400);
        centralWidget = new QWidget(HiveWEClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        widget = new GLWidget(centralWidget);
        widget->setObjectName(QStringLiteral("widget"));

        verticalLayout->addWidget(widget);

        HiveWEClass->setCentralWidget(centralWidget);

        retranslateUi(HiveWEClass);

        QMetaObject::connectSlotsByName(HiveWEClass);
    } // setupUi

    void retranslateUi(QMainWindow *HiveWEClass)
    {
        HiveWEClass->setWindowTitle(QApplication::translate("HiveWEClass", "HiveWE", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class HiveWEClass: public Ui_HiveWEClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HIVEWE_H
