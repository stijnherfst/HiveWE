/********************************************************************************
** Form generated from reading UI file 'TilePicker.ui'
**
** Created by: Qt User Interface Compiler version 5.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TILEPICKER_H
#define UI_TILEPICKER_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TilePicker
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QLabel *selectedTileLabel;
    QHBoxLayout *flowlayout_placeholder_1;
    QLabel *availableTilesLabel;
    QLabel *replacingTileLabel;
    QHBoxLayout *flowlayout_placeholder_2;
    QDialogButtonBox *buttonBox;

    void setupUi(QWidget *TilePicker)
    {
        if (TilePicker->objectName().isEmpty())
            TilePicker->setObjectName(QStringLiteral("TilePicker"));
        TilePicker->resize(579, 153);
        verticalLayout = new QVBoxLayout(TilePicker);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        label = new QLabel(TilePicker);
        label->setObjectName(QStringLiteral("label"));

        verticalLayout->addWidget(label);

        selectedTileLabel = new QLabel(TilePicker);
        selectedTileLabel->setObjectName(QStringLiteral("selectedTileLabel"));

        verticalLayout->addWidget(selectedTileLabel);

        flowlayout_placeholder_1 = new QHBoxLayout();
        flowlayout_placeholder_1->setSpacing(6);
        flowlayout_placeholder_1->setObjectName(QStringLiteral("flowlayout_placeholder_1"));

        verticalLayout->addLayout(flowlayout_placeholder_1);

        availableTilesLabel = new QLabel(TilePicker);
        availableTilesLabel->setObjectName(QStringLiteral("availableTilesLabel"));

        verticalLayout->addWidget(availableTilesLabel);

        replacingTileLabel = new QLabel(TilePicker);
        replacingTileLabel->setObjectName(QStringLiteral("replacingTileLabel"));

        verticalLayout->addWidget(replacingTileLabel);

        flowlayout_placeholder_2 = new QHBoxLayout();
        flowlayout_placeholder_2->setSpacing(6);
        flowlayout_placeholder_2->setObjectName(QStringLiteral("flowlayout_placeholder_2"));

        verticalLayout->addLayout(flowlayout_placeholder_2);

        buttonBox = new QDialogButtonBox(TilePicker);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(TilePicker);

        QMetaObject::connectSlotsByName(TilePicker);
    } // setupUi

    void retranslateUi(QWidget *TilePicker)
    {
        TilePicker->setWindowTitle(QApplication::translate("TilePicker", "TilePicker", nullptr));
        label->setText(QApplication::translate("TilePicker", "<html><head/><body><p><span style=\" font-size:10pt; font-weight:600;\">Replace Tile:</span></p></body></html>", nullptr));
        selectedTileLabel->setText(QApplication::translate("TilePicker", "Tile:", nullptr));
        availableTilesLabel->setText(QApplication::translate("TilePicker", "<html><head/><body><p><span style=\" font-size:10pt; font-weight:600;\">By Tile:</span></p></body></html>", nullptr));
        replacingTileLabel->setText(QApplication::translate("TilePicker", "Tile:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TilePicker: public Ui_TilePicker {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TILEPICKER_H
