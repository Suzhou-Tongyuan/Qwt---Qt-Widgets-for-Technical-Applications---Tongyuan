/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_scale_widget.h"
#include "qwt_painter.h"
#include "qwt_color_map.h"
#include "qwt_scale_map.h"
#include "qwt_scale_div.h"
#include "qwt_text.h"
#include "qwt_scale_engine.h"
#include <qpainter.h>
#include <qevent.h>
#include <qmath.h>
#include <qstyle.h>
#include <qstyleoption.h>

class QwtScaleWidget::PrivateData
{
public:
    PrivateData():
        scaleDraw( NULL )
    {
        colorBar.colorMap = NULL;
    }

    ~PrivateData()
    {
        delete scaleDraw;
        delete colorBar.colorMap;
    }

    QwtScaleDraw *scaleDraw;

    int borderDist[2];
    int minBorderDist[2];
    int scaleLength;
    int margin;

    int titleOffset;
    int spacing;
    QwtText title;
    QwtText scaleIndex;

    LayoutFlags layoutFlags;

    struct t_colorBar
    {
        bool isEnabled;
        int width;
        QwtInterval interval;
        QwtColorMap *colorMap;
    } colorBar;
};

/*!
  \brief Create a scale with the position QwtScaleWidget::Left
  \param parent Parent widget
*/
QwtScaleWidget::QwtScaleWidget( QWidget *parent ):
    QWidget( parent )
{
    initScale( QwtScaleDraw::LeftScale );
}

/*!
  \brief Constructor
  \param align Alignment.
  \param parent Parent widget
*/
QwtScaleWidget::QwtScaleWidget(
        QwtScaleDraw::Alignment align, QWidget *parent ):
    QWidget( parent )
{
    initScale( align );
}

//! Destructor
QwtScaleWidget::~QwtScaleWidget()
{
    delete d_data;
}

//! Initialize the scale
void QwtScaleWidget::initScale( QwtScaleDraw::Alignment align )
{
    d_data = new PrivateData;

    d_data->layoutFlags = 0;
    //if ( align == QwtScaleDraw::RightScale )
    //    d_data->layoutFlags |= TitleInverted;

    d_data->borderDist[0] = 0;
    d_data->borderDist[1] = 0;
    d_data->minBorderDist[0] = 0;
    d_data->minBorderDist[1] = 0;
    d_data->margin = 4;
    d_data->titleOffset = 0;
    d_data->spacing = 2;

    d_data->scaleDraw = new QwtScaleDraw;
    d_data->scaleDraw->setAlignment( align );
    d_data->scaleDraw->setLength( 10 );

    d_data->scaleDraw->setScaleDiv(
        QwtLinearScaleEngine().divideScale( 0.0, 100.0, 10, 5 ) );

    d_data->colorBar.colorMap = new QwtLinearColorMap();
    d_data->colorBar.isEnabled = false;
    d_data->colorBar.width = 10;

    const int flags = Qt::AlignHCenter
        /*| Qt::TextExpandTabs */| Qt::TextSingleLine;
    d_data->title.setRenderFlags( flags );
    d_data->title.setFont( font() );

    QSizePolicy policy( QSizePolicy::MinimumExpanding,
        QSizePolicy::Fixed );
    if ( d_data->scaleDraw->orientation() == Qt::Vertical )
        policy.transpose();

    setSizePolicy( policy );

    setAttribute( Qt::WA_WState_OwnSizePolicy, false );
}

void QwtScaleWidget::updateScaleIndex()
{
    
    if ( d_data->scaleDraw->getScaleDrawMode() != QwtScaleDraw::DrawScaleIndexMode )
    {
        d_data->scaleIndex = QwtText();
        return;
    }
    const QwtScaleDiv & scaldiv = d_data->scaleDraw->scaleDiv();
    double min_value = scaldiv.lowerBound();
    double max_value = scaldiv.upperBound();

    int index_value = qFloor(log10(qMax(fabs( max_value ), fabs( min_value ))));
    index_value =3 * (index_value / 3);

    if (index_value != 0)
    {
        QwtText index_text( QString( "&times;10<sup>%1<sup>" ).arg(index_value), QwtText::RichText );
        index_text.setRenderFlags( Qt::AlignRight );
        d_data->scaleIndex = index_text;
    }
    else
    {
         d_data->scaleIndex = QwtText();
    }
}

