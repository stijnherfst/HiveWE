/********************************************************************************
** Form generated from reading UI file 'TileSetter.ui'
**
** Created by: Qt User Interface Compiler version 5.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TILESETTER_H
#define UI_TILESETTER_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TileSetter
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *selectedTilesLabel;
    QLabel *selectedTileLabel;
    QHBoxLayout *flowlayout_placeholder_1;
    QHBoxLayout *horizontalLayout;
    QPushButton *selectedShiftLeft;
    QPushButton *selectedShiftRight;
    QPushButton *selectedRemove;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout_3;
    QLabel *additionalTilesLabel;
    QComboBox *tileset;
    QLabel *additionalTileLabel;
    QHBoxLayout *flowlayout_placeholder_2;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *additionalAdd;
    QSpacerItem *horizontalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QWidget *TileSetter)
    {
        if (TileSetter->objectName().isEmpty())
            TileSetter->setObjectName(QStringLiteral("TileSetter"));
        TileSetter->setWindowModality(Qt::ApplicationModal);
        TileSetter->resize(579, 207);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(TileSetter->sizePolicy().hasHeightForWidth());
        TileSetter->setSizePolicy(sizePolicy);
        TileSetter->setMinimumSize(QSize(0, 0));
        verticalLayout = new QVBoxLayout(TileSetter);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        selectedTilesLabel = new QLabel(TileSetter);
        selectedTilesLabel->setObjectName(QStringLiteral("selectedTilesLabel"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(selectedTilesLabel->sizePolicy().hasHeightForWidth());
        selectedTilesLabel->setSizePolicy(sizePolicy1);

        verticalLayout->addWidget(selectedTilesLabel);

        selectedTileLabel = new QLabel(TileSetter);
        selectedTileLabel->setObjectName(QStringLiteral("selectedTileLabel"));
        sizePolicy1.setHeightForWidth(selectedTileLabel->sizePolicy().hasHeightForWidth());
        selectedTileLabel->setSizePolicy(sizePolicy1);

        verticalLayout->addWidget(selectedTileLabel);

        flowlayout_placeholder_1 = new QHBoxLayout();
        flowlayout_placeholder_1->setSpacing(6);
        flowlayout_placeholder_1->setObjectName(QStringLiteral("flowlayout_placeholder_1"));
        flowlayout_placeholder_1->setSizeConstraint(QLayout::SetDefaultConstraint);

        verticalLayout->addLayout(flowlayout_placeholder_1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        selectedShiftLeft = new QPushButton(TileSetter);
        selectedShiftLeft->setObjectName(QStringLiteral("selectedShiftLeft"));

        horizontalLayout->addWidget(selectedShiftLeft);

        selectedShiftRight = new QPushButton(TileSetter);
        selectedShiftRight->setObjectName(QStringLiteral("selectedShiftRight"));

        horizontalLayout->addWidget(selectedShiftRight);

        selectedRemove = new QPushButton(TileSetter);
        selectedRemove->setObjectName(QStringLiteral("selectedRemove"));

        horizontalLayout->addWidget(selectedRemove);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        additionalTilesLabel = new QLabel(TileSetter);
        additionalTilesLabel->setObjectName(QStringLiteral("additionalTilesLabel"));
        sizePolicy1.setHeightForWidth(additionalTilesLabel->sizePolicy().hasHeightForWidth());
        additionalTilesLabel->setSizePolicy(sizePolicy1);

        horizontalLayout_3->addWidget(additionalTilesLabel);

        tileset = new QComboBox(TileSetter);
        tileset->setObjectName(QStringLiteral("tileset"));
        tileset->setMaxVisibleItems(20);

        horizontalLayout_3->addWidget(tileset);


        verticalLayout->addLayout(horizontalLayout_3);

        additionalTileLabel = new QLabel(TileSetter);
        additionalTileLabel->setObjectName(QStringLiteral("additionalTileLabel"));
        sizePolicy1.setHeightForWidth(additionalTileLabel->sizePolicy().hasHeightForWidth());
        additionalTileLabel->setSizePolicy(sizePolicy1);

        verticalLayout->addWidget(additionalTileLabel);

        flowlayout_placeholder_2 = new QHBoxLayout();
        flowlayout_placeholder_2->setSpacing(6);
        flowlayout_placeholder_2->setObjectName(QStringLiteral("flowlayout_placeholder_2"));
        flowlayout_placeholder_2->setSizeConstraint(QLayout::SetDefaultConstraint);

        verticalLayout->addLayout(flowlayout_placeholder_2);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        additionalAdd = new QPushButton(TileSetter);
        additionalAdd->setObjectName(QStringLiteral("additionalAdd"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(additionalAdd->sizePolicy().hasHeightForWidth());
        additionalAdd->setSizePolicy(sizePolicy2);

        horizontalLayout_2->addWidget(additionalAdd);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout_2);

        buttonBox = new QDialogButtonBox(TileSetter);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(TileSetter);

        QMetaObject::connectSlotsByName(TileSetter);
    } // setupUi

    void retranslateUi(QWidget *TileSetter)
    {
        TileSetter->setWindowTitle(QApplication::translate("TileSetter", "TileSetter", nullptr));
#ifndef QT_NO_TOOLTIP
        selectedTilesLabel->setToolTip(QApplication::translate("TileSetter", "The tiles in the map tileset.", nullptr));
#endif // QT_NO_TOOLTIP
        selectedTilesLabel->setText(QApplication::translate("TileSetter", "<html><head/><body><p><span style=\" font-size:10pt; font-weight:600;\">Selected Tiles</span></p></body></html>", nullptr));
#ifndef QT_NO_TOOLTIP
        selectedTileLabel->setToolTip(QApplication::translate("TileSetter", "The current tilename.", nullptr));
#endif // QT_NO_TOOLTIP
        selectedTileLabel->setText(QApplication::translate("TileSetter", "Tile: ", nullptr));
#ifndef QT_NO_TOOLTIP
        selectedShiftLeft->setToolTip(QApplication::translate("TileSetter", "Shifts the selected tile left.", nullptr));
#endif // QT_NO_TOOLTIP
        selectedShiftLeft->setText(QApplication::translate("TileSetter", "Shift Left", nullptr));
#ifndef QT_NO_TOOLTIP
        selectedShiftRight->setToolTip(QApplication::translate("TileSetter", "Shift the selected tile right.", nullptr));
#endif // QT_NO_TOOLTIP
        selectedShiftRight->setText(QApplication::translate("TileSetter", "Shift Right", nullptr));
#ifndef QT_NO_TOOLTIP
        selectedRemove->setToolTip(QApplication::translate("TileSetter", "Removes the selected tile from the tleset.", nullptr));
#endif // QT_NO_TOOLTIP
        selectedRemove->setText(QApplication::translate("TileSetter", "Remove", nullptr));
#ifndef QT_NO_TOOLTIP
        additionalTilesLabel->setToolTip(QApplication::translate("TileSetter", "Tiles you can add to the map tileset.", nullptr));
#endif // QT_NO_TOOLTIP
        additionalTilesLabel->setText(QApplication::translate("TileSetter", "<html><head/><body><p><span style=\" font-size:10pt; font-weight:600;\">Additonal Tiles</span></p></body></html>", nullptr));
#ifndef QT_NO_TOOLTIP
        tileset->setToolTip(QApplication::translate("TileSetter", "The tileset to choose additional tiles from.", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        additionalTileLabel->setToolTip(QApplication::translate("TileSetter", "The current tilename.", nullptr));
#endif // QT_NO_TOOLTIP
        additionalTileLabel->setText(QApplication::translate("TileSetter", "Tile:", nullptr));
#ifndef QT_NO_TOOLTIP
        additionalAdd->setToolTip(QApplication::translate("TileSetter", "Adds the selected additional tile to the map tileset.", nullptr));
#endif // QT_NO_TOOLTIP
        additionalAdd->setText(QApplication::translate("TileSetter", "Add", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TileSetter: public Ui_TileSetter {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TILESETTER_H
