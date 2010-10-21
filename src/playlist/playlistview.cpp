#include "playlistview.h"

#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>

#include "tomahawk/tomahawkapp.h"
#include "audioengine.h"
#include "playlistmodel.h"
#include "playlistproxymodel.h"
#include "trackproxymodel.h"
#include "tomahawksettings.h"

using namespace Tomahawk;


PlaylistView::PlaylistView( QWidget* parent )
    : QTreeView( parent )
    , m_model( 0 )
    , m_proxyModel( new PlaylistProxyModel( this ) )
    , m_delegate( new PlaylistItemDelegate( this, m_proxyModel ) )
    , m_resizing( false )
{
    setSortingEnabled( false );
    setAlternatingRowColors( true );
    setMouseTracking( true );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setDragEnabled( true );
    setDropIndicatorShown( false );
    setDragDropMode( QAbstractItemView::InternalMove );
    setDragDropOverwriteMode ( false );
    setAllColumnsShowFocus( true );

    setItemDelegate( m_delegate );
    m_proxyModel->setSourceModel( m_model );

    header()->setMinimumSectionSize( 60 );
    restoreColumnsState();

    connect( this, SIGNAL( activated( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( header(), SIGNAL( sectionResized( int, int, int ) ), SLOT( onSectionResized( int, int, int ) ) );
    connect( m_proxyModel, SIGNAL( filterChanged( QString ) ), SLOT( onFilterChanged( QString ) ) );

    QTreeView::setModel( m_proxyModel );
}


PlaylistView::~PlaylistView()
{
    qDebug() << Q_FUNC_INFO;

    saveColumnsState();
}


void
PlaylistView::setModel( TrackModel* model, PlaylistInterface* modelInterface )
{
    m_model = model;
    m_modelInterface = modelInterface;
    m_proxyModel->setSourceModel( model );

    connect( m_model, SIGNAL( itemSizeChanged( QModelIndex ) ), SLOT( onItemResized( QModelIndex ) ) );

    setAcceptDrops( true );
    setRootIsDecorated( false );
    setUniformRowHeights( true );
}


void
PlaylistView::restoreColumnsState()
{
    TomahawkSettings* s = APP->settings();
    QList<QVariant> list = s->playlistColumnSizes();

    if ( list.count() != 6 ) // FIXME: const
    {
        m_columnWeights << 0.22 << 0.29 << 0.19 << 0.08 << 0.08 << 0.14;
    }
    else
    {
        foreach( const QVariant& v, list )
            m_columnWeights << v.toDouble();
    }

    for ( int i = 0; i < m_columnWeights.count(); i++ )
        m_columnWidths << 0;
}


void
PlaylistView::saveColumnsState()
{
    TomahawkSettings *s = APP->settings();
    QList<QVariant> wlist;
//    int i = 0;

    foreach( double w, m_columnWeights )
    {
        wlist << QVariant( w );
//        qDebug() << "Storing weight for column" << i++ << w;
    }

    s->setPlaylistColumnSizes( wlist );
}


void
PlaylistView::onSectionResized( int logicalIndex, int oldSize, int newSize )
{
    return;
}


void
PlaylistView::onItemActivated( const QModelIndex& index )
{
    PlItem* item = ((PlaylistInterface*)m_model)->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item && item->query()->numResults() )
    {
        qDebug() << "Result activated:" << item->query()->toString() << item->query()->results().first()->url();
        m_proxyModel->setCurrentItem( index );
        APP->audioEngine()->playItem( m_proxyModel, item->query()->results().first() );
    }
}


void
PlaylistView::onItemResized( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;
    m_delegate->updateRowSize( index );
}


void
PlaylistView::resizeEvent( QResizeEvent* event )
{
//    qDebug() << Q_FUNC_INFO;
    resizeColumns();
}


void
PlaylistView::resizeColumns()
{
    double cw = contentsRect().width();
    int i = 0;
    int total = 0;

    QList<double> mcw = m_columnWeights;

    if ( verticalScrollBar() && verticalScrollBar()->isVisible() )
    {
        cw -= verticalScrollBar()->width() + 1;
    }

    m_resizing = true;
    foreach( double w, mcw )
    {
        int fw = (int)( cw * w );
        if ( fw < header()->minimumSectionSize() )
            fw = header()->minimumSectionSize();

        if ( i + 1 == header()->count() )
            fw = cw - total;

        total += fw;
//        qDebug() << "Resizing column:" << i << fw;

        m_columnWidths[ i ] = fw;

        header()->resizeSection( i++, fw );
    }
    m_resizing = false;
}


void
PlaylistView::keyPressEvent( QKeyEvent* event )
{
//    qDebug() << Q_FUNC_INFO;
    QTreeView::keyPressEvent( event );

    if ( !m_model )
        return;

    if ( event->key() == Qt::Key_Delete )
    {
/*        if ( m_model->isPlaylistBacked() && selectedIndexes().count() )
        {
            qDebug() << "Removing selected items";
            QList<PlaylistItem*> items;

            QModelIndexList sidxs = selectedIndexes();
            foreach( const QModelIndex& idx, sidxs )
            {
                if ( idx.column() > 0 )
                    continue;

                PlaylistItem* item = PlaylistModel::indexToPlaylistItem( idx );
                if ( item )
                    items << item;
            }

            m_model->removeItems( items );
        }*/
    }
}


void
PlaylistView::dragEnterEvent( QDragEnterEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    QTreeView::dragEnterEvent( event );

    if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" ) || event->mimeData()->hasFormat( "application/tomahawk.plentry.list" ) )
    {
        m_dragging = true;
        m_dropRect = QRect();

        qDebug() << "Accepting Drag Event";
        event->acceptProposedAction();
    }
}


void
PlaylistView::dragMoveEvent( QDragMoveEvent* event )
{
    QTreeView::dragMoveEvent( event );

/*    if ( m_model->isReadOnly() )
    {
        event->ignore();
        return;
    }*/

    if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" ) || event->mimeData()->hasFormat( "application/tomahawk.plentry.list" ) )
    {
        setDirtyRegion( m_dropRect );
        const QPoint pos = event->pos();
        const QModelIndex index = indexAt( pos );

        if ( index.isValid() )
        {
            const QRect rect = visualRect( index );
            m_dropRect = rect;

            // indicate that the item will be inserted above the current place
            const int gap = 5; // FIXME constant
            m_dropRect = QRect( rect.left(), rect.top() - gap / 2, rect.width(), gap );

            event->acceptProposedAction();
        }

        setDirtyRegion( m_dropRect );
    }
}


void
PlaylistView::dropEvent( QDropEvent* event )
{
/*    const QPoint pos = event->pos();
    const QModelIndex index = indexAt( pos );

    if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" ) )
    {
        const QPoint pos = event->pos();
        const QModelIndex index = indexAt( pos );

        if ( index.isValid() )
        {
            event->acceptProposedAction();
        }
    }*/

    QTreeView::dropEvent( event );
    m_dragging = false;
}


void
PlaylistView::paintEvent( QPaintEvent* event )
{
    QTreeView::paintEvent( event );

    if ( m_dragging )
    {
        // draw drop indicator
        QPainter painter( viewport() );
        {
            // draw indicator for inserting items
            QBrush blendedBrush = viewOptions().palette.brush( QPalette::Normal, QPalette::Highlight );
            QColor color = blendedBrush.color();

            const int y = ( m_dropRect.top() + m_dropRect.bottom() ) / 2;
            const int thickness = m_dropRect.height() / 2;

            int alpha = 255;
            const int alphaDec = alpha / ( thickness + 1 );
            for ( int i = 0; i < thickness; i++ )
            {
                color.setAlpha( alpha );
                alpha -= alphaDec;
                painter.setPen( color );
                painter.drawLine( 0, y - i, width(), y - i );
                painter.drawLine( 0, y + i, width(), y + i );
            }
        }
    }
}


void
PlaylistView::onFilterChanged( const QString& )
{
    if ( selectedIndexes().count() )
        scrollTo( selectedIndexes().at( 0 ), QAbstractItemView::PositionAtCenter );
}
