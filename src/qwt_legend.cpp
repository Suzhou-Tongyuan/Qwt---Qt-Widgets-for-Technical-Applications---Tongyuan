/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_legend.h"
#include "qwt_legend_label.h"
#include "qwt_dyngrid_layout.h"
#include "qwt_math.h"
#include "qwt_plot_item.h"
#include "qwt_painter.h"
#include <qapplication.h>
#include <qscrollbar.h>
#include <qscrollarea.h>
#include <qpainter.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QMenu>

class QwtLegendMap
{
public:
    inline bool isEmpty() const { return d_entries.isEmpty(); }

    void insert( const QVariant &, const QList<QWidget *> & );
    void remove( const QVariant & );

    void removeWidget( const QWidget * );

    QList<QWidget *> legendWidgets( const QVariant & ) const;
    QVariant itemInfo( const QWidget * ) const;

private:
    // we don't know anything about itemInfo and therefore don't have
    // any key that can be used for a map or hashtab.
    // But a simple linear list is o.k. here, as we will never have
    // more than a few entries.

    class Entry
    {
    public:
        QVariant itemInfo;
        QList<QWidget *> widgets;
    };

    QList< Entry > d_entries;
};

void QwtLegendMap::insert( const QVariant &itemInfo, 
    const QList<QWidget *> &widgets )
{
    for ( int i = 0; i < d_entries.size(); i++ )
    {
        Entry &entry = d_entries[i];
        if ( entry.itemInfo == itemInfo )
        {
            entry.widgets = widgets;
            return;
        }
    }

    Entry newEntry;
    newEntry.itemInfo = itemInfo;
    newEntry.widgets = widgets;

    d_entries += newEntry;
}

void QwtLegendMap::remove( const QVariant &itemInfo )
{
    for ( int i = 0; i < d_entries.size(); i++ )
    {
        Entry &entry = d_entries[i];
        if ( entry.itemInfo == itemInfo )
        {
            d_entries.removeAt( i );
            return;
        }
    }
}

void QwtLegendMap::removeWidget( const QWidget *widget )
{
    QWidget *w = const_cast<QWidget *>( widget );

    for ( int i = 0; i < d_entries.size(); i++ )
        d_entries[ i ].widgets.removeAll( w );
}

QVariant QwtLegendMap::itemInfo( const QWidget *widget ) const
{
    if ( widget != NULL )
    {
        QWidget *w = const_cast<QWidget *>( widget );

        for ( int i = 0; i < d_entries.size(); i++ )
        {
            const Entry &entry = d_entries[i];
            if ( entry.widgets.indexOf( w ) >= 0 )
                return entry.itemInfo;
        }
    }

    return QVariant();
}

QList<QWidget *> QwtLegendMap::legendWidgets( const QVariant &itemInfo ) const
{
    if ( itemInfo.isValid() )
    {
        for ( int i = 0; i < d_entries.size(); i++ )
        {
            const Entry &entry = d_entries[i];
            if ( entry.itemInfo == itemInfo )
                return entry.widgets;
        }
    }

    return QList<QWidget *>();
}

class QwtLegend::PrivateData
{
public:
    PrivateData():
        itemMode( QwtLegendData::ReadOnly ),
        view( NULL )
    {
    }

    QGraphicsDropShadowEffect *shadow_effect;
    QGraphicsDropShadowEffect *no_frame_effect;

    QMenu * menu;

    QwtLegendData::Mode itemMode;
    QwtLegendMap itemMap;

    class LegendView;
    LegendView *view;
};

class QwtLegend::PrivateData::LegendView: public QScrollArea
{
public:
    LegendView( QwtLegend *parent ):
    QScrollArea( parent ), pa(parent), isDraging(false)
    {
        contentsWidget = new QWidget( this );
        contentsWidget->setObjectName( "QwtLegendViewContents" );
        contentsWidget->setContentsMargins(5, 5, 5, 5);
        setMouseTracking(true);
        setWidget( contentsWidget );
        setWidgetResizable( false );

        viewport()->setObjectName( "QwtLegendViewport" );

        // QScrollArea::setWidget internally sets autoFillBackground to true
        // But we don't want a background.
        contentsWidget->setAutoFillBackground( true );
        viewport()->setAutoFillBackground( true );
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    virtual bool event( QEvent *event )
    {
        if ( event->type() == QEvent::PolishRequest )
        {
            setFocusPolicy( Qt::NoFocus );
        }
        else if ( event->type() == QEvent::Resize )
        {
            // adjust the size to en/disable the scrollbars
            // before QScrollArea adjusts the viewport size

            const QRect cr = contentsRect();

            int w = cr.width();
            int h = contentsWidget->heightForWidth( cr.width() );
            if ( h > w )
            {
                w -= verticalScrollBar()->sizeHint().width();
                h = contentsWidget->heightForWidth( w );
            }

            contentsWidget->resize( w, h );
        }
        else if ( event->type() == QEvent::MouseButtonPress )
        {
            mousePressEvent((QMouseEvent*)event);
        }
        else if ( event->type() == QEvent::MouseButtonRelease )
        {
            mouseReleaseEvent((QMouseEvent*)event);
        }
        else if ( event->type() == QEvent::MouseMove )
        {
            mouseMoveEvent((QMouseEvent*)event);
        }
        return QScrollArea::event( event );
    }

    virtual bool viewportEvent( QEvent *event )
    {
        bool ok = QScrollArea::viewportEvent( event );

        if ( event->type() == QEvent::Resize )
        {
            layoutContents();
        }
        return ok;
    }

    QSize viewportSize( int w, int h ) const
    {
        const int sbHeight = horizontalScrollBar()->sizeHint().height();
        const int sbWidth = verticalScrollBar()->sizeHint().width();

        const int cw = contentsRect().width();
        const int ch = contentsRect().height();

        int vw = cw;
        int vh = ch;

        if ( w > vw )
            vh -= sbHeight;

        if ( h > vh )
        {
            vw -= sbWidth;
            if ( w > vw && vh == ch )
                vh -= sbHeight;
        }
        return QSize( vw, vh );
    }

    void layoutContents()
    {
        const QwtDynGridLayout *tl = qobject_cast<QwtDynGridLayout *>(
            contentsWidget->layout() );
        if ( tl == NULL )
            return;

        const QSize visibleSize = viewport()->contentsRect().size();

        const int minW = int( tl->maxItemWidth() ) + 2 * tl->margin();

        int w = qMax( visibleSize.width(), minW );
        int h = qMax( tl->heightForWidth( w ), visibleSize.height() );

        const int vpWidth = viewportSize( w, h ).width();
        if ( w > vpWidth )
        {
            w = qMax( vpWidth, minW );
            h = qMax( tl->heightForWidth( w ), visibleSize.height() );
        }

        contentsWidget->resize( w, h );
    }

    virtual void mousePressEvent( QMouseEvent * e)
    {
        if (e->buttons() & Qt::LeftButton)
        {
            QPoint point = e->globalPos();
            lastMousePos = point;
            isDraging = true;
        }
    }

    virtual void mouseReleaseEvent( QMouseEvent * e)
    {
        isDraging = false;
    }

    virtual void mouseMoveEvent( QMouseEvent * e)
    {
        if ((e->buttons() & Qt::LeftButton) && isDraging)
        {
            QPoint pos_change = e->globalPos() - lastMousePos;
            pa->legendMoved(pos_change);
            lastMousePos = e->globalPos();
        }
    }

    QwtLegend *pa;
    QWidget *contentsWidget;
    bool isDraging;
    QPoint lastMousePos;//鼠标上一时刻的点击位置，拖拽用
};

/*!
  Constructor
  \param parent Parent widget
*/
QwtLegend::QwtLegend( QWidget *parent ):
    QwtAbstractLegend( parent )
{
    setFrameStyle( NoFrame );
    setObjectName("QwtLegend");
    d_data = new QwtLegend::PrivateData;

    d_data->view = new QwtLegend::PrivateData::LegendView( this );
    d_data->view->setObjectName( "QwtLegendView" );
    setStyleSheet("border: 0px;");
    QwtDynGridLayout *gridLayout = new QwtDynGridLayout(
        d_data->view->contentsWidget, 1);
    gridLayout->setAlignment( Qt::AlignHCenter | Qt::AlignTop );
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(0);
    d_data->view->contentsWidget->installEventFilter( this );

    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setContentsMargins( 1, 1, 1, 1 );
    layout->addWidget( d_data->view );

    d_data->view->contentsWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    initCustomContextMenu();
    connect(d_data->view->contentsWidget, &QWidget::customContextMenuRequested, this, &QwtLegend::onShowContextMenu);
}

//! Destructor
QwtLegend::~QwtLegend()
{
    delete d_data;
}

void QwtLegend::initCustomContextMenu()
{
    d_data->menu = new QMenu;
}

void QwtLegend::onShowContextMenu()
{
    QPoint rightClickedPos = QCursor::pos();
    d_data->menu->exec(rightClickedPos);
}

QMenu * QwtLegend::getMenu()
{
    return d_data->menu;
}

/*!
  \brief Set the maximum number of entries in a row

  F.e when the maximum is set to 1 all items are aligned
  vertically. 0 means unlimited

  \param numColums Maximum number of entries in a row

  \sa maxColumns(), QwtDynGridLayout::setMaxColumns()
 */
void QwtLegend::setMaxColumns( uint numColums )
{
    QwtDynGridLayout *tl = qobject_cast<QwtDynGridLayout *>(
        d_data->view->contentsWidget->layout() );
    if ( tl )
        tl->setMaxColumns( numColums );
}

/*!
  \return Maximum number of entries in a row
  \sa setMaxColumns(), QwtDynGridLayout::maxColumns()
 */
uint QwtLegend::maxColumns() const
{
    uint maxCols = 0;

    const QwtDynGridLayout *tl = qobject_cast<const QwtDynGridLayout *>(
        d_data->view->contentsWidget->layout() );
    if ( tl )
        maxCols = tl->maxColumns();

    return maxCols;
}

/*!
  \brief Set the default mode for legend labels

  Legend labels will be constructed according to the
  attributes in a QwtLegendData object. When it doesn't
  contain a value for the QwtLegendData::ModeRole the
  label will be initialized with the default mode of the legend.

  \param mode Default item mode

  \sa itemMode(), QwtLegendData::value(), QwtPlotItem::legendData()
  \note Changing the mode doesn't have any effect on existing labels.
 */
void QwtLegend::setDefaultItemMode( QwtLegendData::Mode mode )
{
    d_data->itemMode = mode;
}

/*!
  \return Default item mode
  \sa setDefaultItemMode()
*/
QwtLegendData::Mode QwtLegend::defaultItemMode() const
{
    return d_data->itemMode;
}

/*!
  The contents widget is the only child of the viewport of 
  the internal QScrollArea and the parent widget of all legend items.

  \return Container widget of the legend items
*/
QWidget *QwtLegend::contentsWidget()
{
    return d_data->view->contentsWidget;
}

/*!
  \return Horizontal scrollbar
  \sa verticalScrollBar()
*/
QScrollBar *QwtLegend::horizontalScrollBar() const
{
    return d_data->view->horizontalScrollBar();
}

/*!
  \return Vertical scrollbar
  \sa horizontalScrollBar()
*/
QScrollBar *QwtLegend::verticalScrollBar() const
{
    return d_data->view->verticalScrollBar();
}

void QwtLegend::setScrollBarPolicy(Qt::ScrollBarPolicy policy)
{
    d_data->view->setVerticalScrollBarPolicy(policy);
    d_data->view->setHorizontalScrollBarPolicy(policy);
}
/*!
  The contents widget is the only child of the viewport of 
  the internal QScrollArea and the parent widget of all legend items.

  \return Container widget of the legend items

*/
const QWidget *QwtLegend::contentsWidget() const
{
    return d_data->view->contentsWidget;
}

/*!
  \brief Update the entries for an item

  \param itemInfo Info for an item
  \param data List of legend entry attributes for the item
 */
void QwtLegend::updateLegend( const QVariant &itemInfo, 
    const QList<QwtLegendData> &data )
{
    QList<QWidget *> widgetList = legendWidgets( itemInfo );

    if ( widgetList.size() != data.size() )
    {
        QLayout *contentsLayout = d_data->view->contentsWidget->layout();

        while ( widgetList.size() > data.size() )
        {
            QWidget *w = widgetList.takeLast();

            contentsLayout->removeWidget( w );

            // updates might be triggered by signals from the legend widget
            // itself. So we better don't delete it here.

            w->hide();
            w->deleteLater();
        }

        for ( int i = widgetList.size(); i < data.size(); i++ )
        {
            QWidget *widget = createWidget( data[i] );

            if ( contentsLayout )
                contentsLayout->addWidget( widget );

            widgetList += widget;
        }

        if ( widgetList.isEmpty() )
        {
            d_data->itemMap.remove( itemInfo );
        }
        else
        {
            d_data->itemMap.insert( itemInfo, widgetList );
        }

        updateTabOrder();
    }
    
    for ( int i = 0; i < data.size(); i++ )
        updateWidget( widgetList[i], data[i] );
}

/*!
  \brief Create a widget to be inserted into the legend

  The default implementation returns a QwtLegendLabel.

  \param data Attributes of the legend entry
  \return Widget representing data on the legend
  
  \note updateWidget() will called soon after createWidget()
        with the same attributes.
 */
QWidget *QwtLegend::createWidget( const QwtLegendData &data ) const
{
    Q_UNUSED( data );

    QwtLegendLabel *label = new QwtLegendLabel();
    label->setItemMode( defaultItemMode() );

    connect( label, SIGNAL( clicked(Qt::MouseButtons) ), SLOT( itemClicked(Qt::MouseButtons) ) );
    connect( label, SIGNAL( checked( bool ) ), SLOT( itemChecked( bool ) ) );
    connect( label, SIGNAL( dragStarted() ), SLOT( itemDragStarted() ) );

    return label;
}

/*!
  \brief Update the widget 

  \param widget Usually a QwtLegendLabel
  \param data Attributes to be displayed

  \sa createWidget()
  \note When widget is no QwtLegendLabel updateWidget() does nothing.
 */
void QwtLegend::updateWidget( QWidget *widget, const QwtLegendData &data )
{
    widget->setCursor(Qt::ArrowCursor);
    QwtLegendLabel *label = qobject_cast<QwtLegendLabel *>( widget );
    if ( label )
    {
        label->setData( data );
        if ( !data.value( QwtLegendData::ModeRole ).isValid() )
        {
            // use the default mode, when there is no specific
            // hint from the legend data

            label->setItemMode( defaultItemMode() );
        }
    }
}

void QwtLegend::updateTabOrder()
{
    QLayout *contentsLayout = d_data->view->contentsWidget->layout();
    if ( contentsLayout )
    {
        // set tab focus chain

        QWidget *w = NULL;

        for ( int i = 0; i < contentsLayout->count(); i++ )
        {
            QLayoutItem *item = contentsLayout->itemAt( i );
            if ( w && item->widget() )
                QWidget::setTabOrder( w, item->widget() );

            w = item->widget();
        }
    }
}

//! Return a size hint.
QSize QwtLegend::sizeHint() const
{
    QSize hint = d_data->view->contentsWidget->sizeHint();
    hint += QSize( 2 * frameWidth(), 2 * frameWidth() );

    return hint;
}

/*!
  \return The preferred height, for a width.
  \param width Width
*/
int QwtLegend::heightForWidth( int width ) const
{
    width -= 2 * frameWidth();

    int h = d_data->view->contentsWidget->heightForWidth( width );
    if ( h >= 0 )
        h += 2 * frameWidth();

    return h;
}


/*!
  Handle QEvent::ChildRemoved andQEvent::LayoutRequest events 
  for the contentsWidget().

  \param object Object to be filtered
  \param event Event

  \return Forwarded to QwtAbstractLegend::eventFilter()
*/
bool QwtLegend::eventFilter( QObject *object, QEvent *event )
{
    if ( object == d_data->view->contentsWidget )
    {
        switch ( event->type() )
        {
            case QEvent::ChildRemoved:
            {
                const QChildEvent *ce = 
                    static_cast<const QChildEvent *>(event);
                if ( ce->child()->isWidgetType() )
                {
                    QWidget *w = static_cast< QWidget * >( ce->child() );
                    d_data->itemMap.removeWidget( w );
                }
                break;
            }
            case QEvent::LayoutRequest:
            {
                d_data->view->layoutContents();

                if ( parentWidget() && parentWidget()->layout() == NULL )
                {
                    /*
                       We want the parent widget ( usually QwtPlot ) to recalculate
                       its layout, when the contentsWidget has changed. But
                       because of the scroll view we have to forward the LayoutRequest
                       event manually.

                       We don't use updateGeometry() because it doesn't post LayoutRequest
                       events when the legend is hidden. But we want the
                       parent widget notified, so it can show/hide the legend
                       depending on its items.
                     */
                    QApplication::postEvent( parentWidget(),
                        new QEvent( QEvent::LayoutRequest ) );
                }                
                break;
            }
            default:
                break;
        }
    }

    return QwtAbstractLegend::eventFilter( object, event );
}

/*!
  Called internally when the legend has been clicked on.
  Emits a clicked() signal.
*/
void QwtLegend::itemClicked(Qt::MouseButtons mouseButton)
{
    QWidget *w = qobject_cast<QWidget *>( sender() );
    if ( w )
    {
        const QVariant itemInfo = d_data->itemMap.itemInfo( w );
        if ( itemInfo.isValid() )
        {
            const QList<QWidget *> widgetList =
                d_data->itemMap.legendWidgets( itemInfo );

            const int index = widgetList.indexOf( w );
            if ( index >= 0 )
                Q_EMIT clicked(mouseButton, itemInfo, index );
        }
    }
}

/*!
  Called internally when the legend has been checked
  Emits a checked() signal.
*/
void QwtLegend::itemChecked( bool on )
{
    QWidget *w = qobject_cast<QWidget *>( sender() );
    if ( w )
    {
        const QVariant itemInfo = d_data->itemMap.itemInfo( w );
        if ( itemInfo.isValid() )
        {
            const QList<QWidget *> widgetList =
                d_data->itemMap.legendWidgets( itemInfo );

            const int index = widgetList.indexOf( w );
            if ( index >= 0 )
                Q_EMIT checked( itemInfo, on, index );
        }
    }
}

/*!
  Called internally when the legend has been checked
  Emits a checked() signal.
*/
void QwtLegend::itemDragStarted()
{
    QWidget *w = qobject_cast<QWidget *>( sender() );
    if ( w )
    {
        const QVariant itemInfo = d_data->itemMap.itemInfo( w );
        if ( itemInfo.isValid() )
        {
            const QList<QWidget *> widgetList =
                d_data->itemMap.legendWidgets( itemInfo );

            const int index = widgetList.indexOf( w );
            if ( index >= 0 )
                Q_EMIT dragStarted( itemInfo, index );
        }
    }
}

/*!
  Render the legend into a given rectangle.

  \param painter Painter
  \param rect Bounding rectangle
  \param fillBackground When true, fill rect with the widget background 

  \sa renderLegend() is used by QwtPlotRenderer - not by QwtLegend itself
*/
void QwtLegend::renderLegend( QPainter *painter, 
    const QRectF &rect, bool fillBackground ) const
{
    if ( d_data->itemMap.isEmpty() )
        return;

    if ( fillBackground )
    {
        if ( autoFillBackground() ||
            testAttribute( Qt::WA_StyledBackground ) )
        {
            QwtPainter::drawBackgound( painter, rect, this );
        }
    }

    const QwtDynGridLayout *legendLayout = 
        qobject_cast<QwtDynGridLayout *>( contentsWidget()->layout() );
    if ( legendLayout == NULL )
        return;

    int left, right, top, bottom;
    getContentsMargins( &left, &top, &right, &bottom );

    QRect layoutRect; 
    layoutRect.setLeft( qCeil( rect.left() ) + left );
    layoutRect.setTop( qCeil( rect.top() ) + top );
    layoutRect.setRight( qFloor( rect.right() ) - right );
    layoutRect.setBottom( qFloor( rect.bottom() ) - bottom );

    uint numCols = legendLayout->columnsForWidth( layoutRect.width() );
    QList<QRect> itemRects =
        legendLayout->layoutItems( layoutRect, numCols );

    int index = 0;

    for ( int i = 0; i < legendLayout->count(); i++ )
    {
        QLayoutItem *item = legendLayout->itemAt( i );
        QWidget *w = item->widget();
        //LegendItem过多时，超出视图窗口隐藏在滚动条内部LegendItem不进行渲染
        if( itemRects[i].bottom() > layoutRect.bottom())
        {
            break;
        }
        if ( w )
        {
            painter->save();

            painter->setClipRect( itemRects[index], Qt::IntersectClip );
            renderItem( painter, w, itemRects[index], fillBackground );

            index++;
            painter->restore();
        }
    }
}

/*!
  Render a legend entry into a given rectangle.

  \param painter Painter
  \param widget Widget representing a legend entry
  \param rect Bounding rectangle
  \param fillBackground When true, fill rect with the widget background 

  \note When widget is not derived from QwtLegendLabel renderItem
        does nothing beside the background
*/
void QwtLegend::renderItem( QPainter *painter, 
    const QWidget *widget, const QRectF &rect, bool fillBackground ) const
{
    if ( fillBackground )
    {
        if ( widget->autoFillBackground() ||
            widget->testAttribute( Qt::WA_StyledBackground ) )
        {
            QwtPainter::drawBackgound( painter, rect, widget );
        }
    }

    const QwtLegendLabel *label = qobject_cast<const QwtLegendLabel *>( widget );
    if ( label )
    {
        // icon

        const QwtGraphic &icon = label->data().icon();
        const QSizeF sz = icon.defaultSize();

        const QRectF iconRect( rect.x() + label->margin(),
            rect.center().y() - 0.5 * sz.height(), 
            sz.width(), sz.height() );

        icon.render( painter, iconRect, Qt::KeepAspectRatio );

        // title

        QRectF titleRect = rect;
        titleRect.setX( iconRect.right() + 2 * label->spacing() );

        painter->setFont( label->font() );
        painter->setPen( label->palette().color( QPalette::Text ) );
        const_cast< QwtLegendLabel *>( label )->drawText( painter, titleRect );
    }
}

/*!
  \return List of widgets associated to a item
  \param itemInfo Info about an item
  \sa legendWidget(), itemInfo(), QwtPlot::itemToInfo()
 */
QList<QWidget *> QwtLegend::legendWidgets( const QVariant &itemInfo ) const
{
    return d_data->itemMap.legendWidgets( itemInfo );
}

/*!
  \return First widget in the list of widgets associated to an item
  \param itemInfo Info about an item
  \sa itemInfo(), QwtPlot::itemToInfo()
  \note Almost all types of items have only one widget
*/
QWidget *QwtLegend::legendWidget( const QVariant &itemInfo ) const
{
    const QList<QWidget *> list = d_data->itemMap.legendWidgets( itemInfo );
    if ( list.isEmpty() )
        return NULL;

    return list[0];
}

QList<QwtLegendLabel *> QwtLegend::legendLabelLists() const
{
    QLayout * content_layout = d_data->view->contentsWidget->layout();
    QList<QwtLegendLabel *> list;
    for (int i = 0; i < content_layout->count(); ++i)
    { 
        QWidget * label_widget = content_layout->itemAt(i)->widget();
        if (label_widget == nullptr)
        {
            continue;
        }
        QwtLegendLabel * legend_label = qobject_cast<QwtLegendLabel*>(label_widget);
        if (legend_label != nullptr)
        {
            list.append(legend_label);
        }
    }
    return list;
}

/*!
  Find the item that is associated to a widget

  \param widget Widget on the legend
  \return Associated item info
  \sa legendWidget()
 */
QVariant QwtLegend::itemInfo( const QWidget *widget ) const
{
    return d_data->itemMap.itemInfo( widget );
}

//! \return True, when no item is inserted
bool QwtLegend::isEmpty() const
{
    return d_data->itemMap.isEmpty();
}

/*!
    Return the extent, that is needed for the scrollbars

    \param orientation Orientation ( 
    \return The width of the vertical scrollbar for Qt::Horizontal and v.v.
 */
int QwtLegend::scrollExtent( Qt::Orientation orientation ) const
{
    int extent = 0;

    if ( orientation == Qt::Horizontal )
        extent = verticalScrollBar()->sizeHint().width();
    else
        extent = horizontalScrollBar()->sizeHint().height();

    return extent;
}