const QwtText &QwtScaleWidget::ScaleIndexText() const
{
    return d_data->scaleIndex;
}

QSize QwtScaleWidget::scaleIndexSize() const
{
    if(d_data->scaleIndex.isEmpty())
    {
        return QSize(0, 0);
    }
    return QSize(d_data->scaleIndex.textSize(font()).width() + 10, d_data->scaleIndex.textSize(font()).height() + 10);
}
/*!
   Toggle an layout flag

   \param flag Layout flag
   \param on true/false

   \sa testLayoutFlag(), LayoutFlag
*/
void QwtScaleWidget::setLayoutFlag( LayoutFlag flag, bool on )
{
    if ( ( ( d_data->layoutFlags & flag ) != 0 ) != on )
    {
        if ( on )
            d_data->layoutFlags |= flag;
        else
            d_data->layoutFlags &= ~flag;
    }
}

/*!
   Test a layout flag

   \param flag Layout flag
   \return true/false
   \sa setLayoutFlag(), LayoutFlag
*/
bool QwtScaleWidget::testLayoutFlag( LayoutFlag flag ) const
{
    return ( d_data->layoutFlags & flag );
}

/*!
  Give title new text contents

  \param title New title
  \sa title(), setTitle(const QwtText &);
*/
void QwtScaleWidget::setTitle( const QString &title )
{
    if ( d_data->title.text() != title )
    {
        d_data->title.setText( title );
        layoutScale();
    }
}

/*!
  Give title new text contents

  \param title New title
  \sa title()
  \warning The title flags are interpreted in
               direction of the label, AlignTop, AlignBottom can't be set
               as the title will always be aligned to the scale.
*/
void QwtScaleWidget::setTitle( const QwtText &title )
{
    QwtText t = title;
    const int flags = title.renderFlags() & ~( Qt::AlignTop | Qt::AlignBottom );
    t.setRenderFlags( flags );

    if ( t != d_data->title )
    {
        d_data->title = t;
        layoutScale();
    }
}

/*!
  Change the alignment

  \param alignment New alignment
  \sa alignment()
*/
void QwtScaleWidget::setAlignment( QwtScaleDraw::Alignment alignment )
{
    if ( d_data->scaleDraw )
        d_data->scaleDraw->setAlignment( alignment );

    if ( !testAttribute( Qt::WA_WState_OwnSizePolicy ) )
    {
        QSizePolicy policy( QSizePolicy::MinimumExpanding,
            QSizePolicy::Fixed );
        if ( d_data->scaleDraw->orientation() == Qt::Vertical )
            policy.transpose();

        setSizePolicy( policy );

        setAttribute( Qt::WA_WState_OwnSizePolicy, false );
    }

    layoutScale();
}


/*!
    \return position
    \sa setPosition()
*/
QwtScaleDraw::Alignment QwtScaleWidget::alignment() const
{
    if ( !scaleDraw() )
        return QwtScaleDraw::LeftScale;

    return scaleDraw()->alignment();
}

/*!
  Specify distances of the scale's endpoints from the
  widget's borders. The actual borders will never be less
  than minimum border distance.
  \param dist1 Left or top Distance
  \param dist2 Right or bottom distance
  \sa borderDist()
*/
void QwtScaleWidget::setBorderDist( int dist1, int dist2 )
{
    if ( dist1 != d_data->borderDist[0] || dist2 != d_data->borderDist[1] )
    {
        d_data->borderDist[0] = dist1;
        d_data->borderDist[1] = dist2;
        layoutScale();
    }
}

/*!
  \brief Specify the margin to the colorBar/base line.
  \param margin Margin
  \sa margin()
*/
void QwtScaleWidget::setMargin( int margin )
{
    margin = qMax( 0, margin );
    if ( margin != d_data->margin )
    {
        d_data->margin = margin;
        layoutScale();
    }
}

/*!
  \brief Specify the distance between color bar, scale and title
  \param spacing Spacing
  \sa spacing()
*/
void QwtScaleWidget::setSpacing( int spacing )
{
    spacing = qMax( 0, spacing );
    if ( spacing != d_data->spacing )
    {
        d_data->spacing = spacing;
        layoutScale();
    }
}

