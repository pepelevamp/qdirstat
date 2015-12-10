/*
 *   File name: DirTree.h
 *   Summary:	Support classes for QDirStat
 *   License:	GPL V2 - See file LICENSE for details.
 *
 *   Author:	Stefan Hundhammer <Stefan.Hundhammer@gmx.de>
 */


#ifndef DirTree_h
#define DirTree_h


#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <stdlib.h>

#include "Logger.h"
#include "DirInfo.h"
#include "DirReadJob.h"


namespace QDirStat
{
    // Forward declarations
    class DirReadJob;


    /**
     * This class provides some infrastructure as well as global data for a
     * directory tree. It acts as the glue that holds things together: The root
     * item from which to descend into the subtrees, the read queue and some
     * global policies (like whether or not to cross file systems while reading
     * directories).
     *
     * Notice that this class uses a "pseudo root" to better conform with Qt's
     * notion of tree views and the corresponding data models: They use an
     * invisible root item to support multiple toplevel items.
     *
     * @short Directory tree global data and infrastructure
     **/
    class DirTree: public QObject
    {
	Q_OBJECT

    public:
	/**
	 * Constructor.
	 *
	 * Remember to call @ref startReading() after the constructor and
	 * setting up connections.
	 **/
	DirTree();

	/**
	 * Destructor.
	 **/
	virtual ~DirTree();


     public slots:

	/**
	 * Actually start reading.
	 *
	 * It's not very pretty this is required as an extra method, but this
	 * cannot simply be done in the constructor: We need to give the caller
	 * a chance to set up Qt signal connections, and for this the
	 * constructor must return before any signals are sent, i.e. before
	 * anything is read.
	 **/
	void startReading( const QString & path );

	/**
	 * Forcefully stop a running read process.
	 **/
	void abortReading();

	/**
	 * Refresh a subtree, i.e. read its contents from disk again.
	 *
	 * The old subtree will be deleted and rebuilt from scratch, i.e. all
	 * pointers to elements within this subtree will become invalid (a
	 * @ref subtreeDeleted() signal will be emitted to notify about that
	 * fact).
	 *
	 * When 0 is passed, the entire tree will be refreshed, i.e. from the
	 * root element on.
	 **/
	void refresh( FileInfo *subtree = 0 );

	/**
	 * Select some other item in this tree. Triggers the @ref
	 * selectionChanged() signal - even to the sender of this signal,
	 * i.e. take care not to cause endless signal ping-pong!
	 *
	 * Select nothing if '0' is passed.
	 **/
	void selectItem( FileInfo *newSelection );

	/**
	 * Delete a subtree.
	 **/
	void deleteSubtree( FileInfo *subtree );


    public:

	/**
	 * Return the root item of this tree. Notice that this is a pseudo
	 * root that doesn not really correspond to a filesystem object.
	 **/
	DirInfo * root() const { return _root; }

	/**
	 * Sets the root item of this tree.
	 **/
	void setRoot( DirInfo *newRoot );

	/**
	 * Return the first toplevel item of this tree or 0 if there is
	 * none. This is the logical root item.
	 **/
	FileInfo * firstToplevel() const;

	/**
	 * Return 'true' if 'item' is a toplevel item, i.e. a direct child of
	 * the root item.
	 **/
	bool isToplevel( FileInfo *item ) const;

	/**
	 * Clear all items of this tree.
	 **/
	void clear();

	/**
	 * Locate a child somewhere in the tree whose URL (i.e. complete path)
	 * matches the URL passed. Returns 0 if there is no such child.
	 *
	 * Notice: This is a very expensive operation since the entire tree is
	 * searched recursively.
	 *
	 * 'findDotEntries' specifies if locating "dot entries" (".../<Files>")
	 * is desired.
	 *
	 * This is just a convenience method that maps to
	 *    DirTree::root()->locate( url, findDotEntries )
	 **/
	FileInfo * locate( QString url, bool findDotEntries = false )
	    { return _root ? _root->locate( url, findDotEntries ) : 0; }

#if 0
	/**
	 * Notification of a finished directory read job.
	 * All read jobs are required to call this upon (successful or
	 * unsuccessful) completion.
	 **/
	void jobFinishedNotify( DirReadJob *job );
#endif

	/**
	 * Add a new directory read job to the queue.
	 **/
	void addJob( DirReadJob * job );

	/**
	 * Should directory scans cross file systems?
	 *
	 * Notice: This can only be avoided with local directories where the
	 * device number a file resides on can be obtained.
	 * Remember, that's what this QDirStat business is all about.  ;-)
	 **/
	bool crossFileSystems() const { return _crossFileSystems; }

	/**
	 * Set or unset the "cross file systems" flag.
	 **/
	void setCrossFileSystems( bool doCross ) { _crossFileSystems = doCross; }

	/**
	 * Return the tree's current selection.
	 *
	 * Even though the DirTree by itself doesn't have a visual
	 * representation, it supports the concept of one single selected
	 * item. Views can use this to transparently keep track of this single
	 * selected item, notifying the DirTree and thus other views with @ref
	 * DirTree::selectItem() . Attached views should connect to the @ref
	 * selectionChanged() signal to be notified when the selection changes.
	 *
	 * NOTE: This method returns 0 if nothing is selected.
	 **/
	FileInfo * selection() const { return _selection; }

