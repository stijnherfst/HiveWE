#pragma once

// FlowLayout is a gridlike layout that will adjust the position of the items according to screen width

class FlowLayout : public QLayout {
public:
	explicit FlowLayout(QWidget *parent, int margin = -1, int h_spacing = -1, int v_spacing = -1);
	explicit FlowLayout(int margin = -1, int h_spacing = -1, int v_spacing = -1);
	~FlowLayout();

	void addItem(QLayoutItem *item) override;
	void insert_widget(int index, QWidget* widget);
	void move_widget(int index, QWidget* widget);
	int horizontal_spacing() const;
	int vertical_spacing() const;
	Qt::Orientations expandingDirections() const override;
	bool hasHeightForWidth() const override;
	int heightForWidth(int) const override;
	int count() const override;
	QLayoutItem *itemAt(int index) const override;
	QSize minimumSize() const override;
	void setGeometry(const QRect &rect) override;
	QSize sizeHint() const override;
	QLayoutItem *takeAt(int index) override;
	QList<QLayoutItem*> items() const;
	void clear();

private:
	int do_layout(const QRect &rect, bool test_only, QSize* pMinSize) const;
	int smart_spacing(QStyle::PixelMetric pm) const;

	QSize minSize;
	QList<QLayoutItem *> item_list;
	int h_space;
	int v_space;
};