/*!
  \brief Change the alignment for the labels.

  \sa QwtScaleDraw::setLabelAlignment(), setLabelRotation()
*/
void QwtScaleWidget::setLabelAlignment( Qt::Alignment alignment )
{
    d_data->scaleDraw->setLabelAlignment( alignment );
    layoutScale();
}

/*!
  \brief Change the rotation for the labels.
  See QwtScaleDraw::setLabelRotation().

  \param rotation Rotation
  \sa QwtScaleDraw::setLabelRotation(), setLabelFlags()
*/
void QwtScaleWidget::setLabelRotation( double rotation )
{
    d_data->scaleDraw->setLabelRotation( rotation );
    layoutScale();
}

/*!
  Set a scale draw

  scaleDraw has to be created with new and will be deleted in
  ~QwtScaleWidget() or the next call of setScaleDraw().
  scaleDraw will be initialized with the attributes of
  the previous scaleDraw object.

  \param scaleDraw ScaleDraw object
  \sa scaleDraw()
*/
void QwtScaleWidget::setScaleDraw( QwtScaleDraw *scaleDraw )
{
    if ( ( scaleDraw == NULL ) || ( scaleDraw == d_data->scaleDraw ) )
        return;

    const QwtScaleDraw* sd = d_data->scaleDraw;
    if ( sd )
    {
        scaleDraw->setAlignment( sd->alignment() );
        scaleDraw->setScaleDiv( sd->scaleDiv() );

        QwtTransform *transform = NULL;
        if ( sd->scaleMap().transformation() )
            transform = sd->scaleMap().transformation()->copy();

        scaleDraw->setTransformation( transform );
    }

    delete d_data->scaleDraw;
    d_data->scaleDraw = scaleDraw;

    layoutScale();
}

/*!
    \return scaleDraw of this scale
    \sa setScaleDraw(), QwtScaleDraw::setScaleDraw()
*/
const QwtScaleDraw *QwtScaleWidget::scaleDraw() const
{
    return d_data->scaleDraw;
}

/*!
    \return scaleDraw of this scale
    \sa QwtScaleDraw::setScaleDraw()
*/
QwtScaleDraw *QwtScaleWidget::scaleDraw()
{
    return d_data->scaleDraw;
}

/*!
    \return title
    \sa setTitle()
*/
QwtText QwtScaleWidget::title() const
{
    return d_data->title;
}

/*!
    \return start border distance
    \sa setBorderDist()
*/
int QwtScaleWidget::startBorderDist() const
{
    return d_data->borderDist[0];
}

/*!
    \return end border distance
    \sa setBorderDist()
*/
int QwtScaleWidget::endBorderDist() const
{
    return d_data->borderDist[1];
}

/*!
    \return margin
    \sa setMargin()
*/
int QwtScaleWidget::margin() const
{
    return d_data->margin;
}

/*!
    \return distance between scale and title
    \sa setMargin()
*/
int QwtScaleWidget::spacing() const
{
    return d_data->spacing;
}

/*!
  \brief paintEvent
*/
void QwtScaleWidget::paintEvent( QPaintEvent *event )
{
    QPainter painter( this );
    painter.setClipRegion( event->region() );

    QStyleOption opt;
    opt.init(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    draw( &painter );
}

/*!
  \brief draw the scale
*/
void QwtScaleWidget::draw( QPainter *painter ) const
{
    d_data->scaleDraw->draw( painter, palette() );

    if ( d_data->colorBar.isEnabled && d_data->colorBar.width > 0 &&
        d_data->colorBar.interval.isValid() )
    {
        drawColorBar( painter, colorBarRect( contentsRect() ) );
    }

    QRect r = contentsRect();
    if ( d_data->scaleDraw->orientation() == Qt::Horizontal )
    {
        r.setLeft( r.left() + d_data->borderDist[0] );
        r.setWidth( r.width() - d_data->borderDist[1] );
    }
    else
    {
        r.setTop( r.top() + d_data->borderDist[0] );
        r.setHeight( r.height() - d_data->borderDist[1] );
    }

    if ( !d_data->title.isEmpty() )
        drawTitle( painter, d_data->scaleDraw->alignment(), r );
    if (!d_data->scaleIndex.isEmpty())
    {
        drawScaleIndex(painter, r);
    }
}

void QwtScaleWidget::drawScaleIndex( QPainter *painter, const QRectF &rect ) const
{
    if(scaleDraw()->getScaleDrawMode() != QwtScaleDraw::DrawScaleIndexMode)
    {
        return;
    }
    int flags =  ~( Qt::AlignTop | Qt::AlignBottom | Qt::AlignVCenter );
    QRectF r = rect;
    QwtScaleDraw::Alignment align = d_data->scaleDraw->alignment();
    int text_w = scaleIndexSize().width();
    int text_h = scaleIndexSize().height();
    switch ( align )
    {
    case QwtScaleDraw::LeftScale:
        flags |= Qt::AlignTop;
        r.setRect( r.right() -text_w - 10, r.top(),
            text_w, text_h);
        break;

    case QwtScaleDraw::RightScale:
        flags |= Qt::AlignTop;
        r.setRect( r.left(), r.top(),
            text_w, text_h );
        break;

    case QwtScaleDraw::BottomScale:
        flags |= Qt::AlignRight;
        r.setRect( r.right() - text_w, r.top(),
            text_w, text_h);
        break;

    case QwtScaleDraw::TopScale:
    default:
        flags |= Qt::AlignRight;
        r.setRect( r.right(), r.top(),
            text_w, text_h);
        break;
    }

    painter->save();
    painter->setFont( font() );
    painter->setPen( palette().color( QPalette::Text ) );
    
    painter->translate( r.x(), r.y() );

    QwtText scaleIndex = d_data->scaleIndex;
    scaleIndex.setRenderFlags(flags);
    scaleIndex.draw( painter, QRectF( 0.0, 0.0, text_w, text_h ) );
    //painter->drawText(r, flags, scaleIndex.text());
    painter->restore();
}

/*!
  Calculate the the rectangle for the color bar

  \param rect Bounding rectangle for all components of the scale
  \return Rectangle for the color bar
*/
QRectF QwtScaleWidget::colorBarRect( const QRectF& rect ) const
{
    QRectF cr = rect;

    if ( d_data->scaleDraw->orientation() == Qt::Horizontal )
    {
        cr.setLeft( cr.left() + d_data->borderDist[0] );
        cr.setWidth( cr.width() - d_data->borderDist[1] + 1 );
    }
    else
    {
        cr.setTop( cr.top() + d_data->borderDist[0] );
        cr.setHeight( cr.height() - d_data->borderDist[1] + 1 );
    }

    switch ( d_data->scaleDraw->alignment() )
    {
        case QwtScaleDraw::LeftScale:
        {
            cr.setLeft( cr.right() - d_data->margin
                - d_data->colorBar.width );
            cr.setWidth( d_data->colorBar.width );
            break;
        }

        case QwtScaleDraw::RightScale:
        {
            cr.setLeft( cr.left() + d_data->margin );
            cr.setWidth( d_data->colorBar.width );
            break;
        }

        case QwtScaleDraw::BottomScale:
        {
            cr.setTop( cr.top() + d_data->margin );
            cr.setHeight( d_data->colorBar.width );
            break;
        }

        case QwtScaleDraw::TopScale:
        {
            cr.setTop( cr.bottom() - d_data->margin
                - d_data->colorBar.width );
            cr.setHeight( d_data->colorBar.width );
            break;
        }
    }

    return cr;
}

/*!
  Event handler for resize events
  \param event Resize event
*/
void QwtScaleWidget::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED( event );
    layoutScale( false );
}

