/* $Id$ */
/** @file
 * VBox Qt GUI - UIVisoHostBrowser class declaration.
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_medium_viso_UIVisoHostBrowser_h
#define FEQT_INCLUDED_SRC_medium_viso_UIVisoHostBrowser_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
# include <QModelIndex>

/* GUI includes: */
#include "UIVisoBrowserBase.h"

/* Forward declarations: */
class QItemSelection;
class UIVisoHostBrowserModel;

class SHARED_LIBRARY_STUFF UIVisoHostBrowser : public UIVisoBrowserBase
{
    Q_OBJECT;

signals:

    void sigAddObjectsToViso(QStringList pathList);

public:

    UIVisoHostBrowser(QWidget *pParent = 0, QMenu *pMenu = 0);
    ~UIVisoHostBrowser();
    virtual void showHideHiddenObjects(bool bShow) /* override */;

protected:

    void retranslateUi();
    virtual void tableViewItemDoubleClick(const QModelIndex &index) /* override */;
    virtual void setTableRootIndex(QModelIndex index = QModelIndex()) /* override */;
    virtual void setTreeCurrentIndex(QModelIndex index = QModelIndex()) /* override */;
    virtual void treeSelectionChanged(const QModelIndex &selectedTreeIndex) /* override */;

private slots:

    void sltHandleTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void sltHandleAddAction();

private:

    void prepareObjects();
    void prepareConnections();

    /** We have two file system models (one for each item view) since we set different filter on each of these models. */
    UIVisoHostBrowserModel *m_pTreeModel;
    UIVisoHostBrowserModel *m_pTableModel;
    QAction                *m_pAddAction;
    QTableView             *m_pTableView;

};


#endif /* !FEQT_INCLUDED_SRC_medium_viso_UIVisoHostBrowser_h */
