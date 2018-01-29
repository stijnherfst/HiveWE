/********************************************************************************
** Form generated from reading UI file 'PathingPallete.ui'
**
** Created by: Qt User Interface Compiler version 5.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PATHINGPALLETE_H
#define UI_PATHINGPALLETE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PathingPallete
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *BrushOperationLabel;
    QHBoxLayout *ToolTypeLayout;
    QRadioButton *ReplaceType;
    QRadioButton *AddType;
    QRadioButton *RemoveType;
    QLabel *BrushTypeLabel;
    QHBoxLayout *horizontalLayout;
    QPushButton *Walkable;
    QPushButton *Flyable;
    QPushButton *Buildable;
    QLabel *BrushSizeLabel;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *brushSize1;
    QPushButton *brushSize3;
    QPushButton *brushSize5;
    QPushButton *brushSize7;
    QPushButton *brushSize9;
    QPushButton *brushSize11;
    QHBoxLayout *horizontalLayout_3;
    QSpinBox *spinBox;
    QSlider *brushSize;
    QButtonGroup *ToolTypeGroup;
    QButtonGroup *BrushSizeGroup;

    void setupUi(QWidget *PathingPallete)
    {
        if (PathingPallete->objectName().isEmpty())
            PathingPallete->setObjectName(QStringLiteral("PathingPallete"));
        PathingPallete->setWindowModality(Qt::NonModal);
        PathingPallete->resize(246, 217);
        PathingPallete->setContextMenuPolicy(Qt::DefaultContextMenu);
        PathingPallete->setWindowOpacity(1);
        verticalLayout = new QVBoxLayout(PathingPallete);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        BrushOperationLabel = new QLabel(PathingPallete);
        BrushOperationLabel->setObjectName(QStringLiteral("BrushOperationLabel"));

        verticalLayout->addWidget(BrushOperationLabel);

        ToolTypeLayout = new QHBoxLayout();
        ToolTypeLayout->setSpacing(6);
        ToolTypeLayout->setObjectName(QStringLiteral("ToolTypeLayout"));
        ReplaceType = new QRadioButton(PathingPallete);
        ToolTypeGroup = new QButtonGroup(PathingPallete);
        ToolTypeGroup->setObjectName(QStringLiteral("ToolTypeGroup"));
        ToolTypeGroup->addButton(ReplaceType);
        ReplaceType->setObjectName(QStringLiteral("ReplaceType"));

        ToolTypeLayout->addWidget(ReplaceType);

        AddType = new QRadioButton(PathingPallete);
        ToolTypeGroup->addButton(AddType);
        AddType->setObjectName(QStringLiteral("AddType"));

        ToolTypeLayout->addWidget(AddType);

        RemoveType = new QRadioButton(PathingPallete);
        ToolTypeGroup->addButton(RemoveType);
        RemoveType->setObjectName(QStringLiteral("RemoveType"));

        ToolTypeLayout->addWidget(RemoveType);


        verticalLayout->addLayout(ToolTypeLayout);

        BrushTypeLabel = new QLabel(PathingPallete);
        BrushTypeLabel->setObjectName(QStringLiteral("BrushTypeLabel"));

        verticalLayout->addWidget(BrushTypeLabel);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        Walkable = new QPushButton(PathingPallete);
        Walkable->setObjectName(QStringLiteral("Walkable"));
        QIcon icon;
        icon.addFile(QStringLiteral("Data/Icons/walkable.png"), QSize(), QIcon::Normal, QIcon::Off);
        Walkable->setIcon(icon);
        Walkable->setIconSize(QSize(32, 32));
        Walkable->setCheckable(true);

        horizontalLayout->addWidget(Walkable);

        Flyable = new QPushButton(PathingPallete);
        Flyable->setObjectName(QStringLiteral("Flyable"));
        QIcon icon1;
        icon1.addFile(QStringLiteral("Data/Icons/flyable.png"), QSize(), QIcon::Normal, QIcon::Off);
        Flyable->setIcon(icon1);
        Flyable->setIconSize(QSize(32, 32));
        Flyable->setCheckable(true);

        horizontalLayout->addWidget(Flyable);

        Buildable = new QPushButton(PathingPallete);
        Buildable->setObjectName(QStringLiteral("Buildable"));
        QIcon icon2;
        icon2.addFile(QStringLiteral("Data/Icons/buildable.png"), QSize(), QIcon::Normal, QIcon::Off);
        Buildable->setIcon(icon2);
        Buildable->setIconSize(QSize(32, 32));
        Buildable->setCheckable(true);

        horizontalLayout->addWidget(Buildable);


        verticalLayout->addLayout(horizontalLayout);

        BrushSizeLabel = new QLabel(PathingPallete);
        BrushSizeLabel->setObjectName(QStringLiteral("BrushSizeLabel"));

        verticalLayout->addWidget(BrushSizeLabel);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        brushSize1 = new QPushButton(PathingPallete);
        BrushSizeGroup = new QButtonGroup(PathingPallete);
        BrushSizeGroup->setObjectName(QStringLiteral("BrushSizeGroup"));
        BrushSizeGroup->addButton(brushSize1);
        brushSize1->setObjectName(QStringLiteral("brushSize1"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(brushSize1->sizePolicy().hasHeightForWidth());
        brushSize1->setSizePolicy(sizePolicy);
        brushSize1->setMinimumSize(QSize(32, 32));
        brushSize1->setBaseSize(QSize(0, 0));
        brushSize1->setCheckable(true);

        horizontalLayout_2->addWidget(brushSize1);

        brushSize3 = new QPushButton(PathingPallete);
        BrushSizeGroup->addButton(brushSize3);
        brushSize3->setObjectName(QStringLiteral("brushSize3"));
        brushSize3->setMinimumSize(QSize(32, 32));
        brushSize3->setCheckable(true);

        horizontalLayout_2->addWidget(brushSize3);

        brushSize5 = new QPushButton(PathingPallete);
        BrushSizeGroup->addButton(brushSize5);
        brushSize5->setObjectName(QStringLiteral("brushSize5"));
        brushSize5->setMinimumSize(QSize(32, 32));

        horizontalLayout_2->addWidget(brushSize5);

        brushSize7 = new QPushButton(PathingPallete);
        BrushSizeGroup->addButton(brushSize7);
        brushSize7->setObjectName(QStringLiteral("brushSize7"));
        brushSize7->setMinimumSize(QSize(32, 32));

        horizontalLayout_2->addWidget(brushSize7);

        brushSize9 = new QPushButton(PathingPallete);
        BrushSizeGroup->addButton(brushSize9);
        brushSize9->setObjectName(QStringLiteral("brushSize9"));
        brushSize9->setMinimumSize(QSize(32, 32));

        horizontalLayout_2->addWidget(brushSize9);

        brushSize11 = new QPushButton(PathingPallete);
        BrushSizeGroup->addButton(brushSize11);
        brushSize11->setObjectName(QStringLiteral("brushSize11"));
        brushSize11->setMinimumSize(QSize(32, 32));

        horizontalLayout_2->addWidget(brushSize11);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        spinBox = new QSpinBox(PathingPallete);
        spinBox->setObjectName(QStringLiteral("spinBox"));
        spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);

        horizontalLayout_3->addWidget(spinBox);

        brushSize = new QSlider(PathingPallete);
        brushSize->setObjectName(QStringLiteral("brushSize"));
        brushSize->setMinimum(1);
        brushSize->setMaximum(241);
        brushSize->setSingleStep(2);
        brushSize->setOrientation(Qt::Horizontal);
        brushSize->setInvertedAppearance(false);
        brushSize->setInvertedControls(false);
        brushSize->setTickPosition(QSlider::TicksBelow);
        brushSize->setTickInterval(0);

        horizontalLayout_3->addWidget(brushSize);


        verticalLayout->addLayout(horizontalLayout_3);

        QWidget::setTabOrder(ReplaceType, AddType);
        QWidget::setTabOrder(AddType, RemoveType);
        QWidget::setTabOrder(RemoveType, Walkable);
        QWidget::setTabOrder(Walkable, Flyable);
        QWidget::setTabOrder(Flyable, Buildable);
        QWidget::setTabOrder(Buildable, brushSize1);
        QWidget::setTabOrder(brushSize1, brushSize3);
        QWidget::setTabOrder(brushSize3, brushSize5);
        QWidget::setTabOrder(brushSize5, brushSize7);
        QWidget::setTabOrder(brushSize7, brushSize9);
        QWidget::setTabOrder(brushSize9, brushSize11);
        QWidget::setTabOrder(brushSize11, spinBox);
        QWidget::setTabOrder(spinBox, brushSize);

        retranslateUi(PathingPallete);
        QObject::connect(brushSize, SIGNAL(valueChanged(int)), spinBox, SLOT(setValue(int)));

        QMetaObject::connectSlotsByName(PathingPallete);
    } // setupUi

    void retranslateUi(QWidget *PathingPallete)
    {
        PathingPallete->setWindowTitle(QApplication::translate("PathingPallete", "Pathing Pallete", nullptr));
        BrushOperationLabel->setText(QApplication::translate("PathingPallete", "Brush Operation Type", nullptr));
        ReplaceType->setText(QApplication::translate("PathingPallete", "Replace", nullptr));
        AddType->setText(QApplication::translate("PathingPallete", "Add", nullptr));
        RemoveType->setText(QApplication::translate("PathingPallete", "Remove", nullptr));
        BrushTypeLabel->setText(QApplication::translate("PathingPallete", "Brush Type", nullptr));
        Walkable->setText(QString());
        Flyable->setText(QString());
        Buildable->setText(QString());
        BrushSizeLabel->setText(QApplication::translate("PathingPallete", "Brush Size", nullptr));
        brushSize1->setText(QApplication::translate("PathingPallete", "1", nullptr));
        brushSize3->setText(QApplication::translate("PathingPallete", "3", nullptr));
        brushSize5->setText(QApplication::translate("PathingPallete", "5", nullptr));
        brushSize7->setText(QApplication::translate("PathingPallete", "7", nullptr));
        brushSize9->setText(QApplication::translate("PathingPallete", "9", nullptr));
        brushSize11->setText(QApplication::translate("PathingPallete", "11", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PathingPallete: public Ui_PathingPallete {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PATHINGPALLETE_H
