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
    QLabel *brushOperationLabel;
    QHBoxLayout *ToolTypeLayout;
    QRadioButton *replaceType;
    QRadioButton *addType;
    QRadioButton *removeType;
    QLabel *brushTypeLabel;
    QHBoxLayout *horizontalLayout;
    QPushButton *walkable;
    QPushButton *flyable;
    QPushButton *buildable;
    QLabel *brushSizeLabel;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *brushSize1;
    QPushButton *brushSize3;
    QPushButton *brushSize5;
    QPushButton *brushSize7;
    QPushButton *brushSize9;
    QPushButton *brushSize11;
    QHBoxLayout *horizontalLayout_3;
    QSpinBox *brushSize;
    QSlider *brushSizeSlider;
    QButtonGroup *brushSizeGroup;
    QButtonGroup *brushTypeGroup;
    QButtonGroup *toolTypeGroup;

    void setupUi(QWidget *PathingPallete)
    {
        if (PathingPallete->objectName().isEmpty())
            PathingPallete->setObjectName(QStringLiteral("PathingPallete"));
        PathingPallete->setWindowModality(Qt::NonModal);
        PathingPallete->resize(246, 217);
        PathingPallete->setWindowOpacity(1);
        verticalLayout = new QVBoxLayout(PathingPallete);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        brushOperationLabel = new QLabel(PathingPallete);
        brushOperationLabel->setObjectName(QStringLiteral("brushOperationLabel"));

        verticalLayout->addWidget(brushOperationLabel);

        ToolTypeLayout = new QHBoxLayout();
        ToolTypeLayout->setSpacing(6);
        ToolTypeLayout->setObjectName(QStringLiteral("ToolTypeLayout"));
        replaceType = new QRadioButton(PathingPallete);
        toolTypeGroup = new QButtonGroup(PathingPallete);
        toolTypeGroup->setObjectName(QStringLiteral("toolTypeGroup"));
        toolTypeGroup->addButton(replaceType);
        replaceType->setObjectName(QStringLiteral("replaceType"));
        replaceType->setChecked(true);

        ToolTypeLayout->addWidget(replaceType);

        addType = new QRadioButton(PathingPallete);
        toolTypeGroup->addButton(addType);
        addType->setObjectName(QStringLiteral("addType"));

        ToolTypeLayout->addWidget(addType);

        removeType = new QRadioButton(PathingPallete);
        toolTypeGroup->addButton(removeType);
        removeType->setObjectName(QStringLiteral("removeType"));

        ToolTypeLayout->addWidget(removeType);


        verticalLayout->addLayout(ToolTypeLayout);

        brushTypeLabel = new QLabel(PathingPallete);
        brushTypeLabel->setObjectName(QStringLiteral("brushTypeLabel"));

        verticalLayout->addWidget(brushTypeLabel);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        walkable = new QPushButton(PathingPallete);
        brushTypeGroup = new QButtonGroup(PathingPallete);
        brushTypeGroup->setObjectName(QStringLiteral("brushTypeGroup"));
        brushTypeGroup->setExclusive(false);
        brushTypeGroup->addButton(walkable);
        walkable->setObjectName(QStringLiteral("walkable"));
        QIcon icon;
        icon.addFile(QStringLiteral("Data/Icons/walkable.png"), QSize(), QIcon::Normal, QIcon::Off);
        walkable->setIcon(icon);
        walkable->setIconSize(QSize(32, 32));
        walkable->setCheckable(true);

        horizontalLayout->addWidget(walkable);

        flyable = new QPushButton(PathingPallete);
        brushTypeGroup->addButton(flyable);
        flyable->setObjectName(QStringLiteral("flyable"));
        QIcon icon1;
        icon1.addFile(QStringLiteral("Data/Icons/flyable.png"), QSize(), QIcon::Normal, QIcon::Off);
        flyable->setIcon(icon1);
        flyable->setIconSize(QSize(32, 32));
        flyable->setCheckable(true);

        horizontalLayout->addWidget(flyable);

        buildable = new QPushButton(PathingPallete);
        brushTypeGroup->addButton(buildable);
        buildable->setObjectName(QStringLiteral("buildable"));
        QIcon icon2;
        icon2.addFile(QStringLiteral("Data/Icons/buildable.png"), QSize(), QIcon::Normal, QIcon::Off);
        buildable->setIcon(icon2);
        buildable->setIconSize(QSize(32, 32));
        buildable->setCheckable(true);

        horizontalLayout->addWidget(buildable);


        verticalLayout->addLayout(horizontalLayout);

        brushSizeLabel = new QLabel(PathingPallete);
        brushSizeLabel->setObjectName(QStringLiteral("brushSizeLabel"));

        verticalLayout->addWidget(brushSizeLabel);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        brushSize1 = new QPushButton(PathingPallete);
        brushSizeGroup = new QButtonGroup(PathingPallete);
        brushSizeGroup->setObjectName(QStringLiteral("brushSizeGroup"));
        brushSizeGroup->addButton(brushSize1);
        brushSize1->setObjectName(QStringLiteral("brushSize1"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(brushSize1->sizePolicy().hasHeightForWidth());
        brushSize1->setSizePolicy(sizePolicy);
        brushSize1->setMinimumSize(QSize(32, 32));
        brushSize1->setBaseSize(QSize(0, 0));
        brushSize1->setCheckable(true);
        brushSize1->setChecked(false);

        horizontalLayout_2->addWidget(brushSize1);

        brushSize3 = new QPushButton(PathingPallete);
        brushSizeGroup->addButton(brushSize3);
        brushSize3->setObjectName(QStringLiteral("brushSize3"));
        brushSize3->setMinimumSize(QSize(32, 32));
        brushSize3->setCheckable(true);

        horizontalLayout_2->addWidget(brushSize3);

        brushSize5 = new QPushButton(PathingPallete);
        brushSizeGroup->addButton(brushSize5);
        brushSize5->setObjectName(QStringLiteral("brushSize5"));
        brushSize5->setMinimumSize(QSize(32, 32));
        brushSize5->setCheckable(true);

        horizontalLayout_2->addWidget(brushSize5);

        brushSize7 = new QPushButton(PathingPallete);
        brushSizeGroup->addButton(brushSize7);
        brushSize7->setObjectName(QStringLiteral("brushSize7"));
        brushSize7->setMinimumSize(QSize(32, 32));
        brushSize7->setCheckable(true);

        horizontalLayout_2->addWidget(brushSize7);

        brushSize9 = new QPushButton(PathingPallete);
        brushSizeGroup->addButton(brushSize9);
        brushSize9->setObjectName(QStringLiteral("brushSize9"));
        brushSize9->setMinimumSize(QSize(32, 32));
        brushSize9->setCheckable(true);

        horizontalLayout_2->addWidget(brushSize9);

        brushSize11 = new QPushButton(PathingPallete);
        brushSizeGroup->addButton(brushSize11);
        brushSize11->setObjectName(QStringLiteral("brushSize11"));
        brushSize11->setMinimumSize(QSize(32, 32));
        brushSize11->setCheckable(true);

        horizontalLayout_2->addWidget(brushSize11);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        brushSize = new QSpinBox(PathingPallete);
        brushSize->setObjectName(QStringLiteral("brushSize"));
        brushSize->setReadOnly(true);
        brushSize->setButtonSymbols(QAbstractSpinBox::NoButtons);
        brushSize->setMinimum(1);
        brushSize->setMaximum(241);
        brushSize->setSingleStep(2);
        brushSize->setValue(1);

        horizontalLayout_3->addWidget(brushSize);

        brushSizeSlider = new QSlider(PathingPallete);
        brushSizeSlider->setObjectName(QStringLiteral("brushSizeSlider"));
        brushSizeSlider->setMinimum(1);
        brushSizeSlider->setMaximum(128);
        brushSizeSlider->setSingleStep(1);
        brushSizeSlider->setOrientation(Qt::Horizontal);
        brushSizeSlider->setInvertedAppearance(false);
        brushSizeSlider->setInvertedControls(false);
        brushSizeSlider->setTickPosition(QSlider::TicksBelow);
        brushSizeSlider->setTickInterval(0);

        horizontalLayout_3->addWidget(brushSizeSlider);


        verticalLayout->addLayout(horizontalLayout_3);

        QWidget::setTabOrder(replaceType, addType);
        QWidget::setTabOrder(addType, removeType);
        QWidget::setTabOrder(removeType, walkable);
        QWidget::setTabOrder(walkable, flyable);
        QWidget::setTabOrder(flyable, buildable);
        QWidget::setTabOrder(buildable, brushSize1);
        QWidget::setTabOrder(brushSize1, brushSize3);
        QWidget::setTabOrder(brushSize3, brushSize5);
        QWidget::setTabOrder(brushSize5, brushSize7);
        QWidget::setTabOrder(brushSize7, brushSize9);
        QWidget::setTabOrder(brushSize9, brushSize11);
        QWidget::setTabOrder(brushSize11, brushSize);
        QWidget::setTabOrder(brushSize, brushSizeSlider);

        retranslateUi(PathingPallete);
        QObject::connect(brushSizeSlider, SIGNAL(valueChanged(int)), brushSize, SLOT(setValue(int)));

        QMetaObject::connectSlotsByName(PathingPallete);
    } // setupUi

    void retranslateUi(QWidget *PathingPallete)
    {
        PathingPallete->setWindowTitle(QApplication::translate("PathingPallete", "Pathing Pallete", nullptr));
#ifndef QT_NO_TOOLTIP
        brushOperationLabel->setToolTip(QApplication::translate("PathingPallete", "The type of operation the brush will apply.", nullptr));
#endif // QT_NO_TOOLTIP
        brushOperationLabel->setText(QApplication::translate("PathingPallete", "Brush Operation Type", nullptr));
#ifndef QT_NO_TOOLTIP
        replaceType->setToolTip(QApplication::translate("PathingPallete", "Replaces the existing pathing when the brush is used.", nullptr));
#endif // QT_NO_TOOLTIP
        replaceType->setText(QApplication::translate("PathingPallete", "Replace", nullptr));
#ifndef QT_NO_TOOLTIP
        addType->setToolTip(QApplication::translate("PathingPallete", "Adds to the existing pathing any brush types that are selected.", nullptr));
#endif // QT_NO_TOOLTIP
        addType->setText(QApplication::translate("PathingPallete", "Add", nullptr));
#ifndef QT_NO_TOOLTIP
        removeType->setToolTip(QApplication::translate("PathingPallete", "Removes from the existing pathing all the brush types selected.", nullptr));
#endif // QT_NO_TOOLTIP
        removeType->setText(QApplication::translate("PathingPallete", "Remove", nullptr));
#ifndef QT_NO_TOOLTIP
        brushTypeLabel->setToolTip(QApplication::translate("PathingPallete", "Which pathing type will be applied by the brush.", nullptr));
#endif // QT_NO_TOOLTIP
        brushTypeLabel->setText(QApplication::translate("PathingPallete", "Brush Type", nullptr));
#ifndef QT_NO_TOOLTIP
        walkable->setToolTip(QApplication::translate("PathingPallete", "Whether the brush will apply unwalkable.", nullptr));
#endif // QT_NO_TOOLTIP
        walkable->setText(QString());
#ifndef QT_NO_TOOLTIP
        flyable->setToolTip(QApplication::translate("PathingPallete", "Whether the brush will apply unflyable.", nullptr));
#endif // QT_NO_TOOLTIP
        flyable->setText(QString());
#ifndef QT_NO_TOOLTIP
        buildable->setToolTip(QApplication::translate("PathingPallete", "Whether the brush will aply unbuildable", nullptr));
#endif // QT_NO_TOOLTIP
        buildable->setText(QString());
#ifndef QT_NO_TOOLTIP
        brushSizeLabel->setToolTip(QApplication::translate("PathingPallete", "The brush size that will be applied on the terrain. The final dimensions are: (size-1)*2+1 x (size-1)*2+1.", nullptr));
#endif // QT_NO_TOOLTIP
        brushSizeLabel->setText(QApplication::translate("PathingPallete", "Brush Size", nullptr));
#ifndef QT_NO_TOOLTIP
        brushSize1->setToolTip(QApplication::translate("PathingPallete", "Sets the brush size to 1x1.", nullptr));
#endif // QT_NO_TOOLTIP
        brushSize1->setText(QApplication::translate("PathingPallete", "1", nullptr));
#ifndef QT_NO_TOOLTIP
        brushSize3->setToolTip(QApplication::translate("PathingPallete", "Sets the brush size to 3x3.", nullptr));
#endif // QT_NO_TOOLTIP
        brushSize3->setText(QApplication::translate("PathingPallete", "2", nullptr));
#ifndef QT_NO_TOOLTIP
        brushSize5->setToolTip(QApplication::translate("PathingPallete", "Sets the brush size to 5x5.", nullptr));
#endif // QT_NO_TOOLTIP
        brushSize5->setText(QApplication::translate("PathingPallete", "3", nullptr));
#ifndef QT_NO_TOOLTIP
        brushSize7->setToolTip(QApplication::translate("PathingPallete", "Sets the brush size to 9x9.", nullptr));
#endif // QT_NO_TOOLTIP
        brushSize7->setText(QApplication::translate("PathingPallete", "5", nullptr));
#ifndef QT_NO_TOOLTIP
        brushSize9->setToolTip(QApplication::translate("PathingPallete", "Sets the brush size to 15x15.", nullptr));
#endif // QT_NO_TOOLTIP
        brushSize9->setText(QApplication::translate("PathingPallete", "8", nullptr));
#ifndef QT_NO_TOOLTIP
        brushSize11->setToolTip(QApplication::translate("PathingPallete", "Sets the brush size to 21x21.", nullptr));
#endif // QT_NO_TOOLTIP
        brushSize11->setText(QApplication::translate("PathingPallete", "11", nullptr));
#ifndef QT_NO_TOOLTIP
        brushSize->setToolTip(QApplication::translate("PathingPallete", "The current brush size", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        brushSizeSlider->setToolTip(QApplication::translate("PathingPallete", "Sets the brush size.", nullptr));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class PathingPallete: public Ui_PathingPallete {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PATHINGPALLETE_H
