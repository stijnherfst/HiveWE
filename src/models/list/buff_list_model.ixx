module;

#include <QSortFilterProxyModel>

export module BuffListModel;

import BaseListModel;
import Globals;

export class BuffListModel: public BaseListModel {
	Q_OBJECT

  public:
	explicit BuffListModel(QObject* parent = nullptr) : BaseListModel(buff_slk, parent) {}

	[[nodiscard]]
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override {
		if (!proxyIndex.isValid()) {
			return {};
		}

		return sourceModel()->index(proxyIndex.row(), buff_slk.column_headers.at("editorname"));
	}

	[[nodiscard]]
	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		switch (role) {
			case Qt::DisplayRole: {
				const QString editorname = sourceModel()->index(index.row(), buff_slk.column_headers.at("editorname")).data(role).toString();
				const QString editorsuffix = QString::fromStdString(buff_slk.data("editorsuffix", index.row()));
				if (editorname.isEmpty()) {
					return sourceModel()->index(index.row(), buff_slk.column_headers.at("bufftip")).data(role).toString() + " " + editorsuffix;;
				} else {
					return editorname + " " + editorsuffix;
				}
			}
			case Qt::UserRole:
				return QString::fromStdString("buffs/" + buff_slk.data("race", index.row()) + "/" + buff_slk.index_to_row.at(index.row()));
			case Qt::DecorationRole:
				return sourceModel()->index(index.row(), buff_slk.column_headers.at("buffart")).data(role);
			default:
				return BaseListModel::data(index, role);
		}
	}
};

export class BuffListFilter: public QSortFilterProxyModel {
	Q_OBJECT

	[[nodiscard]]
	bool filterAcceptsRow(const int sourceRow, const QModelIndex& sourceParent) const override {
		if (QString::fromStdString(buff_slk.index_to_row.at(sourceRow)).contains(filterRegularExpression())) {
			return true;
		}

		if (!filterRegularExpression().pattern().isEmpty()) {
			const QModelIndex source_index = sourceModel()->index(sourceRow, 0);
			return source_index.data().toString().contains(filterRegularExpression());
		}

		return true;
	}

  public:
	using QSortFilterProxyModel::QSortFilterProxyModel;
};

#include "buff_list_model.moc"
