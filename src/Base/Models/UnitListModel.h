#pragma once

#include <QIdentityProxyModel>
#include <QSortFilterProxyModel>

class UnitListModel : public QIdentityProxyModel {
	Q_OBJECT

public:
	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;

	using QIdentityProxyModel::QIdentityProxyModel;
};

class UnitListFilter : public QSortFilterProxyModel {
	Q_OBJECT

	bool filterAcceptsRow(int sourceRow,const QModelIndex& sourceParent) const override;
	bool lessThan(const QModelIndex& left,const QModelIndex& right) const override;
	QString filterRace = "human";

public:
	using QSortFilterProxyModel::QSortFilterProxyModel;
	
public slots:
	void setFilterRace(QString race);
};