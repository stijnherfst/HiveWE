#pragma once

#include <QAbstractTableModel>

#include "QIconResource.h"
#include "SLK.h"

class TableModel : public QAbstractTableModel {
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	std::unordered_map<std::string, std::string> meta_field_to_key;
	std::shared_ptr<QIconResource> invalid_icon;


public: 
	slk::SLK* meta_slk;
	slk::SLK* slk;
	explicit TableModel(slk::SLK* slk, slk::SLK* meta_slk, QObject* parent = nullptr);
	
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
};