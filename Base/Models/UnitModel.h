#pragma once

#include <QAbstractTableModel>
#include <QAbstractProxyModel>
#include <QPainter>
#include <QHeaderView>

#include <vector>

#include "HiveWE.h"
#include "QIconResource.h"

class UnitModel : public QAbstractTableModel {
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	std::unordered_map<std::string, int> meta_field_to_index;

public: 
	explicit UnitModel(QObject* parent = nullptr);
	//bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
};

class UnitSingleModel : public QAbstractProxyModel {
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
};

class AlterHeader : public QHeaderView {
	Q_OBJECT

public:
	//explicit AlterHeader(Qt::Orientation orientation, QWidget* parent = nullptr);
	using QHeaderView::QHeaderView;
protected:
	void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const;
};