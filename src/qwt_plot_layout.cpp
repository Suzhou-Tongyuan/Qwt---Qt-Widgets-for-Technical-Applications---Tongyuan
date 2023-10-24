/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot_layout.h"
#include "qwt_text.h"
#include "qwt_text_label.h"
#include "qwt_scale_widget.h"
#include "qwt_abstract_legend.h"
#include <qmath.h>

class QwtPlotLayout::LayoutData
{
public:
    LayoutData();

    void init( const QwtPlot *, const QRectF &rect );
    struct t_legendData
    {
        int frameWidth;
        int hScrollExtent;
        int vScrollExtent;
        QSize hint;
        double xOffset;//相对于顶部的偏移比；只有拖拽legend才会改变这个值
        double yOffset;//
        double xTotalSize;
        double yTotalSize;
        QSize maxHint;
    } legend;

    struct t_titleData
    {
        QwtText text;
        int frameWidth;
    } title;

    struct t_footerData
    {
        QwtText text;
        int frameWidth;
    } footer;

    struct t_scaleData
    {
        bool isEnabled;
        const QwtScaleWidget *scaleWidget;
        QFont scaleFont;
        int start;
        int end;
        int baseLineOffset;
        double tickOffset;
        int hintDim; //推荐dim，这个值由所有子窗口共同决定，供参考
        int fixedDim; //外部主动设置的dim,此长度包括标题
        int dim; //根据本身坐标轴文本布局自动计算得到的dim
        int dimBefore; //原先的dim，即没有updatelayout前的dim

        int hintMargin;
        int fixedMargin;
        int margin;//坐标轴最外侧到窗口最外侧的距离，
        int marginBefore;//之前坐标轴最外侧到窗口最外侧的距离；

        int dimWithoutTitle; //绘制采用的dim，如果fixedDim非0，则采用fixedDim，否则采用dimForLenth()计算
    } scale[QwtPlot::axisCnt];

    struct t_canvasData
    {
        int contentsMargins[QwtPlot::axisCnt];

    } canvas;
};

QwtPlotLayout::LayoutData::LayoutData()
{
    for (int i = 0; i < QwtPlot::axisCnt; ++i)
    {
        scale[i].fixedDim = 0;
        scale[i].hintDim = 0;
        scale[i].dim = 0;
        scale[i].dimBefore = 0;
        scale[i].fixedMargin = 0;
        scale[i].hintMargin = 0;
        scale[i].margin = 0;
        scale[i].marginBefore = 0;
    }
    legend.xOffset = 0;
    legend.yOffset = 0;
    legend.xTotalSize = 0;
    legend.yTotalSize = 0;
}

/*!
  Extract all layout relevant data from the plot components
*/
void QwtPlotLayout::LayoutData::init( const QwtPlot *plot, const QRectF &rect )
{
    // legend

    if ( plot->legend() )
    {
        legend.frameWidth = plot->legend()->frameWidth();
        legend.hScrollExtent =
            plot->legend()->scrollExtent( Qt::Horizontal );
        legend.vScrollExtent =
            plot->legend()->scrollExtent( Qt::Vertical );

        const QSize hint = plot->legend()->sizeHint();
        legend.maxHint = hint;
        int w = qMin( hint.width(), qFloor( rect.width() ) );
        int h = plot->legend()->heightForWidth( w );
        if ( h <= 0 )
            h = hint.height();

        if ( h > rect.height() )
            w += legend.hScrollExtent;

        legend.hint = QSize( w, h );
    }

    // title

    title.frameWidth = 0;
    title.text = QwtText();

    if ( plot->titleLabel() )
    {
        const QwtTextLabel *label = plot->titleLabel();
        title.text = label->text();
        if ( !( title.text.testPaintAttribute( QwtText::PaintUsingTextFont ) ) )
            title.text.setFont( label->font() );

        title.frameWidth = plot->titleLabel()->frameWidth();
    }

    // footer

    footer.frameWidth = 0;
    footer.text = QwtText();

    if ( plot->footerLabel() )
    {
        const QwtTextLabel *label = plot->footerLabel();
        footer.text = label->text();
        if ( !( footer.text.testPaintAttribute( QwtText::PaintUsingTextFont ) ) )
            footer.text.setFont( label->font() );

        footer.frameWidth = plot->footerLabel()->frameWidth();
    }

    // scales

    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
    {
        if ( plot->axisEnabled( axis ) )
        {
            const QwtScaleWidget *scaleWidget = plot->axisWidget( axis );

            scale[axis].isEnabled = true;

            scale[axis].scaleWidget = scaleWidget;

            scale[axis].scaleFont = scaleWidget->font();

            scale[axis].start = scaleWidget->startBorderDist();
            scale[axis].end = scaleWidget->endBorderDist();

            scale[axis].baseLineOffset = scaleWidget->margin();
            scale[axis].tickOffset = scaleWidget->margin();
            if ( scaleWidget->scaleDraw()->hasComponent(
                QwtAbstractScaleDraw::Ticks ) )
            {
                scale[axis].tickOffset +=
                    scaleWidget->scaleDraw()->maxTickLength();
            }
            //若fixedDim非0，那么用fixedDim初始化DimWithoutTitle，此处应减去标题
            if ( scale[axis].fixedDim != 0 )
            {
                scale[axis].dimWithoutTitle = scale[axis].fixedDim;
                if (!scaleWidget->title().isEmpty())
                {
                    scale[axis].dimWithoutTitle -=
                        scaleWidget->titleHeightForWidth(QWIDGETSIZE_MAX);
                }
            }
            else
            {
                scale[axis].dimWithoutTitle = scaleWidget->dimForLength(
                    QWIDGETSIZE_MAX, scale[axis].scaleFont );

                if (!scaleWidget->title().isEmpty())
                {
                    scale[axis].dimWithoutTitle -=
                        scaleWidget->titleHeightForWidth(QWIDGETSIZE_MAX);
                }
            }
        }
        else
        {
            scale[axis].isEnabled = false;
            scale[axis].start = 0;
            scale[axis].end = 0;
            scale[axis].baseLineOffset = 0;
            scale[axis].tickOffset = 0.0;
            scale[axis].dimWithoutTitle = scale[axis].fixedDim;
        }
    }

    // canvas

    plot->canvas()->getContentsMargins( 
        &canvas.contentsMargins[ QwtPlot::yLeft ], 
        &canvas.contentsMargins[ QwtPlot::xTop ],
        &canvas.contentsMargins[ QwtPlot::yRight ],
        &canvas.contentsMargins[ QwtPlot::xBottom ] );
}

class QwtPlotLayout::PrivateData
{
public:
    PrivateData()
        : spacing(5)
        , allScaleVisible(true)
        , legendPos(QwtPlot::TopLegend)
        , legendFloatPos(QwtPlot::TopCenter)
        , minCanvasWidth(40)
        , minCanvasHeight(40)
    {
    }

    QRectF titleRect;
    QRectF footerRect;
    QRectF legendRect;
    QRectF scaleRect[QwtPlot::axisCnt];
    QRectF scaleIndexRect[QwtPlot::axisCnt];
    QRectF canvasRect;
    LayoutData layoutData;

    QwtPlot::LegendLayout legendPos;
    QwtPlot::FloatPosition legendFloatPos;
    double legendRatio;
    unsigned int spacing;
    unsigned int canvasMargin[QwtPlot::axisCnt];
    bool alignCanvasToScales[QwtPlot::axisCnt];
    bool allScaleVisible;
    bool dimChanged;
    int minCanvasWidth;
    int minCanvasHeight;
};

/*!
  \brief Constructor
 */
QwtPlotLayout::QwtPlotLayout()
{
    d_data = new PrivateData;

    setLegendPosition( QwtPlot::BottomLegend );
    setCanvasMargin( 4 );
    setAlignCanvasToScales( false );

    invalidate();
}

