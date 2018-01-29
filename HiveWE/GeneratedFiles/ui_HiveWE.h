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
    QAction *actionConvert_Tile_Type_to_PathMap;
    QAction *actionPathing_Pallete;
    QAction *actionSave;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    GLWidget *widget;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuWindow;
    QMenu *menuNew_Pallete;

    void setupUi(QMainWindow *HiveWEClass)
    {
        if (HiveWEClass->objectName().isEmpty())
            HiveWEClass->setObjectName(QStringLiteral("HiveWEClass"));
        HiveWEClass->resize(600, 400);
        actionConvert_Blockers_to_Pathmap = new QAction(HiveWEClass);
        actionConvert_Blockers_to_Pathmap->setObjectName(QStringLiteral("actionConvert_Blockers_to_Pathmap"));
        actionOpen = new QAction(HiveWEClass);
        actionOpen->setObjectName(QStringLiteral("actionOpen"));
        actionConvert_Tile_Type_to_PathMap = new QAction(HiveWEClass);
        actionConvert_Tile_Type_to_PathMap->setObjectName(QStringLiteral("actionConvert_Tile_Type_to_PathMap"));
        actionPathing_Pallete = new QAction(HiveWEClass);
        actionPathing_Pallete->setObjectName(QStringLiteral("actionPathing_Pallete"));
        actionSave = new QAction(HiveWEClass);
        actionSave->setObjectName(QStringLiteral("actionSave"));
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
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        menuWindow = new QMenu(menuBar);
        menuWindow->setObjectName(QStringLiteral("menuWindow"));
        menuNew_Pallete = new QMenu(menuWindow);
        menuNew_Pallete->setObjectName(QStringLiteral("menuNew_Pallete"));
        HiveWEClass->setMenuBar(menuBar);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuWindow->menuAction());
        menuFile->addAction(actionOpen);
        menuFile->addAction(actionSave);
        menuWindow->addAction(menuNew_Pallete->menuAction());
        menuNew_Pallete->addAction(actionPathing_Pallete);

        retranslateUi(HiveWEClass);

        QMetaObject::connectSlotsByName(HiveWEClass);
    } // setupUi

    void retranslateUi(QMainWindow *HiveWEClass)
    {
        HiveWEClass->setWindowTitle(QApplication::translate("HiveWEClass", "HiveWE", nullptr));
        actionConvert_Blockers_to_Pathmap->setText(QApplication::translate("HiveWEClass", "Convert Blockers to Pathing Map", nullptr));
        actionOpen->setText(QApplication::translate("HiveWEClass", "Open", nullptr));
        actionConvert_Tile_Type_to_PathMap->setText(QApplication::translate("HiveWEClass", "Convert Tile Type to Pathing Map", nullptr));
        actionPathing_Pallete->setText(QApplication::translate("HiveWEClass", "Pathing", nullptr));
        actionSave->setText(QApplication::translate("HiveWEClass", "Save", nullptr));
        menuFile->setTitle(QApplication::translate("HiveWEClass", "File", nullptr));
        menuWindow->setTitle(QApplication::translate("HiveWEClass", "Window", nullptr));
        menuNew_Pallete->setTitle(QApplication::translate("HiveWEClass", "New Pallete", nullptr));
    } // retranslateUi

};

namespace Ui {
    class HiveWEClass: public Ui_HiveWEClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HIVEWE_H
