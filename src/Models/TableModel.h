#pragma once

#include <QAbstractTableModel>

#include "QIconResource.h"
#include "SLK.h"

class TableModel : public QAbstractTableModel {
	//std::unordered_map<std::string, std::string> meta_field_to_key;
	std::shared_ptr<QIconResource> invalid_icon;

public: 
	slk::SLK* meta_slk;
	slk::SLK* slk;
	explicit TableModel(slk::SLK* slk, slk::SLK* meta_slk, QObject* parent = nullptr);
	
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	void copyRow(std::string_view row_header, std::string_view new_row_header);
	void deleteRow(const std::string_view);

	std::string fieldToMetaID(const std::string& id, const std::string& field) const;
	QModelIndex rowIDToIndex(const std::string& id) const;
};