//! Destructor
QwtPlotLayout::~QwtPlotLayout()
{
    delete d_data;
}

bool QwtPlotLayout::isDimChanged()
{
    return d_data->dimChanged;
}

/*!
  Change a margin of the canvas. The margin is the space
  above/below the scale ticks. A negative margin will
  be set to -1, excluding the borders of the scales.

  \param margin New margin
  \param axis One of QwtPlot::Axis. Specifies where the position of the margin.
              -1 means margin at all borders.
  \sa canvasMargin()

  \warning The margin will have no effect when alignCanvasToScale() is true
*/
void QwtPlotLayout::setCanvasMargin( int margin, int axis )
{
    if ( margin < -1 )
        margin = -1;

    if ( axis == -1 )
    {
        for ( axis = 0; axis < QwtPlot::axisCnt; axis++ )
            d_data->canvasMargin[axis] = margin;
    }
    else if ( axis >= 0 && axis < QwtPlot::axisCnt )
        d_data->canvasMargin[axis] = margin;
}

/*!
    \param axisId Axis index
    \return Margin around the scale tick borders
    \sa setCanvasMargin()
*/
int QwtPlotLayout::canvasMargin( int axisId ) const
{
    if ( axisId < 0 || axisId >= QwtPlot::axisCnt )
        return 0;

    return d_data->canvasMargin[axisId];
}

/*!
  \brief Set the align-canvas-to-axis-scales flag for all axes

  \param on True/False
  \sa setAlignCanvasToScale(), alignCanvasToScale()
*/
void QwtPlotLayout::setAlignCanvasToScales( bool on )
{
    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
        d_data->alignCanvasToScales[axis] = on;
}

/*!
  Change the align-canvas-to-axis-scales setting. The canvas may:

  - extend beyond the axis scale ends to maximize its size,
  - align with the axis scale ends to control its size.

  The axisId parameter is somehow confusing as it identifies a border
  of the plot and not the axes, that are aligned. F.e when QwtPlot::yLeft
  is set, the left end of the the x-axes ( QwtPlot::xTop, QwtPlot::xBottom )
  is aligned.

  \param axisId Axis index
  \param on New align-canvas-to-axis-scales setting

  \sa setCanvasMargin(), alignCanvasToScale(), setAlignCanvasToScales()
  \warning In case of on == true canvasMargin() will have no effect
*/
void QwtPlotLayout::setAlignCanvasToScale( int axisId, bool on )
{
    if ( axisId >= 0 && axisId < QwtPlot::axisCnt )
        d_data->alignCanvasToScales[axisId] = on;
}

/*!
  Return the align-canvas-to-axis-scales setting. The canvas may:
  - extend beyond the axis scale ends to maximize its size
  - align with the axis scale ends to control its size.

  \param axisId Axis index
  \return align-canvas-to-axis-scales setting
  \sa setAlignCanvasToScale(), setAlignCanvasToScale(), setCanvasMargin()
*/
bool QwtPlotLayout::alignCanvasToScale( int axisId ) const
{
    if ( axisId < 0 || axisId >= QwtPlot::axisCnt )
        return false;

    return d_data->alignCanvasToScales[ axisId ];
}

/*!
设置画布的最小尺寸，小于该尺寸则隐藏画布外的所有元素。

\param w is width, h is height
*/
void QwtPlotLayout::setCanvasMinSize(int w, int h)
{
    d_data->minCanvasHeight = h;
    d_data->minCanvasWidth = w;
}

/*!
  Change the spacing of the plot. The spacing is the distance
  between the plot components.

  \param spacing New spacing
  \sa setCanvasMargin(), spacing()
*/
void QwtPlotLayout::setSpacing( int spacing )
{
    d_data->spacing = qMax( 0, spacing );
}

/*!
  \return Spacing
  \sa margin(), setSpacing()
*/
int QwtPlotLayout::spacing() const
{
    return d_data->spacing;
}

/*!
  \legend标签相对于最左边的偏移比例。仅对浮动模式下的legend有效
 */
double QwtPlotLayout::legendXOffsetRatio()
{
    return d_data->layoutData.legend.xOffset;
}

double QwtPlotLayout::legendYOffsetRataio()
{
    return d_data->layoutData.legend.yOffset;
}

void QwtPlotLayout::adjustLegendOffset(double x_off, double y_off)
{
    d_data->layoutData.legend.xOffset += x_off;
    d_data->layoutData.legend.yOffset += y_off;
    d_data->legendFloatPos = QwtPlot::FreePosition;
}
/*!
  \brief Specify the position of the legend
  \param pos The legend's position.
  \param ratio Ratio between legend and the bounding rectangle
               of title, footer, canvas and axes. The legend will be shrunk
               if it would need more space than the given ratio.
               The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0
               it will be reset to the default ratio.
               The default vertical/horizontal ratio is 0.33/0.5.

  \sa QwtPlot::setLegendPosition()
*/

void QwtPlotLayout::setLegendFloatPosition(QwtPlot::FloatPosition f_pos)
{
    d_data->legendFloatPos = f_pos;
    if (f_pos !=QwtPlot::FreePosition)
    {
        d_data->legendPos = QwtPlot::FloatLegend;
    }
}

void QwtPlotLayout::setLegendPosition( QwtPlot::LegendLayout pos, double ratio )
{
    if ( ratio > 1.0 )
        ratio = 1.0;

    switch ( pos )
    {
        case QwtPlot::TopLegend:
        case QwtPlot::BottomLegend:
            if ( ratio <= 0.0 )
                ratio = 0.33;
            d_data->legendRatio = ratio;
            d_data->legendPos = pos;
            break;
        case QwtPlot::LeftLegend:
        case QwtPlot::RightLegend:
            if ( ratio <= 0.0 )
                ratio = 0.5;
            d_data->legendRatio = ratio;
            d_data->legendPos = pos;
            break;
        case QwtPlot::FloatLegend:
            d_data->legendPos = pos;
            d_data->legendRatio = ratio;
        case QwtPlot::HideLegend:
            d_data->legendPos = pos;
            break;
        default:
            break;
    }
}

/*!
  \brief Specify the position of the legend
  \param pos The legend's position. Valid values are
      \c QwtPlot::LeftLegend, \c QwtPlot::RightLegend,
      \c QwtPlot::TopLegend, \c QwtPlot::BottomLegend.

  \sa QwtPlot::setLegendPosition()
*/
void QwtPlotLayout::setLegendPosition( QwtPlot::LegendLayout pos )
{
    setLegendPosition( pos, 0.0 );
}

/*!
  \return Position of the legend
  \sa setLegendPosition(), QwtPlot::setLegendPosition(),
      QwtPlot::legendPosition()
*/
QwtPlot::LegendLayout QwtPlotLayout::legendPosition() const
{
    return d_data->legendPos;
}

/*!
  \return Position of the legend
*/
QwtPlot::FloatPosition QwtPlotLayout::legendFloatPosition() const
{
    return d_data->legendFloatPos;
}
/*!
  Specify the relative size of the legend in the plot
  \param ratio Ratio between legend and the bounding rectangle
               of title, footer, canvas and axes. The legend will be shrunk
               if it would need more space than the given ratio.
               The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0
               it will be reset to the default ratio.
               The default vertical/horizontal ratio is 0.33/0.5.
*/
void QwtPlotLayout::setLegendRatio( double ratio )
{
    setLegendPosition( legendPosition(), ratio );
}

/*!
  \return The relative size of the legend in the plot.
  \sa setLegendPosition()
*/
double QwtPlotLayout::legendRatio() const
{
    return d_data->legendRatio;
}

/*!
  \brief Set the geometry for the title

  This method is intended to be used from derived layouts
  overloading activate()

  \sa titleRect(), activate()
 */
void QwtPlotLayout::setTitleRect( const QRectF &rect )
{
    d_data->titleRect = rect;
}

