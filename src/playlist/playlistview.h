#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QHeaderView>
#include <QTreeView>
#include <QTimer>
#include <QSortFilterProxyModel>

#include "tomahawk/source.h"
#include "plitem.h"
#include "trackmodel.h"
#include "playlistitemdelegate.h"

class PlaylistModel;
class PlaylistProxyModel;
class PlaylistInterface;
class TrackProxyModel;

class PlaylistView : public QTreeView
{
Q_OBJECT

public:
    explicit PlaylistView( QWidget* parent = 0 );
    ~PlaylistView();

    TrackModel* model() { return m_model; }
    TrackProxyModel* proxyModel() { return (TrackProxyModel*)m_proxyModel; }
    PlaylistItemDelegate* delegate() { return m_delegate; }

    void setModel( TrackModel* model, PlaylistInterface* modelInterface );

protected:
    virtual void resizeEvent( QResizeEvent* event );
    virtual void keyPressEvent( QKeyEvent* event );

    virtual void dragEnterEvent( QDragEnterEvent* event );
    virtual void dragLeaveEvent( QDragLeaveEvent* event ) { m_dragging = false; setDirtyRegion( m_dropRect ); }
    virtual void dragMoveEvent( QDragMoveEvent* event );
    virtual void dropEvent( QDropEvent* event );

    void paintEvent( QPaintEvent* event );

private slots:
    void onItemActivated( const QModelIndex& index );
    void onItemResized( const QModelIndex& index );

    void resizeColumns();
    void onSectionResized( int logicalIndex, int oldSize, int newSize );

    void onFilterChanged( const QString& filter );

private:
    void restoreColumnsState();
    void saveColumnsState();

    TrackModel* m_model;
    PlaylistInterface* m_modelInterface;
    PlaylistProxyModel* m_proxyModel;
    PlaylistItemDelegate* m_delegate;

    QList<double> m_columnWeights;
    QList<int> m_columnWidths;

    bool m_resizing;
    bool m_dragging;
    QRect m_dropRect;
};

#endif // PLAYLISTVIEW_H
