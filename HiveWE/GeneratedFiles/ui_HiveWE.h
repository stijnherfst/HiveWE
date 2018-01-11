/********************************************************************************
** Form generated from reading UI file 'HiveWE.ui'
**
** Created by: Qt User Interface Compiler version 5.10.0
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
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "glwidget.h"

QT_BEGIN_NAMESPACE

class Ui_HiveWEClass
{
public:
    QAction *actionConvert_Blockers_to_Pathmap;
    QAction *actionOpen;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    GLWidget *widget;
    QMenuBar *menuBar;
    QMenu *menuTools;
    QMenu *menuPathing;
    QMenu *menuFile;

    void setupUi(QMainWindow *HiveWEClass)
    {
        if (HiveWEClass->objectName().isEmpty())
            HiveWEClass->setObjectName(QStringLiteral("HiveWEClass"));
        HiveWEClass->resize(600, 400);
        actionConvert_Blockers_to_Pathmap = new QAction(HiveWEClass);
        actionConvert_Blockers_to_Pathmap->setObjectName(QStringLiteral("actionConvert_Blockers_to_Pathmap"));
        actionOpen = new QAction(HiveWEClass);
        actionOpen->setObjectName(QStringLiteral("actionOpen"));
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
        menuBar = new QMenuBar(HiveWEClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 600, 21));
        menuTools = new QMenu(menuBar);
        menuTools->setObjectName(QStringLiteral("menuTools"));
        menuPathing = new QMenu(menuTools);
        menuPathing->setObjectName(QStringLiteral("menuPathing"));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        HiveWEClass->setMenuBar(menuBar);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuTools->menuAction());
        menuTools->addAction(menuPathing->menuAction());
        menuPathing->addAction(actionConvert_Blockers_to_Pathmap);
        menuFile->addAction(actionOpen);

        retranslateUi(HiveWEClass);

        QMetaObject::connectSlotsByName(HiveWEClass);
    } // setupUi

    void retranslateUi(QMainWindow *HiveWEClass)
    {
        HiveWEClass->setWindowTitle(QApplication::translate("HiveWEClass", "HiveWE", nullptr));
        actionConvert_Blockers_to_Pathmap->setText(QApplication::translate("HiveWEClass", "Convert Blockers to Pathmap", nullptr));
        actionOpen->setText(QApplication::translate("HiveWEClass", "Open", nullptr));
        menuTools->setTitle(QApplication::translate("HiveWEClass", "Tools", nullptr));
        menuPathing->setTitle(QApplication::translate("HiveWEClass", "Pathing", nullptr));
        menuFile->setTitle(QApplication::translate("HiveWEClass", "File", nullptr));
    } // retranslateUi

};

namespace Ui {
    class HiveWEClass: public Ui_HiveWEClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HIVEWE_H
