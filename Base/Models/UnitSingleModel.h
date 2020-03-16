#pragma once

#include <QAbstractProxyModel>
#include <QHeaderView>
#include <QStyledItemDelegate>

#include <vector>

class UnitSingleModel : public QAbstractProxyModel {
	Q_OBJECT

	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;


	std::string unit_id = "hpea";
	std::vector<int> id_mapping;

public:
	explicit UnitSingleModel(QObject* parent = nullptr);
	void setUnitID(const std::string_view unitID);
	const std::vector<int>& getMapping() const {
		return id_mapping;
	}
};

// Provides row headers that have alternate colors
class AlterHeader : public QHeaderView {
	Q_OBJECT

public:
	using QHeaderView::QHeaderView;
protected:
	void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const;
};


class TableDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	TableDelegate(QObject* parent = nullptr);

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
		const QModelIndex& index) const override;

	void setEditorData(QWidget* editor, const QModelIndex& index) const override;
	void setModelData(QWidget* editor, QAbstractItemModel* model,
		const QModelIndex& index) const override;

	void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
		const QModelIndex& index) const override;
};