/*!
  Recalculate the scale's geometry and layout based on
  the current geometry and fonts.

  \param update_geometry Notify the layout system and call update
                         to redraw the scale
*/
void QwtScaleWidget::layoutScale( bool update_geometry )
{
    int bd0, bd1;
    getBorderDistHint( bd0, bd1 );
    if ( d_data->borderDist[0] > bd0 )
        bd0 = d_data->borderDist[0];
    if ( d_data->borderDist[1] > bd1 )
        bd1 = d_data->borderDist[1];

    int colorBarWidth = 0;
    if ( d_data->colorBar.isEnabled && d_data->colorBar.interval.isValid() )
        colorBarWidth = d_data->colorBar.width + d_data->spacing;

    const QRectF r = rect();
    double x, y, length;
    updateScaleIndex();
    if ( d_data->scaleDraw->orientation() == Qt::Vertical )
    {
        y = r.top() +bd0 + scaleIndexSize().height();
        length = r.height() -(bd0 + bd1) - scaleIndexSize().height();

        if (d_data->scaleDraw->alignment() == QwtScaleDraw::LeftScale)
            x = r.right() - 1.0 -d_data->margin - colorBarWidth;
        else
            x = r.left() +d_data->margin + colorBarWidth;
    }
    else
    {
        x = r.left() + bd0;
        length = r.width() - ( bd0 + bd1 ) - scaleIndexSize().width();

        if (d_data->scaleDraw->alignment() == QwtScaleDraw::BottomScale)
            y = r.top() + d_data->margin + colorBarWidth;
        else
            y = r.bottom() - 1.0 -d_data->margin - colorBarWidth;
    }
    int start;
    int end;
    getBorderDistHint(start, end);
    d_data->scaleDraw->setBorderDist(start, end);
    d_data->scaleDraw->move( x, y );
    d_data->scaleDraw->setLength( length );
    //如果布局范围固定，那么ScaleDraw的绘制应该根据
    //当前scaleWidget的大小
    if (testLayoutFlag(ExtentFixed))
    {
        d_data->scaleDraw->setDim( extentWithoutTitle() );
    }
    else
    {
        d_data->scaleDraw->setDim( QRect() );
    }

    d_data->titleOffset = titleHeightForWidth(QWIDGETSIZE_MAX);

    if ( update_geometry )
    {
        updateGeometry();
        update();
    }
}

/*!
  Draw the color bar of the scale widget

  \param painter Painter
  \param rect Bounding rectangle for the color bar

  \sa setColorBarEnabled()
*/
void QwtScaleWidget::drawColorBar( QPainter *painter, const QRectF& rect ) const
{
    if ( !d_data->colorBar.interval.isValid() )
        return;

    const QwtScaleDraw* sd = d_data->scaleDraw;

    QwtPainter::drawColorBar( painter, *d_data->colorBar.colorMap,
        d_data->colorBar.interval.normalized(), sd->scaleMap(),
        sd->orientation(), rect );
}

/*!
  Rotate and paint a title according to its position into a given rectangle.

  \param painter Painter
  \param align Alignment
  \param rect Bounding rectangle
*/

void QwtScaleWidget::drawTitle( QPainter *painter,
    QwtScaleDraw::Alignment align, const QRectF &rect ) const
{
    QRectF r = rect;
    double angle;
    int flags = d_data->title.renderFlags() &
        ~( Qt::AlignTop | Qt::AlignBottom | Qt::AlignVCenter );

    switch ( align )
    {
        case QwtScaleDraw::LeftScale:
            angle = -90.0;
            flags |= Qt::AlignTop;
            r.setRect( r.left(), r.bottom(),
                r.height(), r.width());
            break;

        case QwtScaleDraw::RightScale:
            angle = -90.0;
            flags |= Qt::AlignTop;
            r.setRect( r.right() - d_data->titleOffset, r.bottom(),
                r.height(), r.width() - d_data->titleOffset );
            break;

        case QwtScaleDraw::BottomScale:
            angle = 0.0;
            flags |= Qt::AlignBottom;
            r.setTop( r.top()  );
            break;

        case QwtScaleDraw::TopScale:
        default:
            angle = 0.0;
            flags |= Qt::AlignTop;
            r.setBottom( r.bottom() );
            break;
    }

    if ( d_data->layoutFlags & TitleInverted )
    {
        if ( align == QwtScaleDraw::LeftScale
            || align == QwtScaleDraw::RightScale )
        {
            angle = -angle;
            r.setRect( r.x() + r.height(), r.y() - r.width(),
                r.width(), r.height() );
        }
    }

    painter->save();
    painter->setFont( font() );
    painter->setPen( palette().color( QPalette::Text ) );

    painter->translate( r.x(), r.y() );
    if ( angle != 0.0 )
        painter->rotate( angle );

    QwtText title = d_data->title;
    title.setRenderFlags( flags );
    title.draw( painter, QRectF( 0.0, 0.0, r.width(), r.height() ) );

    painter->restore();
}

/*!
  \brief Notify a change of the scale

  This virtual function can be overloaded by derived
  classes. The default implementation updates the geometry
  and repaints the widget.
*/