/*!
  \return Geometry for the title
  \sa activate(), invalidate()
*/
QRectF QwtPlotLayout::titleRect() const
{
    return d_data->titleRect;
}

/*!
  \brief Set the geometry for the footer

  This method is intended to be used from derived layouts
  overloading activate()

  \sa footerRect(), activate()
 */
void QwtPlotLayout::setFooterRect( const QRectF &rect )
{
    d_data->footerRect = rect;
}

/*!
  \return Geometry for the footer
  \sa activate(), invalidate()
*/
QRectF QwtPlotLayout::footerRect() const
{
    return d_data->footerRect;
}

/*!
  \brief Set the geometry for the legend

  This method is intended to be used from derived layouts
  overloading activate()

  \param rect Rectangle for the legend

  \sa legendRect(), activate()
 */
void QwtPlotLayout::setLegendRect( const QRectF &rect )
{
    d_data->legendRect = rect;
}

/*!
  \return Geometry for the legend
  \sa activate(), invalidate()
*/
QRectF QwtPlotLayout::legendRect() const
{
    return d_data->legendRect;
}

/*!
  \brief Set the geometry for an axis

  This method is intended to be used from derived layouts
  overloading activate()

  \param axis Axis index
  \param rect Rectangle for the scale

  \sa scaleRect(), activate()
 */
void QwtPlotLayout::setScaleRect( int axis, const QRectF &rect )
{
    if ( axis >= 0 && axis < QwtPlot::axisCnt )
        d_data->scaleRect[axis] = rect;
}

/*!
  \param axis Axis index
  \return Geometry for the scale
  \sa activate(), invalidate()
*/
QRectF QwtPlotLayout::scaleRect( int axis ) const
{
    if ( axis < 0 || axis >= QwtPlot::axisCnt )
    {
        static QRectF dummyRect;
        return dummyRect;
    }
    return d_data->scaleRect[axis];
}

/*!
  \brief Set the geometry for the canvas

  This method is intended to be used from derived layouts
  overloading activate()

  \sa canvasRect(), activate()
 */
void QwtPlotLayout::setCanvasRect( const QRectF &rect )
{
    d_data->canvasRect = rect;
}

/*!
  \return Geometry for the canvas
  \sa activate(), invalidate()
*/
QRectF QwtPlotLayout::canvasRect() const
{
    return d_data->canvasRect;
}

/*!
  Invalidate the geometry of all components.
  \sa activate()
*/
void QwtPlotLayout::invalidate()
{
    d_data->titleRect = d_data->footerRect
        = d_data->legendRect = d_data->canvasRect = QRect();

    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
    {   
        d_data->scaleRect[axis] = QRect();
        d_data->scaleIndexRect[axis] = QRect();
    }
    d_data->dimChanged = false;
}

bool QwtPlotLayout::scaleVisible() const
{
    return d_data->allScaleVisible;
}

void QwtPlotLayout::setAllScaleVisible( bool visible )
{
    d_data->allScaleVisible = visible;
}

/*!
  \return Minimum size hint
  \param plot Plot widget

  \sa QwtPlot::minimumSizeHint()
*/

QSize QwtPlotLayout::minimumSizeHint( const QwtPlot *plot ) const
{
    //@zhenghy:为了PlotGroup布局需要，暂时使sizehint失效
    return QSize(-1, -1);
    class ScaleData
    {
    public:
        ScaleData()
        {
            w = h = minLeft = minRight = tickOffset = 0;
        }

        int w;
        int h;
        int minLeft;
        int minRight;
        int tickOffset;
    } scaleData[QwtPlot::axisCnt];

    int canvasBorder[QwtPlot::axisCnt];

    int fw;
    plot->canvas()->getContentsMargins( &fw, NULL, NULL, NULL );

    int axis;
    for ( axis = 0; axis < QwtPlot::axisCnt; axis++ )
    {
        if ( plot->axisEnabled( axis ) )
        {
            const QwtScaleWidget *scl = plot->axisWidget( axis );
            ScaleData &sd = scaleData[axis];

            const QSize hint = scl->minimumSizeHint();
            sd.w = hint.width();
            sd.h = hint.height();
            scl->getBorderDistHint( sd.minLeft, sd.minRight );
            sd.tickOffset = scl->margin();
            if ( scl->scaleDraw()->hasComponent( QwtAbstractScaleDraw::Ticks ) )
                sd.tickOffset += qCeil( scl->scaleDraw()->maxTickLength() );
        }

        canvasBorder[axis] = fw + d_data->canvasMargin[axis] + 1;
    }


    for ( axis = 0; axis < QwtPlot::axisCnt; axis++ )
    {
        ScaleData &sd = scaleData[axis];
        if ( sd.w && ( axis == QwtPlot::xBottom || axis == QwtPlot::xTop ) )
        {
            if ( ( sd.minLeft > canvasBorder[QwtPlot::yLeft] )
                && scaleData[QwtPlot::yLeft].w )
            {
                int shiftLeft = sd.minLeft - canvasBorder[QwtPlot::yLeft];
                if ( shiftLeft > scaleData[QwtPlot::yLeft].w )
                    shiftLeft = scaleData[QwtPlot::yLeft].w;

                sd.w -= shiftLeft;
            }
            if ( ( sd.minRight > canvasBorder[QwtPlot::yRight] )
                && scaleData[QwtPlot::yRight].w )
            {
                int shiftRight = sd.minRight - canvasBorder[QwtPlot::yRight];
                if ( shiftRight > scaleData[QwtPlot::yRight].w )
                    shiftRight = scaleData[QwtPlot::yRight].w;

                sd.w -= shiftRight;
            }
        }

        if ( sd.h && ( axis == QwtPlot::yLeft || axis == QwtPlot::yRight ) )
        {
            if ( ( sd.minLeft > canvasBorder[QwtPlot::xBottom] ) &&
                scaleData[QwtPlot::xBottom].h )
            {
                int shiftBottom = sd.minLeft - canvasBorder[QwtPlot::xBottom];
                if ( shiftBottom > scaleData[QwtPlot::xBottom].tickOffset )
                    shiftBottom = scaleData[QwtPlot::xBottom].tickOffset;

                sd.h -= shiftBottom;
            }
            if ( ( sd.minLeft > canvasBorder[QwtPlot::xTop] ) &&
                scaleData[QwtPlot::xTop].h )
            {
                int shiftTop = sd.minRight - canvasBorder[QwtPlot::xTop];
                if ( shiftTop > scaleData[QwtPlot::xTop].tickOffset )
                    shiftTop = scaleData[QwtPlot::xTop].tickOffset;

                sd.h -= shiftTop;
            }
        }
    }

    const QWidget *canvas = plot->canvas();

    int left, top, right, bottom;
    canvas->getContentsMargins( &left, &top, &right, &bottom );

    const QSize minCanvasSize = canvas->minimumSize();

    int w = scaleData[QwtPlot::yLeft].w + scaleData[QwtPlot::yRight].w;
    int cw = qMax( scaleData[QwtPlot::xBottom].w, scaleData[QwtPlot::xTop].w )
        + left + 1 + right + 1;
    w += qMax( cw, minCanvasSize.width() );

    int h = scaleData[QwtPlot::xBottom].h + scaleData[QwtPlot::xTop].h;
    int ch = qMax( scaleData[QwtPlot::yLeft].h, scaleData[QwtPlot::yRight].h )
        + top + 1 + bottom + 1;
    h += qMax( ch, minCanvasSize.height() );

    const QwtTextLabel *labels[2];
    labels[0] = plot->titleLabel();
    labels[1] = plot->footerLabel();

    for ( int i = 0; i < 2; i++ )
    {
        const QwtTextLabel *label   = labels[i];
        if ( label && !label->text().isEmpty() )
        {
            // If only QwtPlot::yLeft or QwtPlot::yRight is showing,
            // we center on the plot canvas.
            const bool centerOnCanvas = !( plot->axisEnabled( QwtPlot::yLeft )
                && plot->axisEnabled( QwtPlot::yRight ) );

            int labelW = w;
            if ( centerOnCanvas )
            {
                labelW -= scaleData[QwtPlot::yLeft].w
                    + scaleData[QwtPlot::yRight].w;
            }

            int labelH = label->heightForWidth( labelW );
            if ( labelH > labelW ) // Compensate for a long title
            {
                w = labelW = labelH;
                if ( centerOnCanvas )
                {
                    w += scaleData[QwtPlot::yLeft].w
                        + scaleData[QwtPlot::yRight].w;
                }

                labelH = label->heightForWidth( labelW );
            }
            h += labelH + d_data->spacing;
        }
    }

    // Compute the legend contribution

    const QwtAbstractLegend *legend = plot->legend();
    if ( legend && !legend->isEmpty() )
    {
        if ( d_data->legendPos == QwtPlot::LeftLegend
            || d_data->legendPos == QwtPlot::RightLegend )
        {
            int legendW = legend->sizeHint().width();
            int legendH = legend->heightForWidth( legendW );

            if ( legend->frameWidth() > 0 )
                w += d_data->spacing;

            if ( legendH > h )
                legendW += legend->scrollExtent( Qt::Horizontal );

            if ( d_data->legendRatio < 1.0 )
                legendW = qMin( legendW, int( w / ( 1.0 - d_data->legendRatio ) ) );

            w += legendW + d_data->spacing;
        }
        else // QwtPlot::Top, QwtPlot::Bottom
        {
            int legendW = qMin( legend->sizeHint().width(), w );
            int legendH = legend->heightForWidth( legendW );

            if ( legend->frameWidth() > 0 )
                h += d_data->spacing;

            if ( d_data->legendRatio < 1.0 )
                legendH = qMin( legendH, int( h / ( 1.0 - d_data->legendRatio ) ) );

            h += legendH + d_data->spacing;
        }
    }

    return QSize( w, h );
}

/*!
  Find the geometry for the legend

  \param options Options how to layout the legend
  \param rect Rectangle where to place the legend

  \return Geometry for the legend
  \sa Options
*/

QRectF QwtPlotLayout::layoutLegend(Options options,
    const QRectF &rect) const
{
    QSize hint(d_data->layoutData.legend.hint);
    if (d_data->legendPos == QwtPlot::FloatLegend)
    {
        hint = d_data->layoutData.legend.maxHint;
    }
    int dim;
    if (d_data->legendPos == QwtPlot::LeftLegend
        || d_data->legendPos == QwtPlot::RightLegend)
    {
        // We don't allow vertical legends to take more than
        // half of the available space.

        dim = qMin(hint.width(), int(rect.width() * d_data->legendRatio));

        if (!(options & IgnoreScrollbars))
        {
            if (hint.height() > rect.height())
            {
                // The legend will need additional
                // space for the vertical scrollbar.

                dim += d_data->layoutData.legend.hScrollExtent;
            }
        }
    }
    else
    {
        dim = qMin(hint.height(), int(rect.height() * d_data->legendRatio));
        dim = qMax(dim, d_data->layoutData.legend.vScrollExtent);
    }

    //图例是否出界
    bool is_normal_pos = d_data->legendFloatPos == QwtPlot::FreePosition;
    bool out_of_plot_x_left = false;
    bool out_of_plot_y_top = false;
    bool out_of_plot_x_right = false;
    bool out_of_plot_y_bottom = false;
    out_of_plot_x_left |= (d_data->layoutData.legend.xOffset < 10);
    out_of_plot_y_top |= (d_data->layoutData.legend.yOffset < 10);
    out_of_plot_x_right |= (d_data->layoutData.legend.xOffset + hint.width() + 10> d_data->layoutData.legend.xTotalSize);
    out_of_plot_y_bottom |= (d_data->layoutData.legend.yOffset + hint.height() + 10> d_data->layoutData.legend.yTotalSize);
    //out_of_plot |= hint.width() > rect.width();
    //out_of_plot |= hint.height() > rect.height();
    if (d_data->layoutData.legend.xTotalSize != 0 && out_of_plot_x_right && is_normal_pos)
    {
        d_data->layoutData.legend.xOffset += (rect.width() - d_data->layoutData.legend.xTotalSize);
    }
    else if (d_data->layoutData.legend.xTotalSize != 0 && !out_of_plot_x_left && is_normal_pos)
    {
        double x_ratio = (d_data->layoutData.legend.xOffset + hint.width() / 2 ) / (d_data->layoutData.legend.xTotalSize);
        d_data->layoutData.legend.xOffset = x_ratio * rect.width() - hint.width() / 2;
        d_data->layoutData.legend.xOffset = qMax(10.0, d_data->layoutData.legend.xOffset);
        d_data->layoutData.legend.xOffset = qMin(rect.width() - hint.width() - 10, d_data->layoutData.legend.xOffset);
    }

    if (d_data->layoutData.legend.yTotalSize != 0 && out_of_plot_y_bottom && is_normal_pos)
    {
        d_data->layoutData.legend.yOffset += (rect.height() - d_data->layoutData.legend.yTotalSize);
    }
    else if (d_data->layoutData.legend.yTotalSize != 0 && !out_of_plot_y_top && is_normal_pos)
    {
        double y_ratio = (d_data->layoutData.legend.yOffset + hint.height() / 2) / (d_data->layoutData.legend.yTotalSize);
        d_data->layoutData.legend.yOffset = y_ratio * rect.height() - hint.height() / 2;
        d_data->layoutData.legend.yOffset = qMax(10.0, d_data->layoutData.legend.yOffset);
        d_data->layoutData.legend.yOffset = qMin(rect.height() - hint.height() - 10, d_data->layoutData.legend.yOffset);
    }

    if (d_data->legendFloatPos == QwtPlot::TopLeft)
    {
        d_data->layoutData.legend.xOffset = 0;
        d_data->layoutData.legend.yOffset = 0;
    }
    else if (d_data->legendFloatPos == QwtPlot::TopCenter)
    {
        d_data->layoutData.legend.xOffset = rect.center().x() - hint.width() / 2 - rect.left();
        d_data->layoutData.legend.yOffset = 0;
    }
    else if (d_data->legendFloatPos == QwtPlot::TopRight)
    {
        d_data->layoutData.legend.xOffset = rect.right() - hint.width() - rect.left();
        d_data->layoutData.legend.yOffset = 0;
    }
    else if (d_data->legendFloatPos == QwtPlot::LeftCenter)
    {
        d_data->layoutData.legend.xOffset = 0;
        d_data->layoutData.legend.yOffset = rect.center().y() - hint.height() / 2 - rect.top();
    }
    else if (d_data->legendFloatPos == QwtPlot::RightCenter)
    {
        d_data->layoutData.legend.xOffset = rect.right() - hint.width() - rect.left();
        d_data->layoutData.legend.yOffset = rect.center().y() - hint.height() / 2 - rect.top();
    }
    else if (d_data->legendFloatPos == QwtPlot::BottomLeft)
    {
        d_data->layoutData.legend.xOffset = 0;
        d_data->layoutData.legend.yOffset = rect.bottom() - hint.height() - rect.top();
    }
    else if (d_data->legendFloatPos == QwtPlot::BottomCenter)
    {
        d_data->layoutData.legend.xOffset = rect.center().x() - hint.width() / 2 - rect.left();
        d_data->layoutData.legend.yOffset = rect.bottom() - hint.height() - rect.top();
    }
    else if (d_data->legendFloatPos == QwtPlot::BottomRight)
    {
        d_data->layoutData.legend.xOffset = rect.right() - hint.width() - rect.left();
        d_data->layoutData.legend.yOffset = rect.bottom() - hint.height() - rect.top();
    }

    d_data->layoutData.legend.xTotalSize = rect.width();
    d_data->layoutData.legend.yTotalSize = rect.height();
    QRectF legendRect = rect;
    switch ( d_data->legendPos )
    {
        case QwtPlot::LeftLegend:
            legendRect.setWidth( dim );
            break;
        case QwtPlot::RightLegend:
            legendRect.setX( rect.right() - dim );
            legendRect.setWidth( dim );
            break;
        case QwtPlot::TopLegend:
            legendRect.setHeight( dim );
            break;
        case QwtPlot::BottomLegend:
            legendRect.setY( rect.bottom() - dim );
            legendRect.setHeight( dim );
            break;
        case QwtPlot::FloatLegend:
            //TODO:@zhenghy 此处的数值与qwtplot的margin有关
            legendRect.setX( d_data->layoutData.legend.xOffset + rect.left());
            legendRect.setY( d_data->layoutData.legend.yOffset + rect.top()); 
            legendRect.setX(legendRect.x() + hint.width() < 5 ? 5 - hint.width() : legendRect.x());
            legendRect.setY(legendRect.y() + hint.height() < 5 ? 5 - hint.height() : legendRect.y());
            legendRect.setX(legendRect.x() - 5 < rect.right() ? legendRect.x() : rect.right() + 5);
            legendRect.setY(legendRect.y() - 5 < rect.bottom() ? legendRect.y() : rect.bottom() + 5);
            d_data->layoutData.legend.xOffset = legendRect.x() - rect.left();
            d_data->layoutData.legend.yOffset = legendRect.y() - rect.top();
            legendRect.setWidth(hint.width());
            legendRect.setHeight(hint.height());
            break;
    }
    return legendRect;
}

/*!
  Align the legend to the canvas

  \param canvasRect Geometry of the canvas
  \param legendRect Maximum geometry for the legend

  \return Geometry for the aligned legend
*/
QRectF QwtPlotLayout::alignLegend( const QRectF &canvasRect,
    const QRectF &legendRect ) const
{
    QRectF alignedRect = legendRect;

    if ( d_data->legendPos == QwtPlot::BottomLegend
        || d_data->legendPos == QwtPlot::TopLegend )
    {
        if ( d_data->layoutData.legend.hint.width() < canvasRect.width() )
        {
            alignedRect.setX( canvasRect.x() );
            alignedRect.setWidth( canvasRect.width() );
        }
    }
    else if ( d_data->legendPos == QwtPlot::LeftLegend
        || d_data->legendPos == QwtPlot::RightLegend )
    {
        if ( d_data->layoutData.legend.hint.height() < canvasRect.height() )
        {
            alignedRect.setY( canvasRect.y() );
            alignedRect.setHeight( canvasRect.height() );
        }
    }

    return alignedRect;
}

/*!
  Expand all line breaks in text labels, and calculate the height
  of their widgets in orientation of the text.

  \param options Options how to layout the legend
  \param rect Bounding rectangle for title, footer, axes and canvas.
  \param dimTitle Expanded height of the title widget
  \param dimFooter Expanded height of the footer widget
  \param dimAxis Expanded heights of the axis in axis orientation.

  \sa Options
*/
void QwtPlotLayout::expandLineBreaks(const QwtPlot *plot, Options options, const QRectF &rect,
    int &dimTitle, int &dimFooter, int dimAxis[QwtPlot::axisCnt], int axisExHeight[QwtPlot::axisCnt] )
{
    dimFooter = 0;
    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
        dimAxis[axis] = axisExHeight[axis] = 0;

    int backboneOffset[QwtPlot::axisCnt];
    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
    {
        backboneOffset[axis] = 0;
        if ( !( options & IgnoreFrames ) )
            backboneOffset[axis] += d_data->layoutData.canvas.contentsMargins[ axis ];

        if ( !d_data->alignCanvasToScales[axis] )
            backboneOffset[axis] += d_data->canvasMargin[axis];
    }

    bool done = false;
    while ( !done )
    {
        done = true;

        // the size for the 4 axis depend on each other. Expanding
        // the height of a horizontal axis will shrink the height
        // for the vertical axis, shrinking the height of a vertical
        // axis will result in a line break what will expand the
        // width and results in shrinking the width of a horizontal
        // axis what might result in a line break of a horizontal
        // axis ... . So we loop as long until no size changes.
        if ( !( ( options & IgnoreFooter ) ||
            d_data->layoutData.footer.text.isEmpty() ) )
        {
            double w = rect.width();

            if ( d_data->layoutData.scale[QwtPlot::yLeft].isEnabled
                != d_data->layoutData.scale[QwtPlot::yRight].isEnabled )
            {
                // center to the canvas
                w -= dimAxis[QwtPlot::yLeft] + dimAxis[QwtPlot::yRight];
            }

            int d = qCeil( d_data->layoutData.footer.text.heightForWidth( w ) );
            if ( !( options & IgnoreFrames ) )
                d += 2 * d_data->layoutData.footer.frameWidth;

            if ( d > dimFooter )
            {
                dimFooter = d;
                done = false;
            }
        }
        //如果fixedDim为0，那么ScaleWidget的宽度等于刻度文本的长度加上标题的长度；
        //也就是说此时设置标题会影响ScaleWidget的宽度
        //如果fixedDim已经被设置，那么ScaleWidget的宽度就固定了。
        //此时设置标题不再影响ScaleWidget的宽度，但是可能会导致标题和刻度文本重叠
        for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
        {
            const struct LayoutData::t_scaleData &scaleData =
                d_data->layoutData.scale[axis];
            int d = scaleData.dimWithoutTitle;
            //更新指数坐标
            if ((axis == QwtPlot::xBottom || axis == QwtPlot::xTop) && plot->axisEnabled(axis) &&
                 plot->axisWidget(axis)->scaleDraw()->getScaleDrawMode() == QwtScaleDraw::DrawScaleIndexMode)
            {
                int textHeight = plot->axisWidget(axis)->scaleIndexSize().width();
                axisExHeight[axis] = textHeight;
            }
            else if (plot->axisEnabled(axis) &&
                     plot->axisWidget(axis)->scaleDraw()->getScaleDrawMode() == QwtScaleDraw::DrawScaleIndexMode)
            {
                int textHeight = plot->axisWidget(axis)->scaleIndexSize().height();
                axisExHeight[axis] = textHeight;
            }
            if ( scaleData.isEnabled )
            {
                if ( !scaleData.scaleWidget->title().isEmpty() )
                {
                    d += scaleData.scaleWidget->titleHeightForWidth( qFloor( QWIDGETSIZE_MAX ) );
                }


                if ( d > dimAxis[axis] )
                {
                    done = false;
                }
            }
            dimAxis[axis] = d;
        }
    }
}

int QwtPlotLayout::fixedDimOfAxis(int axisId) const
{
    return d_data->layoutData.scale[axisId].fixedDim;
}

void QwtPlotLayout::setFixedDimOfAxis(int axisId, int dimLength)
{
    d_data->layoutData.scale[axisId].fixedDim = dimLength;
}

int QwtPlotLayout::hintDimOfAxis(int axisId) const
{
    return d_data->layoutData.scale[axisId].hintDim;
}

void QwtPlotLayout::setHintDimOfAxis(int axisId, int dimLength)
{
    d_data->layoutData.scale[axisId].hintDim = dimLength;
}

int QwtPlotLayout::dimOfAxis(int axisId) const
{
    return d_data->layoutData.scale[axisId].dim;
}

void QwtPlotLayout::setDimOfAxis(int axis, int dimLen)
{
   d_data->layoutData.scale[axis].dim = dimLen;
}

int QwtPlotLayout::marginOfAxis(int axisId) const
{
    return d_data->layoutData.scale[axisId].margin;
}

void QwtPlotLayout::setMarginOfAxis(int axisId, int margin)
{
    d_data->layoutData.scale[axisId].margin = margin;
}

void QwtPlotLayout::setHintMarginOfAxis(int axisId, int margin)
{
    d_data->layoutData.scale[axisId].hintMargin = margin;
}

int QwtPlotLayout::hintMarginOfAxis(int axisId) const
{
    return d_data->layoutData.scale[axisId].hintMargin;
}

/*!
  Align the ticks of the axis to the canvas borders using
  the empty corners.

  \param options Layout options
  \param canvasRect Geometry of the canvas ( IN/OUT )
  \param scaleRect Geometries of the scales ( IN/OUT )

  \sa Options
*/

void QwtPlotLayout::alignScales( Options options,
    QRectF &canvasRect, QRectF scaleRect[QwtPlot::axisCnt] ) const
{
    int backboneOffset[QwtPlot::axisCnt];
    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
    {
        backboneOffset[axis] = 0;

        if ( !d_data->alignCanvasToScales[axis] )
        {
            backboneOffset[axis] += d_data->canvasMargin[axis];
        }

        if ( !( options & IgnoreFrames ) )
        {
            backboneOffset[axis] += 
                d_data->layoutData.canvas.contentsMargins[axis];
        }
    }

    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
    {
        if ( !scaleRect[axis].isValid() )
            continue;

        const int startDist = d_data->layoutData.scale[axis].start;
        const int endDist = d_data->layoutData.scale[axis].end;

        QRectF &axisRect = scaleRect[axis];

        if ( axis == QwtPlot::xTop || axis == QwtPlot::xBottom )
        {
            const QRectF &leftScaleRect = scaleRect[QwtPlot::yLeft];
            const int leftOffset =
                backboneOffset[QwtPlot::yLeft] - startDist;

            if ( leftScaleRect.isValid() )
            {
                const double dx = leftOffset + leftScaleRect.width();
                if ( d_data->alignCanvasToScales[QwtPlot::yLeft] && dx < 0.0 )
                {
                    /*
                      The axis needs more space than the width
                      of the left scale.
                     */
                    const double cLeft = canvasRect.left(); // qreal -> double
                    canvasRect.setLeft( qMax( cLeft, axisRect.left() - dx ) );
                }
                else
                {
                    const double minLeft = leftScaleRect.left();
                    const double left = axisRect.left() + leftOffset;
                    axisRect.setLeft( qMax( left, minLeft ) );
                }
            }
            else
            {
                if ( d_data->alignCanvasToScales[QwtPlot::yLeft] && leftOffset < 0 )
                {
                    canvasRect.setLeft( qMax( canvasRect.left(),
                        axisRect.left() - leftOffset ) );
                }
                else
                {
                    if ( leftOffset > 0 )
                        axisRect.setLeft( axisRect.left() + leftOffset );
                }
            }

            const QRectF &rightScaleRect = scaleRect[QwtPlot::yRight];
            const int rightOffset =
                backboneOffset[QwtPlot::yRight] - endDist + 1;

            if ( rightScaleRect.isValid() )
            {
                const double dx = rightOffset + rightScaleRect.width();
                if ( d_data->alignCanvasToScales[QwtPlot::yRight] && dx < 0 )
                {
                    /*
                      The axis needs more space than the width
                      of the right scale.
                     */
                    const double cRight = canvasRect.right(); // qreal -> double
                    canvasRect.setRight( qMin( cRight, axisRect.right() + dx ) );
                }   

                const double maxRight = rightScaleRect.right();
                const double right = axisRect.right() - rightOffset;
                axisRect.setRight( qMin( right, maxRight ) );
            }
            else
            {
                if ( d_data->alignCanvasToScales[QwtPlot::yRight] && rightOffset < 0 )
                {
                    canvasRect.setRight( qMin( canvasRect.right(),
                        axisRect.right() + rightOffset ) );
                }
                else
                {
                    if ( rightOffset > 0 )
                        axisRect.setRight( axisRect.right() - rightOffset );
                }
            }
        }
        else // QwtPlot::yLeft, QwtPlot::yRight
        {
            const QRectF &bottomScaleRect = scaleRect[QwtPlot::xBottom];
            const int bottomOffset =
                backboneOffset[QwtPlot::xBottom] - endDist + 1;

            if ( bottomScaleRect.isValid() )
            {
                const double dy = bottomOffset + bottomScaleRect.height();
                if ( d_data->alignCanvasToScales[QwtPlot::xBottom] && dy < 0 )
                {
                    /*
                      The axis needs more space than the height
                      of the bottom scale.
                     */
                    const double cBottom = canvasRect.bottom(); // qreal -> double
                    canvasRect.setBottom( qMin( cBottom, axisRect.bottom() + dy ) );
                }
                else
                {
                    const double maxBottom = bottomScaleRect.top() +
                        d_data->layoutData.scale[QwtPlot::xBottom].tickOffset;
                    const double bottom = axisRect.bottom() - bottomOffset;
                    axisRect.setBottom( qMin( bottom, maxBottom ) );
                }
            }
            else
            {
                if ( d_data->alignCanvasToScales[QwtPlot::xBottom] && bottomOffset < 0 )
                {
                    canvasRect.setBottom( qMin( canvasRect.bottom(),
                        axisRect.bottom() + bottomOffset ) );
                }
                else
                {
                    if ( bottomOffset > 0 )
                        axisRect.setBottom( axisRect.bottom() - bottomOffset );
                }
            }

            const QRectF &topScaleRect = scaleRect[QwtPlot::xTop];
            const int topOffset = backboneOffset[QwtPlot::xTop] - startDist;

            if ( topScaleRect.isValid() )
            {
                const double dy = topOffset + topScaleRect.height();
                if ( d_data->alignCanvasToScales[QwtPlot::xTop] && dy < 0 )
                {
                    /*
                      The axis needs more space than the height
                      of the top scale.
                     */
                    const double cTop = canvasRect.top(); // qreal -> double
                    canvasRect.setTop( qMax( cTop, axisRect.top() - dy ) );
                }
                else
                {
                    const double minTop = topScaleRect.bottom() -
                        d_data->layoutData.scale[QwtPlot::xTop].tickOffset;
                    const double top = axisRect.top() + topOffset;
                    axisRect.setTop( qMax( top, minTop ) );
                }
            }
            else
            {
                if ( d_data->alignCanvasToScales[QwtPlot::xTop] && topOffset < 0 )
                {
                    canvasRect.setTop( qMax( canvasRect.top(),
                        axisRect.top() - topOffset ) );
                }
                else
                {
                    if ( topOffset > 0 )
                        axisRect.setTop( axisRect.top() + topOffset );
                }
            }
        }
    }

    /*
      The canvas has been aligned to the scale with largest
      border distances. Now we have to realign the other scale.
     */


    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
    {
        QRectF &sRect = scaleRect[axis];

        if ( !sRect.isValid() )
            continue;

        if ( axis == QwtPlot::xBottom || axis == QwtPlot::xTop )
        {
            if ( d_data->alignCanvasToScales[QwtPlot::yLeft] )
            {
                double y = canvasRect.left() - d_data->layoutData.scale[axis].start;
                if ( !( options & IgnoreFrames ) )
                    y += d_data->layoutData.canvas.contentsMargins[ QwtPlot::yLeft ];

                sRect.setLeft( y );
            }
            if ( d_data->alignCanvasToScales[QwtPlot::yRight] )
            {
                double y = canvasRect.right() - 1 + d_data->layoutData.scale[axis].end;
                if ( !( options & IgnoreFrames ) )
                    y -= d_data->layoutData.canvas.contentsMargins[ QwtPlot::yRight ];

                sRect.setRight( y );
            }

            if ( d_data->alignCanvasToScales[ axis ] )
            {
                if ( axis == QwtPlot::xTop )
                    sRect.setBottom( canvasRect.top() );
                else
                    sRect.setTop( canvasRect.bottom() );
            }
        }
        else
        {
            if ( d_data->alignCanvasToScales[QwtPlot::xTop] )
            {
                double x = canvasRect.top() - d_data->layoutData.scale[axis].start;
                if ( !( options & IgnoreFrames ) )
                    x += d_data->layoutData.canvas.contentsMargins[ QwtPlot::xTop ];

                sRect.setTop( x );
            }
            if ( d_data->alignCanvasToScales[QwtPlot::xBottom] )
            {
                double x = canvasRect.bottom() - 1 + d_data->layoutData.scale[axis].end;
                if ( !( options & IgnoreFrames ) )
                    x -= d_data->layoutData.canvas.contentsMargins[ QwtPlot::xBottom ];

                sRect.setBottom( x );
            }

            if ( d_data->alignCanvasToScales[ axis ] )
            {
                if ( axis == QwtPlot::yLeft )
                    sRect.setRight( canvasRect.left() );
                else
                    sRect.setLeft( canvasRect.right() );
            }
        }
    }
}

/*!
  \brief Recalculate the geometry of all components.

  \param plot Plot to be layout
  \param plotRect Rectangle where to place the components
  \param options Layout options

  \sa invalidate(), titleRect(), footerRect()
      legendRect(), scaleRect(), canvasRect()
*/
void QwtPlotLayout::activate( const QwtPlot *plot,
    const QRectF &plotRect, Options options )
{
    invalidate();

    QRectF rect( plotRect );  // undistributed rest of the plot rect

    // We extract all layout relevant parameters from the widgets,
    // and save them to d_data->layoutData.

    d_data->layoutData.init( plot, rect );    
    //坐标轴到窗口的边距
    int axisMargin[QwtPlot::axisCnt] = {0, 0, 0, 0};
    //rect已经减去的位置，作最终计算画布位置用
    int axisSubtracted[QwtPlot::axisCnt] = {0, 0, 0, 0};

    int dimTitle, dimFooter, dimAxes[QwtPlot::axisCnt], axisExHeight[QwtPlot::axisCnt];
    dimTitle = 0;
    if (!((options & IgnoreTitle) ||
        d_data->layoutData.title.text.isEmpty()))
    {
        double w = rect.width();

        dimTitle = qCeil(d_data->layoutData.title.text.heightForWidth(w));
        if (!(options & IgnoreFrames))
            dimTitle += 2 * d_data->layoutData.title.frameWidth;

        axisSubtracted[QwtPlot::xTop] += (dimTitle + d_data->spacing);
        d_data->titleRect.setRect(
            rect.left(), rect.top(), rect.width(), dimTitle);

        rect.setTop(d_data->titleRect.bottom() + d_data->spacing);
    }

    if ( !( options & IgnoreLegend )
        && plot->legend() && !plot->legend()->isEmpty() && !(legendPosition() == QwtPlot::HideLegend))
    {
        d_data->legendRect = layoutLegend( options, rect );
        
        // subtract d_data->legendRect from rect
        if(d_data->legendPos != QwtPlot::FloatLegend)
        {
            const QRegion region( rect.toRect() );
            rect = region.subtracted( d_data->legendRect.toRect() ).boundingRect();
        }

        switch ( d_data->legendPos )
        {
            case QwtPlot::LeftLegend:
                rect.setLeft( rect.left() + d_data->spacing );
                axisMargin[QwtPlot::yLeft] += d_data->legendRect.width() + d_data->spacing;
                axisSubtracted[QwtPlot::yLeft] += d_data->legendRect.width() + d_data->spacing;
                break;
            case QwtPlot::RightLegend:
                rect.setRight( rect.right() - d_data->spacing );
                axisMargin[QwtPlot::yRight] += d_data->legendRect.width() + d_data->spacing;
                axisSubtracted[QwtPlot::yRight] += d_data->legendRect.width() + d_data->spacing;
                break;
            case QwtPlot::TopLegend:
                rect.setTop( rect.top() + d_data->spacing );
                axisMargin[QwtPlot::xTop] += d_data->legendRect.height() + d_data->spacing;
                axisSubtracted[QwtPlot::xTop] += d_data->legendRect.height() + d_data->spacing;
                break;
            case QwtPlot::BottomLegend:
                rect.setBottom( rect.bottom() - d_data->spacing );
                axisMargin[QwtPlot::xBottom] += d_data->legendRect.height() + d_data->spacing;
                axisSubtracted[QwtPlot::xBottom] += d_data->legendRect.height() + d_data->spacing;
                break;
        }
    }
    //如果没有legend 则yleft轴的指数刻度值需要撑开一定的宽度保证可以显示出来

    /*
     +---+-----------+---+
     |       Title       |
     +---+-----------+---+
     |   |   Axis    |   |
     +---+-----------+---+
     | A |           | A |
     | x |  Canvas   | x |
     | i |           | i |
     | s |           | s |
     +---+-----------+---+
     |   |   Axis    |   |
     +---+-----------+---+
     |      Footer       |
     +---+-----------+---+
    */

    // title, footer and axes include text labels. The height of each
    // label depends on its line breaks, that depend on the width
    // for the label. A line break in a horizontal text will reduce
    // the available width for vertical texts and vice versa.
    // expandLineBreaks finds the height/width for title, footer and axes
    // including all line breaks.

    expandLineBreaks(plot, options, rect, dimTitle, dimFooter, dimAxes, axisExHeight);
    dimAxes[QwtPlot::xTop] = d_data->layoutData.scale[QwtPlot::xTop].fixedDim != 0 ?
                             dimAxes[QwtPlot::xTop] : qMax(dimAxes[QwtPlot::xTop], axisExHeight[QwtPlot::yLeft]);
    dimAxes[QwtPlot::xTop] = d_data->layoutData.scale[QwtPlot::xTop].fixedDim != 0 ?
                             dimAxes[QwtPlot::xTop] : qMax(dimAxes[QwtPlot::xTop], axisExHeight[QwtPlot::yRight]);
    dimAxes[QwtPlot::yRight] = d_data->layoutData.scale[QwtPlot::yRight].fixedDim != 0 ?
                               dimAxes[QwtPlot::yRight] : qMax(dimAxes[QwtPlot::yRight], axisExHeight[QwtPlot::xTop]);
    dimAxes[QwtPlot::yRight] = d_data->layoutData.scale[QwtPlot::yRight].fixedDim != 0 ?
                               dimAxes[QwtPlot::yRight] : qMax(dimAxes[QwtPlot::yRight], axisExHeight[QwtPlot::xBottom]);

    axisMargin[QwtPlot::xTop] = axisMargin[QwtPlot::xTop] + ( dimTitle > 0 ? dimTitle + d_data->spacing : 0);
    axisMargin[QwtPlot::xBottom] = axisMargin[QwtPlot::xBottom] + ( dimFooter > 0 ? dimFooter + d_data->spacing : 0);
    //在画布宽度被压缩到很小时，直接舍弃除画布之外的所有区域，只保留canvas区域
    //在yRight轴显示的情况下，需要把xBottomIndexScale往下挪，以保证xBottomIndexScale不会与yRight
    //轴的刻度值重合
    //在yRight轴隐藏的情况下，canvas会直接顶到最右边，因此需要单独拿出一块区域供xBottomIndexScale使用
    //但是在DimOfAxis被设置的情况下，认为坐标轴固定，因此不再顶出一块区域
    for (int axis = 0; axis < QwtPlot::axisCnt; axis++)
    {
        //autoscaledim表示无hintDim时原本应该具有的宽度
        int autoScaleDim = plot->axisEnabled( axis ) ? d_data->layoutData.scale[axis].scaleWidget->dimForLength(
            QWIDGETSIZE_MAX, d_data->layoutData.scale[axis].scaleFont ) : 0;
        //某一个轴的指数坐标刻度文本可能会撑开其他坐标轴的宽度
        if (axis == QwtPlot::yRight )
        {
            autoScaleDim = qMax(autoScaleDim, axisExHeight[QwtPlot::xBottom]);
            autoScaleDim = qMax(autoScaleDim, axisExHeight[QwtPlot::xTop]);
        }
        else if (axis == QwtPlot::xTop)
        {    
            autoScaleDim = qMax(autoScaleDim, axisExHeight[QwtPlot::yLeft]);
            autoScaleDim = qMax(autoScaleDim, axisExHeight[QwtPlot::yRight]);
        }
        setDimOfAxis(axis, autoScaleDim);
        int dimBefore = d_data->layoutData.scale[axis].dimBefore;
        //各轴的Margin也要保持一致
        setMarginOfAxis(axis, axisMargin[axis]);
        int marginBefore = d_data->layoutData.scale[axis].marginBefore;
        
        bool emitSignal = (dimBefore != dimOfAxis(axis)) || (marginBefore != axisMargin[axis]);
        d_data->layoutData.scale[axis].dimBefore = dimOfAxis(axis);
        d_data->layoutData.scale[axis].marginBefore = axisMargin[axis];
        //当前曲线窗口的Dim发生改变（即：dim!=dimbefore）时，需要发送信号，让所有的子窗口边距都统一
        //无论最终绘图是采用Dim还是HintDim，都必须发送一次信号
        //需要避免反复递归调用问题，先设置dimbefore再调用信号。
        //获取的是其他曲线窗口上一时刻的dim，始终具有滞后性。
        if (emitSignal)
        {
            d_data->dimChanged = true;
        }
        if (fixedDimOfAxis(axis) == 0)
        {
            dimAxes[axis] = qMax(dimAxes[axis], hintDimOfAxis(axis));
        }
        axisMargin[axis] = qMax(axisMargin[axis], hintMarginOfAxis(axis));
    }

    if ( dimFooter > 0 )
    {
        axisSubtracted[QwtPlot::xBottom] += (dimFooter + d_data->spacing);
        d_data->footerRect.setRect(
            rect.left(), rect.bottom() - dimFooter, rect.width(), dimFooter );

        rect.setBottom( d_data->footerRect.top() - d_data->spacing );

        if ( d_data->layoutData.scale[QwtPlot::yLeft].isEnabled !=
            d_data->layoutData.scale[QwtPlot::yRight].isEnabled )
        {
            // if only one of the y axes is missing we align
            // the footer centered to the canvas

            d_data->footerRect.setX( rect.left() + dimAxes[QwtPlot::yLeft] );
            d_data->footerRect.setWidth( rect.width()
                - dimAxes[QwtPlot::yLeft] - dimAxes[QwtPlot::yRight] );
        }
    }

    rect.setTop( rect.top() + axisMargin[QwtPlot::xTop] - axisSubtracted[QwtPlot::xTop]);
    rect.setBottom( rect.bottom() - axisMargin[QwtPlot::xBottom] + axisSubtracted[QwtPlot::xBottom]);
    rect.setLeft( rect.left() + axisMargin[QwtPlot::yLeft] - axisSubtracted[QwtPlot::yLeft]);
    rect.setRight( rect.right() - axisMargin[QwtPlot::yRight] + axisSubtracted[QwtPlot::yRight]);
    d_data->canvasRect.setRect(
        rect.x() + dimAxes[QwtPlot::yLeft],
        rect.y() + dimAxes[QwtPlot::xTop],
        rect.width() - dimAxes[QwtPlot::yRight] - dimAxes[QwtPlot::yLeft],
        rect.height() - dimAxes[QwtPlot::xBottom] - dimAxes[QwtPlot::xTop] );

    //scale小于40则隐藏
    //当canvasRect很小时，canvasRect直接扩张到整个窗口内，坐标轴和画布边距完全一致，同时不再绘制除画布之外的任何组件
    if ( d_data->canvasRect.height() < d_data->minCanvasHeight
        || d_data->canvasRect.width() < d_data->minCanvasWidth)
    {
        //if(d_data->legendPos != QwtPlot::FloatLegend) //无论是否为浮动图例，都隐藏
        {
            d_data->legendRect = QRectF();
        }
        d_data->canvasRect = plotRect;
        for ( int axisId = 0; axisId < QwtPlot::axisCnt; axisId++ )
        {
            if( !plot->axisEnabled( axisId ) )
            {
                continue;
            }

            QRectF &scaleRect = d_data->scaleRect[axisId];
            scaleRect = d_data->canvasRect;
            switch ( axisId )
            {
            case QwtPlot::yLeft:
                scaleRect.setX( d_data->canvasRect.left() - 1 );
                scaleRect.setWidth( 1 );
                break;
            case QwtPlot::yRight:
                scaleRect.setX( d_data->canvasRect.right() );
                scaleRect.setWidth( 1 );
                break;
            case QwtPlot::xBottom:
                scaleRect.setY( d_data->canvasRect.bottom() );
                scaleRect.setHeight( 1 );
                break;
            case QwtPlot::xTop:
                scaleRect.setY( d_data->canvasRect.top() - 1 );
                scaleRect.setHeight( 1 );
                break;
            }
        }
        setAllScaleVisible( false );
        return;
    }

    //scale 可见
    setAllScaleVisible( true );
    for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
    {
        // set the rects for the axes and scale indexes
        if ( dimAxes[axis] )
        {
            //计算坐标轴指数刻度的文本宽度和长度
            const QSizeF textSize = plot->ScaleIndexLabel(axis).textSize();

            int dim = dimAxes[axis];
            QRectF &scaleRect = d_data->scaleRect[axis];
            scaleRect = d_data->canvasRect;
            switch ( axis )
            {
                case QwtPlot::yLeft:
                    scaleRect.setX( d_data->canvasRect.left() - dim );
                    scaleRect.setWidth( dim );
                    scaleRect.setY(scaleRect.y() - axisExHeight[axis]);
                    break;
                case QwtPlot::yRight:
                    scaleRect.setX( d_data->canvasRect.right() );
                    scaleRect.setWidth( dim );
                    scaleRect.setY(scaleRect.y() - axisExHeight[axis]);
                    break;
                case QwtPlot::xBottom:
                    scaleRect.setY( d_data->canvasRect.bottom() );
                    scaleRect.setHeight( dim );
                    scaleRect.setWidth(scaleRect.width() + axisExHeight[axis]);
                    break;
                case QwtPlot::xTop:
                    scaleRect.setY( d_data->canvasRect.top() - dim );
                    scaleRect.setHeight( dim );
                    break;
            }
            scaleRect = scaleRect.normalized();
        }
    }

    // +---+-----------+---+
    // |  <-   Axis   ->   |
    // +-^-+-----------+-^-+
    // | | |           | | |
    // |   |           |   |
    // | A |           | A |
    // | x |  Canvas   | x |
    // | i |           | i |
    // | s |           | s |
    // |   |           |   |
    // | | |           | | |
    // +-V-+-----------+-V-+
    // |   <-  Axis   ->   |
    // +---+-----------+---+

    // The ticks of the axes - not the labels above - should
    // be aligned to the canvas. So we try to use the empty
    // corners to extend the axes, so that the label texts
    // left/right of the min/max ticks are moved into them.

    // alignScales() 用于对齐坐标轴和画布，在增加指数刻度后，此算法会把坐标轴挤压，因此避免调用此函数，未发现有副作用
    //alignScales( options, d_data->canvasRect, d_data->scaleRect );
    //qDebug() << canvasRect() << scaleRect(QwtPlot::yLeft);
    if ( !d_data->legendRect.isEmpty() )
    {
        // We prefer to align the legend to the canvas - not to
        // the complete plot - if possible.

        d_data->legendRect = alignLegend( d_data->canvasRect, d_data->legendRect );
    }
}