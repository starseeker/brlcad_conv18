#ifndef CAD_ATTRIBUTES_H
#define CAD_ATTRIBUTES_H

#include <QVariant>
#include <QString>
#include <QList>
#include <QImage>
#include <QAbstractItemModel>
#include <QObject>
#include <QTreeView>
#include <QModelIndex>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QUrl>

#ifndef Q_MOC_RUN
#include "bu/avs.h"
#include "raytrace.h"
#include "ged.h"
#endif

class CADAttributesNode
{
public:
    CADAttributesNode(CADAttributesNode *aParent=0);
    ~CADAttributesNode();

    QString name;
    QString value;
    int attr_type;

    CADAttributesNode *parent;
    QList<CADAttributesNode*> children;
};

class CADAttributesModel : public QAbstractItemModel
{
    Q_OBJECT

    public:  // "standard" custom tree model functions
	explicit CADAttributesModel(QObject *parent = 0, struct db_i *dbip = DBI_NULL, struct directory *dp = RT_DIR_NULL, int show_std = 0, int show_user = 0);
	~CADAttributesModel();

	QModelIndex index(int row, int column, const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &child) const;
	int rowCount(const QModelIndex &child) const;
	int columnCount(const QModelIndex &child) const;

	void setRootNode(CADAttributesNode *root);
	CADAttributesNode* rootNode();

	bool hasChildren(const QModelIndex &parent) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

	CADAttributesNode *m_root;
	QModelIndex NodeIndex(CADAttributesNode *node) const;
	CADAttributesNode *IndexNode(const QModelIndex &index) const;

    public:  // BRL-CAD specific operations
	int update(struct db_i *new_dbip, struct directory *dp);

    public slots:
	void refresh(const QModelIndex &idx);

    protected:

	int NodeRow(CADAttributesNode *node) const;
	bool canFetchMore(const QModelIndex &parent) const;
	void fetchMore(const QModelIndex &parent);

    private:
	CADAttributesNode *add_attribute(const char *name, const char *value, CADAttributesNode *curr_node, int type);
	void add_Children(const char *name, CADAttributesNode *curr_node);
	struct db_i *current_dbip;
	struct directory *current_dp;
	struct bu_attribute_value_set *avs;
	int std_visible;
	int user_visible;
};

class GAttributeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:
	GAttributeDelegate(QWidget *pparent = 0) : QStyledItemDelegate(pparent) {}

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class CADAttributesView : public QTreeView
{
    Q_OBJECT

    public:
	CADAttributesView(QWidget *pparent, int decorate_tree = 0);
	~CADAttributesView() {};
};

#endif /*CAD_ATTRIBUTES_H*/

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */

