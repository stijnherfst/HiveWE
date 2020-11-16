#pragma once

#include <QAbstractProxyModel>
#include <QIdentityProxyModel>
#include <QHeaderView>
#include <QStyledItemDelegate>

#include <vector>

#include "TableModel.h"
#include "SLK.h"

class SingleModel : public QAbstractProxyModel {
	Q_OBJECT
		
public:
	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
	
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;

	void setSourceModel(QAbstractItemModel* sourceModel) override;

	explicit SingleModel(TableModel* table, QObject* parent = nullptr);
	void setID(const std::string id);
	std::string getID() const;

	struct Mapping {
		std::string key;
		std::string field;
		int level;
	};

	const std::vector<Mapping>& getMapping() const {
		return id_mapping;
	}

	slk::SLK* meta_slk;
private:
	void buildMapping();

	slk::SLK* slk;

	std::string id = "hpea";
	std::vector<Mapping> id_mapping;
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
	TableDelegate(QWidget* parent = nullptr);

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	void setEditorData(QWidget* editor, const QModelIndex& index) const override;
	void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

	void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};