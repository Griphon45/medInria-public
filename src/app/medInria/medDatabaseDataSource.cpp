/*=========================================================================

 medInria

 Copyright (c) INRIA 2013 - 2018. All rights reserved.
 See LICENSE.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

=========================================================================*/

#include "medDatabaseDataSource.h"

#include <medActionsToolBox.h>
#include <medApplicationContext.h>
#include <medDatabaseExporter.h>
#include <medDatabaseModel.h>
#include <medDatabaseProxyModel.h>
#include <medDataManager.h>
#include <medSourcesLoader.h>
#include <medDataHub.h>
#include <medSourceModelPresenter.h>
#include <medDataHubPresenter.h>
#include <medSourcesWidget.h>

#include <medVirtualRepresentationPresenter.h>

class medDatabaseDataSourcePrivate
{
public:
    QPointer<QWidget> mainWidget;

    medDatabaseModel *model;
    medSourcesWidget *compactView;
    QTreeView *virtualTreeView;

    medDatabaseProxyModel *proxy;
    medDatabaseProxyModel *compactProxy;

    QList<medToolBox*> toolBoxes;
    medActionsToolBox* actionsToolBox;
    medDataHubPresenter *multiSources_tree;
    int currentSource;
};

medDatabaseDataSource::medDatabaseDataSource( QWidget* parent ): medAbstractDataSource(parent), d(new medDatabaseDataSourcePrivate)
{
    d->model = new medDatabaseModel (this);
    d->proxy = new medDatabaseProxyModel(this);
    d->proxy->setSourceModel(d->model);

    d->compactProxy = new medDatabaseProxyModel(this);
    d->compactProxy->setSourceModel(d->model);
    d->multiSources_tree = new medDataHubPresenter(medDataHub::instance());
    d->currentSource = 0;
}

medDatabaseDataSource::~medDatabaseDataSource()
{
    delete d;
    d = nullptr;
}

QWidget* medDatabaseDataSource::buildSourcesBrowser()
{
    auto *widget = new QWidget();

    auto *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QStackedWidget *mainViewWidgetRes = d->multiSources_tree->buildBrowser();
    connect(this, SIGNAL(changeSource(int)), mainViewWidgetRes, SLOT(setCurrentIndex(int)));

    layout->addWidget(mainViewWidgetRes);


    widget->setLayout(layout);

    return widget;
}

QWidget* medDatabaseDataSource::buildSourcesTreeViewList()
{
    QWidget *compactViewWidgetRes = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout();

    auto filterLabel = new QLabel("Filter ");
    auto filterLineEdit = new QLineEdit();

    medDataManager::instance()->setIndexV2Handler([](medDataIndex const & dataIndex) -> medAbstractData* {return medDataHub::instance()->getData(dataIndex); },
                                                  [](medAbstractData & data, bool originSrc) -> QUuid {return medDataHub::instance()->writeResultsHackV3(data, originSrc); },
                                                  [](QString const & path, QUuid uuid) -> void {return medDataHub::instance()->loadDataFromLocalFileSystem(path, uuid); }
                                                  );//TODO Remove ok c'est le truc le moins classe du monde (Part3)

    d->compactView = d->multiSources_tree->buildTree();

    connect(filterLineEdit, SIGNAL(textChanged(const QString &)), d->multiSources_tree, SIGNAL(filterProxy(const QString &)));

    d->compactView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    connect(d->compactView, SIGNAL(openOnDoubleClick(medDataIndex)), this, SIGNAL(openOnDoubleClick(medDataIndex)));

    //d->compactPreview = new medDatabasePreview(compactViewWidgetRes);

    QVBoxLayout *filterLayout = new QVBoxLayout;
    filterLayout->addWidget(filterLabel, 0);
    filterLayout->addWidget(filterLineEdit, 1);
    
    d->virtualTreeView = medVirtualRepresentationPresenter(medApplicationContext::instance()->getVirtualRepresentation()).buildTree();

    layout->addWidget(d->virtualTreeView, 0);
    layout->addLayout(filterLayout, 1);
    layout->addWidget(d->compactView, 2);
    //layout->addWidget(d->compactPreview);

    compactViewWidgetRes->setLayout(layout);

    return compactViewWidgetRes;
}

QWidget* medDatabaseDataSource::buildSourceListSelectorWidget()
{
    auto *widget = new QWidget();

    auto *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QListWidget *listSources = d->multiSources_tree->buildSourceList();
    connect(listSources, SIGNAL(currentRowChanged(int)), this, SLOT(currentSourceChanged(int)));

    layout->addWidget(listSources);

    widget->setLayout(layout);

    return widget;
}

void medDatabaseDataSource::currentSourceChanged(int current)
{
    d->currentSource = current;
    emit changeSource(current);
}

QString medDatabaseDataSource::tabName()
{
    return tr("Data Sources");
}

QList<medToolBox*> medDatabaseDataSource::getToolBoxes()
{
    if(d->toolBoxes.isEmpty())
    {        
        auto * pToolBox = new medToolBox();
        auto * pFiltersStackBySource = d->multiSources_tree->buildFilters();
        connect(this, SIGNAL(changeSource(int)), pFiltersStackBySource, SLOT(setCurrentIndex(int)));
        pToolBox->addWidget(pFiltersStackBySource);
        d->toolBoxes.push_back(pToolBox);
    }
    return d->toolBoxes;
}

void medDatabaseDataSource::onFilter( const QString &text, int column )
{
    // adding or overriding filter on column
    d->proxy->setFilterRegExpWithColumn(QRegExp(text, Qt::CaseInsensitive, QRegExp::Wildcard), column);
}

void medDatabaseDataSource::compactFilter( const QString &text, int column )
{
    // adding or overriding filter on column
    d->compactProxy->setFilterRegExpWithColumn(QRegExp(text, Qt::CaseInsensitive, QRegExp::Wildcard), column);
}

QWidget * medDatabaseDataSource::mainViewWidget()
{
    return buildSourcesBrowser();
}

QWidget * medDatabaseDataSource::sourceSelectorWidget()
{
    return buildSourceListSelectorWidget();
}

QString medDatabaseDataSource::description() const
{
    return QString();
}