void QwtScaleWidget::scaleChange()
{
    layoutScale();
}

/*!
  \return a size hint
*/
QSize QwtScaleWidget::sizeHint() const
{
    return minimumSizeHint();
}

/*!
  \return a minimum size hint
*/
QSize QwtScaleWidget::minimumSizeHint() const
{
    const Qt::Orientation o = d_data->scaleDraw->orientation();

    // Border Distance cannot be less than the scale borderDistHint
    // Note, the borderDistHint is already included in minHeight/minWidth
    int length = 0;
    int mbd1, mbd2;
    getBorderDistHint( mbd1, mbd2 );
    length += qMax( 0, d_data->borderDist[0] - mbd1 );
    length += qMax( 0, d_data->borderDist[1] - mbd2 );
    length += d_data->scaleDraw->minLength( font() );

    int dim = dimForLength( length, font() );
    if ( length < dim )
    {
        // compensate for long titles
        length = dim;
        dim = dimForLength( length, font() );
    }

    QSize size( length + 2, dim );
    if ( o == Qt::Vertical )
        size.transpose();

    int left, right, top, bottom;
    getContentsMargins( &left, &top, &right, &bottom );
    return size + QSize( left + right, top + bottom );
}

/*!
  \brief Find the height of the title for a given width.
  \param width Width
  \return height Height
 */

int QwtScaleWidget::titleHeightForWidth( int width ) const
{
    return qCeil( d_data->title.heightForWidth( width, font() ) );
}

/*!
  \brief Find the minimum dimension for a given length.
         dim is the height, length is the width seen in
         direction of the title.
  \param length width for horizontal, height for vertical scales
  \param scaleFont Font of the scale
  \return height for horizontal, width for vertical scales
*/
int QwtScaleWidget::dimForLength( int length, const QFont &scaleFont ) const
{
    int extent = qCeil(d_data->scaleDraw->extent(scaleFont));

    if(!d_data->scaleIndex.isEmpty())
    {
        if ( d_data->scaleDraw->orientation() == Qt::Vertical )
        {
            extent = qMax(extent, scaleIndexSize().width());
        }
        else
        {
            extent = qMax(extent, scaleIndexSize().height());
        }
    }

    int dim = d_data->margin + extent + 1;

    if (!d_data->title.isEmpty())
        dim += titleHeightForWidth(length) + d_data->spacing;

    if (d_data->colorBar.isEnabled && d_data->colorBar.interval.isValid())
        dim += d_data->colorBar.width + d_data->spacing;

    return dim;
}

/*!
  \brief Calculate the rect that scale label can occupy
         according to contentsrect of the widget itself,
  \return the rect that the label can use
*/
QRect QwtScaleWidget::extentWithoutTitle()
{
    QRect rect = contentsRect();
    if (d_data->scaleDraw->alignment() == QwtScaleDraw::LeftScale
        || d_data->scaleDraw->alignment() == QwtScaleDraw::RightScale)
    {
        rect.setWidth(rect.width() - d_data->margin - 1);
        if ( !d_data->title.isEmpty() )
        {
            rect.setWidth(rect.width() - titleHeightForWidth( QWIDGETSIZE_MAX ) - d_data->spacing);
        }
    }
    else
    {
        // d_data->scaleDraw->alignment() == QwtScaleDraw::TopScale
        // d_data->scaleDraw->alignment() == QwtScaleDraw::BottomScale
        rect.setHeight(rect.height() - d_data->margin - 1);
        if ( !d_data->title.isEmpty() )
        {
            rect.setHeight(rect.height() - titleHeightForWidth( QWIDGETSIZE_MAX ) - d_data->spacing);
        }
    }
    
    rect = rect.normalized();
    return rect;
}

/*!
  \brief Calculate a hint for the border distances.

  This member function calculates the distance
  of the scale's endpoints from the widget borders which
  is required for the mark labels to fit into the widget.
  The maximum of this distance an the minimum border distance
  is returned.

  \param start Return parameter for the border width at 
               the beginning of the scale
  \param end Return parameter for the border width at the 
             end of the scale

  \warning
  <ul> <li>The minimum border distance depends on the font.</ul>
  \sa setMinBorderDist(), getMinBorderDist(), setBorderDist()
*/
void QwtScaleWidget::getBorderDistHint( int &start, int &end ) const
{
    d_data->scaleDraw->getBorderDistHint( font(), start, end );

    if ( start < d_data->minBorderDist[0] )
        start = d_data->minBorderDist[0];

    if ( end < d_data->minBorderDist[1] )
        end = d_data->minBorderDist[1];
}

