#ifndef CADACCORDIAN_H
#define CADACCORDIAN_H
#include "cadtreemodel.h"
#include "cadattributes.h"
#include "QToolPalette.h"
#include "QAccordionWidget.h"

class CADViewControls : public QWidget
{
    Q_OBJECT

    public:
	CADViewControls(QWidget *pparent = 0);
	~CADViewControls();

    private:
	QToolPalette *tpalette;
	QWidget *info_view;
};

class CADInstanceEdit : public QWidget
{
    Q_OBJECT

    public:
	CADInstanceEdit(QWidget *pparent = 0);
	~CADInstanceEdit();

    private:
	QToolPalette *tpalette;
	QWidget *info_view;
};

class CADPrimitiveEdit : public QWidget
{
    Q_OBJECT

    public:
	CADPrimitiveEdit(QWidget *pparent = 0);
	~CADPrimitiveEdit();

    private:
	QToolPalette *tpalette;
	QWidget *shape_properties;
};

class EditStateFilter : public QObject
{
    Q_OBJECT

    protected:
	bool eventFilter(QObject *target, QEvent *e);
};

class CADAccordion : public QAccordionWidget
{
    Q_OBJECT
    public:
	CADAccordion(QWidget *pparent);
	~CADAccordion();

	void highlight_selected(QAccordionObject *);

	CADViewControls *view_ctrls;
	QAccordionObject *view_obj;
	CADInstanceEdit *instance_ctrls;
	QAccordionObject *instance_obj;
	CADPrimitiveEdit *primitive_ctrls;
	QAccordionObject *primitive_obj;
	CADAttributesModel *stdpropmodel;
	CADAttributesView *stdpropview;
	QAccordionObject *stdprop_obj;
	CADAttributesModel *userpropmodel;
	CADAttributesView *userpropview;
	QAccordionObject *userprop_obj;
	// Need to create the following
#if 0
	CADAttributesModel *globalpropmodel;
	CADAttributesView *globalpropview;
	QAccordionObject *globalprop_obj;

	Tools Palette (measurement, nirt, raytracing(maybe), etc.)
	Primitive and procedural object creation (would be cool if we could do a wireframe-follows-mouse thing where each click of the mouse inserted a copy of the primitive with the current values at the specified point...)
#endif
	QVector<QAccordionObject *> active_objects;
};

#endif /* CADACCORDIAN_H */

/*
 * Local Variables:
 * mode: C++
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */

