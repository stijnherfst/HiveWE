#pragma once

#include <QAbstractTableModel>
#include <QPainter>

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
};