	/**
	 * Notification that a child has been added.
	 *
	 * Directory read jobs are required to call this for each child added
	 * so the tree can emit the corresponding @ref childAdded() signal.
	 **/
	virtual void childAddedNotify( FileInfo *newChild );

	/**
	 * Notification that a child is about to be deleted.
	 *
	 * Directory read jobs are required to call this for each deleted child
	 * so the tree can emit the corresponding @ref deletingChild() signal.
	 **/
	virtual void deletingChildNotify( FileInfo *deletedChild );

	/**
	 * Notification that one or more children have been deleted.
	 *
	 * Directory read jobs are required to call this when one or more
	 * children are deleted so the tree can emit the corresponding @ref
	 * deletingChild() signal. For multiple deletions (e.g. entire
	 * subtrees) this should only happen once at the end.
	 **/
	virtual void childDeletedNotify();

        /**
	 * Send a @ref startingReading() signal.
	 **/
	void sendStartingReading();

	/**
	 * Send a @ref finished() signal.
	 **/
	void sendFinished();

	/**
	 * Send a @ref aborted() signal.
	 **/
	void sendAborted();

	/**
	 * Send a @ref progressInfo() signal to keep the user entertained while
	 * directories are being read.
	 **/
	void sendProgressInfo( const QString &infoLine );

        /**
         * Send a @ref startingReading( DirInfo * ) signal.
         **/
        void sendStartingReading( DirInfo * dir );

        /**
         * Send a @ref readJobFinished( DirInfo * ) signal.
         **/
        void sendReadJobFinished( DirInfo * dir );

	/**
	 * Send a @ref finalizeLocal() signal to give views a chance to
	 * finalize the display of this directory level - e.g. clean up dot
	 * entries, set the final "expandable" state etc.
	 **/
	void sendFinalizeLocal( DirInfo *dir );

	/**
	 * Returns 'true' if directory reading is in progress in this tree.
	 **/
	bool isBusy() { return _isBusy; }

	/**
	 * Write the complete tree to a cache file.
	 *
	 * Returns true if OK, false upon error.
	 **/
	bool writeCache( const QString & cacheFileName );

	/**
	 * Read a cache file.
	 **/
	void readCache( const QString & cacheFileName );


    signals:

	/**
	 * Emitted when a child has been added.
	 **/
	void childAdded( FileInfo *newChild );

	/**
	 * Emitted when a child is about to be deleted.
	 **/
	void deletingChild( FileInfo *deletedChild );

	/**
	 * Emitted after a child is deleted. If you are interested which child
	 * it was, better use the @ref deletingChild() signal.
	 * @ref childDeleted() is only useful to rebuild a view etc. completely.
	 * If possible, this signal is sent only once for multiple deletions -
	 * e.g., when entire subtrees are deleted.
	 **/
	void childDeleted();

	/**
	 * Emitted when reading is started.
	 **/
	void startingReading();

	/**
	 * Emitted when reading this directory tree is completely finished.
	 **/
	void finished();

	/**
	 * Emitted when reading this directory tree has been aborted.
	 **/
	void aborted();

        /**
         * Emitted when reading the specified directory is started.
         **/
        void startingReading( DirInfo * dir );

        /**
         * Emitted when reading the specified directory has been finished.
         * This is sent AFTER finalizeLocal( DirInfo * dir ).
         **/
        void readJobFinished( DirInfo * dir );

	/**
	 * Emitted when reading a directory is finished.
	 * This does _not_ mean reading all subdirectories is finished, too -
	 * only this directory level is complete!
	 *
	 * WARNING: 'dir' may be 0 if the the tree's root could not be read.
	 *
	 * Use this signal to do similar cleanups like
	 * @ref DirInfo::finalizeLocal(), e.g. cleaning up unused / undesired
	 * dot entries like in @ref DirInfo::cleanupDotEntries().
	 *
	 * Notice that the dot entry might be removed and its children reparented
	 * after this signal.
	 **/
	void finalizeLocal( DirInfo *dir );

	/**
	 * Emitted when the current selection has changed, i.e. whenever some
	 * attached view triggers the @ref selectItem() slot or when the
	 * current selection is deleted.
	 *
	 * NOTE: 'newSelection' may be 0 if nothing is selected.
	 **/
	void selectionChanged( FileInfo *newSelection );

	/**
	 * Single line progress information, emitted when the read status
	 * changes - typically when a new directory is being read. Connect to a
	 * status bar etc. to keep the user entertained.
	 **/
	void progressInfo( const QString &infoLine );


    protected slots:

	/**
	 * Read some parameters from the global @ref KConfig object.
	 **/
	void readConfig();

	/**
	 * Notification that all jobs in the job queue are finished.
	 * This will emit the finished() signal.
	 **/
	void slotFinished();


    protected:

	DirInfo *	_root;
	FileInfo *	_selection;
	DirReadJobQueue _jobQueue;
	bool		_crossFileSystems;
	bool		_enableLocalDirReader;
	bool		_isBusy;

    };	// class DirTree

}	// namespace QDirStat


#endif // ifndef DirTree_h