/*!
  Set a minimum value for the distances of the scale's endpoints from
  the widget borders. This is useful to avoid that the scales
  are "jumping", when the tick labels or their positions change
  often.

  \param start Minimum for the start border
  \param end Minimum for the end border
  \sa getMinBorderDist(), getBorderDistHint()
*/
void QwtScaleWidget::setMinBorderDist( int start, int end )
{
    d_data->minBorderDist[0] = start;
    d_data->minBorderDist[1] = end;
}

/*!
  Get the minimum value for the distances of the scale's endpoints from
  the widget borders.

  \param start Return parameter for the border width at 
               the beginning of the scale
  \param end Return parameter for the border width at the 
             end of the scale

  \sa setMinBorderDist(), getBorderDistHint()
*/
void QwtScaleWidget::getMinBorderDist( int &start, int &end ) const
{
    start = d_data->minBorderDist[0];
    end = d_data->minBorderDist[1];
}

/*!
  \brief Assign a scale division

  The scale division determines where to set the tick marks.

  \param scaleDiv Scale Division
  \sa For more information about scale divisions, see QwtScaleDiv.
*/
void QwtScaleWidget::setScaleDiv( const QwtScaleDiv &scaleDiv )
{
    QwtScaleDraw *sd = d_data->scaleDraw;
    if ( sd->scaleDiv() != scaleDiv )
    {
        sd->setScaleDiv( scaleDiv );
        layoutScale();

        Q_EMIT scaleDivChanged();
    }
}

/*!
  Set the transformation

  \param transformation Transformation
  \sa QwtAbstractScaleDraw::scaleDraw(), QwtScaleMap
 */
void QwtScaleWidget::setTransformation( QwtTransform *transformation )
{
    d_data->scaleDraw->setTransformation( transformation );
    layoutScale();
}

/*!
  En/disable a color bar associated to the scale
  \sa isColorBarEnabled(), setColorBarWidth()
*/
void QwtScaleWidget::setColorBarEnabled( bool on )
{
    if ( on != d_data->colorBar.isEnabled )
    {
        d_data->colorBar.isEnabled = on;
        layoutScale();
    }
}

/*!
  \return true, when the color bar is enabled
  \sa setColorBarEnabled(), setColorBarWidth()
*/
bool QwtScaleWidget::isColorBarEnabled() const
{
    return d_data->colorBar.isEnabled;
}

/*!
  Set the width of the color bar

  \param width Width
  \sa colorBarWidth(), setColorBarEnabled()
*/
void QwtScaleWidget::setColorBarWidth( int width )
{
    if ( width != d_data->colorBar.width )
    {
        d_data->colorBar.width = width;
        if ( isColorBarEnabled() )
            layoutScale();
    }
}

/*!
  \return Width of the color bar
  \sa setColorBarEnabled(), setColorBarEnabled()
*/
int QwtScaleWidget::colorBarWidth() const
{
    return d_data->colorBar.width;
}

/*!
  \return Value interval for the color bar
  \sa setColorMap(), colorMap()
*/
QwtInterval QwtScaleWidget::colorBarInterval() const
{
    return d_data->colorBar.interval;
}

/*!
  Set the color map and value interval, that are used for displaying
  the color bar.

  \param interval Value interval
  \param colorMap Color map

  \sa colorMap(), colorBarInterval()
*/
void QwtScaleWidget::setColorMap(
    const QwtInterval &interval, QwtColorMap *colorMap )
{
    d_data->colorBar.interval = interval;

    if ( colorMap != d_data->colorBar.colorMap )
    {
        delete d_data->colorBar.colorMap;
        d_data->colorBar.colorMap = colorMap;
    }

    if ( isColorBarEnabled() )
        layoutScale();
}

/*!
  \return Color map
  \sa setColorMap(), colorBarInterval()
*/
const QwtColorMap *QwtScaleWidget::colorMap() const
{
    return d_data->colorBar.colorMap;
}