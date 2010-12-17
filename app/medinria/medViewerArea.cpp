/* medViewerArea.cpp --- 
 * 
 * Author: Julien Wintz
 * Copyright (C) 2008 - Julien Wintz, Inria.
 * Created: Fri Sep 18 12:43:06 2009 (+0200)
 * Version: $Id$
 * Last-Updated: Wed Nov 10 16:15:44 2010 (+0100)
 *           By: Julien Wintz
 *     Update #: 1061
 */

/* Commentary: 
 * 
 */

/* Change log:
 * 
 */

#include "medViewerArea.h"
#include "medViewerArea_p.h"
#include "medViewerToolBoxConfiguration.h"

#include "medGui/medViewContainerStack.h"
#include "medGui/medViewerToolBoxLayout.h"
#include "medViewerToolBoxPatient.h"
#include "medGui/medViewerToolBoxView.h"

#include <dtkCore/dtkAbstractViewFactory.h>
#include <dtkCore/dtkAbstractView.h>
#include <dtkCore/dtkAbstractDataFactory.h>
#include <dtkCore/dtkAbstractData.h>
#include <dtkCore/dtkAbstractDataReader.h>
#include <dtkCore/dtkGlobal.h>

#include <dtkVr/dtkVrHeadRecognizer.h>
#include <dtkVr/dtkVrGestureRecognizer.h>

#include <medCore/medDataIndex.h>
#include <medCore/medDataManager.h>
#include <medCore/medViewManager.h>
#include <medCore/medAbstractView.h>

#include <medSql/medDatabaseController.h>
#include <medSql/medDatabaseNonPersitentItem.h>
#include <medSql/medDatabaseNonPersitentController.h>
#include <medSql/medDatabaseNavigator.h>
#include <medSql/medDatabaseNavigatorController.h>

#include <medGui/medClutEditor.h>
#include <medGui/medToolBox.h>
#include <medGui/medToolBoxContainer.h>
#include <medGui/medToolBoxRegistration.h>
#include <medGui/medToolBoxDiffusion.h>
#include <medGui/medViewContainer.h>
#include <medGui/medViewContainerCustom.h>
#include <medGui/medViewContainerMulti.h>
#include <medGui/medViewContainerSingle.h>
#include <medGui/medViewPool.h>
#include <medGui/medViewerConfigurationFactory.h>

#include <QtGui>
#include <QtSql>
#include <QPropertyAnimation>
#include <QEasingCurve>

// /////////////////////////////////////////////////////////////////
// medViewerArea
// /////////////////////////////////////////////////////////////////

medViewerArea::medViewerArea(QWidget *parent) : QWidget(parent), d(new medViewerAreaPrivate)
{
    // -- Internal logic
    d->current_patient = -1;
    d->current_configuration_name = "";
    d->current_configuration = 0;
    d->current_layout = medViewerConfiguration::LeftDbRightTb;
    d->current_container = 0;
    d->current_container_preset = 0;

    d->splitter = new QSplitter(Qt::Horizontal,this);
    d->splitter->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    d->splitter->setHandleWidth(3);
    // -- User interface setup

    d->stack = new QStackedWidget(this);
    d->stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    d->toolboxPatient = new medViewerToolBoxPatient(this);
    d->toolboxPatient->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);//, QSizePolicy::Minimum);
    d->toolboxPatient->setFixedWidth(176); // 186 - 10

    
    // Setting up toolbox container

    d->toolbox_container = new medToolBoxContainer(this);
    d->toolbox_container->setOrientation(Qt::Vertical);
    d->toolbox_container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    d->toolbox_container->setMinimumWidth(320);

    // Setting up view container

    d->view_container = new QWidget(this);

    QVBoxLayout *view_container_layout = new QVBoxLayout(d->view_container);
    view_container_layout->setContentsMargins(0, 10, 0, 10);
    view_container_layout->addWidget(d->stack);


    // Setting up navigator container

    d->navigator_container = new QFrame(this);
    d->navigator_container->setObjectName("medNavigatorContainer");
    d->navigator_container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    d->navigator_container->setMinimumWidth(186);

    // Setting up navigator
    
    medDatabaseNavigatorController::instance()->setOrientation( Qt::Vertical );
    d->navigator = new medDatabaseNavigator(d->navigator_container);

    d->navigator_animation = new QPropertyAnimation (d->navigator, "geometry");
    d->navigator_animation->setDuration (500);
    d->navigator_animation->setEasingCurve (QEasingCurve::OutQuad);

    // d->navigator_container_layout = 0;
    d->navigator_container_layout = new QGridLayout(d->navigator_container);
    d->navigator_container_layout->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
    d->navigator_container_layout->setContentsMargins(0, 0, 0, 0);
    d->navigator_container_layout->setSpacing(0);
    d->navigator_container_layout->addWidget(d->toolboxPatient, 0, 0);
    d->navigator_container_layout->addWidget(d->navigator, 1, 0);

    //Set up viewer layout
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(d->splitter);
    setLayout(layout);
    d->splitter->addWidget(d->navigator_container);
    d->splitter->addWidget(d->view_container);
    d->splitter->addWidget(d->toolbox_container);

    //restore previous splitter position.
//    QSettings settings("inria","medinria");
//    if (!d->splitter->restoreState(settings.value("ViewerSplitterSizes").toByteArray()))
//    {
        //viewcontainer size
        int containerSize = QWIDGETSIZE_MAX - d->navigator->minimumWidth() - d->toolbox_container->minimumWidth();
        QList<int> sizes;
        sizes.append(d->navigator->minimumWidth());
        sizes.append(containerSize);
        sizes.append(d->toolbox_container->minimumWidth());
        d->splitter->setSizes(sizes);
//    }

    //action for transfer function
             QAction * transFunAction =
      new QAction("Toggle Tranfer Function Widget", this);
    transFunAction->setShortcut(Qt::ControlModifier + Qt::ShiftModifier +
				Qt::Key_L);
    transFunAction->setCheckable( true );
    transFunAction->setChecked( false );
    connect(transFunAction, SIGNAL(toggled(bool)),
	    this, SLOT(bringUpTransferFunction(bool)));

    this->addAction(transFunAction);
    
    connect (d->toolboxPatient, SIGNAL(patientIndexChanged(int)), this, SLOT(switchToPatient(int)));
}

medViewerArea::~medViewerArea(void)
{
    //No one wants to keep these settings at the moment, but I will persist, one day it will be there, oh yes one day...
//    QSettings settings("inria","medinria");
//    settings.setValue("ViewerSplitterSizes",
//                      d->splitter->saveState());
    delete d;

    d = NULL;
}

//! Status bar setup
/*! 
 *  Called whenever the viewer area is switched to. Add viewer area
 *  specific status widgets on the left of the status bar.
 */

void medViewerArea::setup(QStatusBar *status)
{

}

//! Status bar setdown
/*! 
 *  Called whenever the viewer area is left. Remove viewer area
 *  specific status widgets from the left of the status bar.
 */

void medViewerArea::setdw(QStatusBar *status)
{

}



//! Split the currently displayed custom container.
/*! 
 *  This slots make the connection between the layout toolbox gui and
 *  the actual custom view container.
 */

void medViewerArea::split(int rows, int cols)
{
    if (d->view_stacks.count())
        d->view_stacks.value(d->current_patient)->current()->split(rows, cols);
}

//! Open data corresponding to index \param index.
/*! 
 * 
 */

void medViewerArea::open(const medDataIndex& index)
{
    if(!((medDataIndex)index).isValid())
        return;
    
    this->switchToPatient(index.patientId());
    
    if(((medDataIndex)index).isValidForSeries()) {
        
        dtkAbstractData *data = NULL;
        dtkAbstractView *view = NULL;
        
        if(!data)
            data = medDataManager::instance()->data(index);
        
        if(!data)
            data = medDatabaseNonPersitentController::instance()->data(index);
        
        if(!data)
            data = medDatabaseController::instance()->read(index);
        
        if(!data)
            return;
        
        medDataManager::instance()->insert(index, data);
        
        if(!view)
            view = d->view_stacks.value(d->current_patient)->current()->current()->view();
        
        if(!view) {
            view = dtkAbstractViewFactory::instance()->create("v3dView");
            connect (view, SIGNAL(closed()), this, SLOT(onViewClosed()));
        }
        
        if(!view) {
            qDebug() << "Unable to create a v3dView";
            return;
        }
        
        medViewManager::instance()->insert(index, view);
        
        view->setData(data);
        view->reset(); // called in view_stacks -> setView but seems necessary with the streaming approach
        
        QMutexLocker ( &d->mutex );
        d->view_stacks.value(d->current_patient)->current()->current()->setView(view);
        d->view_stacks.value(d->current_patient)->current()->current()->setFocus(Qt::MouseFocusReason);
        
        return;
    }
    
    if(((medDataIndex)index).isValidForPatient()) {
        
        this->setupConfiguration("Visualization");
        this->switchToPatient(index.patientId());
        this->switchToContainer(1);
        
        QSqlQuery stQuery(*(medDatabaseController::instance()->database()));
        stQuery.prepare("SELECT * FROM study WHERE patient = :id");
        stQuery.bindValue(":id", index.patientId());
        if(!stQuery.exec())
            qDebug() << DTK_COLOR_FG_RED << stQuery.lastError() << DTK_NO_COLOR;
        
        while(stQuery.next()) {
            
            QSqlQuery seQuery(*(medDatabaseController::instance()->database()));
            seQuery.prepare("SELECT * FROM series WHERE study = :id");
            seQuery.bindValue(":id", stQuery.value(0));
            if(!seQuery.exec())
                qDebug() << DTK_COLOR_FG_RED << seQuery.lastError() << DTK_NO_COLOR;
            
            while(seQuery.next())
                this->open(medDataIndex(index.patientId(), stQuery.value(0).toInt(), seQuery.value(0).toInt()));
        }
        
    }
}

//! Open file on the local filesystem.
/*! 
 * 
 */

void medViewerArea::open(const QString& file)
{
    this->open(medDatabaseNonPersitentController::instance()->read(file));
}

void medViewerArea::onViewClosed(void)
{
    if (dtkAbstractView *view = dynamic_cast<dtkAbstractView*> (this->sender())) {        
        QList<medToolBox *> toolboxes = d->toolbox_container->toolBoxes();
        foreach( medToolBox *tb, toolboxes)
            tb->update(NULL);
        
        medDataIndex index = medViewManager::instance()->index( view );
        medViewManager::instance()->remove(index, view); // deletes the view
    }
}

//! Switch the view area layout to the one of patient with database index \param index.
/*! 
 * 
 */

void medViewerArea::switchToPatient(int id)
{
    if(id < 0 || d->current_patient==id)
        return;

    d->current_patient = id;

    // Setup view container

    medViewContainerStack *view_stack;

    if(!d->view_stacks.contains(d->current_patient)) {
        view_stack = new medViewContainerStack(this);
        connect(view_stack, SIGNAL(dropped(medDataIndex)), this, SLOT(open(medDataIndex)));
        connect(view_stack, SIGNAL(focused(dtkAbstractView*)), this, SLOT(onViewFocused(dtkAbstractView*)));
        d->view_stacks.insert(d->current_patient, view_stack);
        d->stack->addWidget(view_stack);
    }
    else {
        view_stack = d->view_stacks.value(d->current_patient);
    }

    if (d->current_configuration) {
        d->current_configuration->setupViewContainerStack( view_stack );
        switchToContainer (d->current_configuration->viewLayoutType());
        switchToContainerPreset (d->current_configuration->customLayoutType());
    }

    d->stack->setCurrentWidget(view_stack);

    // Setup navigator

    if (d->navigator) {
        d->navigator->onPatientClicked(d->current_patient);
        
        QRect endGeometry = d->navigator->geometry();
        QRect startGeometry = endGeometry;
        if (d->navigator->orientation()==Qt::Vertical)
            startGeometry.setY (endGeometry.y()+1000);
        else
            startGeometry.setX (endGeometry.x()+1000);
        
        d->navigator_animation->setStartValue(startGeometry);
        d->navigator_animation->setEndValue(endGeometry);
        d->navigator_animation->start();
    }

    // Setup patient toolbox
    //TODO emit a signal to the Patient Toolbox
    //emit (setPatientIndex(id));

    // Setup layout toolbox
}

//! Set stack index.
/*! 
 *  This method actually allows one to switch between the
 *  single/multi/custom modes for the currently displayed view
 *  stack. A view stack is composed of a single/custom/multi layout.
 */

void medViewerArea::switchToContainer(int index)
{
    if (d->current_container==index)
        return;
    
    d->current_container = index;
    
    if(index < 0)
        return;
        
    if (d->view_stacks.count())
      if (d->view_stacks.value(d->current_patient)) {
          d->view_stacks.value(d->current_patient)->setCurrentIndex(index);
        //this->currentContainer()->setFocus(Qt::MouseFocusReason);
     }
    
    if (d->current_configuration)
        d->current_configuration->setViewLayoutType(index);
}

//! Set custom view preset
/*! 
 *  Presets are defined in src/medGui/medViewContainerCustom.
 */

void medViewerArea::switchToContainerPreset(int index)
{
    if(index < 0)
        return;

    if (d->view_stacks.count()) {
        if (d->view_stacks.value(d->current_patient)) {
	    if(medViewContainerCustom *custom = dynamic_cast<medViewContainerCustom *>(d->view_stacks.value(d->current_patient)->custom())) {
                custom->setPreset(index);
            }
        }
    }
    
    if (d->current_configuration)
        d->current_configuration->setCustomLayoutType(index);
}

void medViewerArea::addToolBox(medToolBox *toolbox)
{
    d->toolbox_container->addToolBox(toolbox);
}

void medViewerArea::removeToolBox(medToolBox *toolbox)
{
    d->toolbox_container->removeToolBox(toolbox);
}


#include <dtkVr/dtkVrController.h>

//! View focused callback. 
/*! 
 *  This method updates the toolboxes according to the focused view.
 */

void medViewerArea::onViewFocused(dtkAbstractView *view)
{
    // set head recognizer

    static dtkVrHeadRecognizer *head_recognizer = NULL;

    if(dtkApplicationArgumentsContain(qApp, "--tracker")) {

        if(!head_recognizer) {
            head_recognizer = new dtkVrHeadRecognizer;
            head_recognizer->startConnection(QUrl(dtkApplicationArgumentsValue(qApp, "--tracker")));
        }

        if(view->property("Orientation") == "3D")
            head_recognizer->setView(view);
        else
            head_recognizer->setView(NULL);
    }

    // set gesture recognizer

    static dtkVrGestureRecognizer *gesture_recognizer = NULL;

    if(dtkApplicationArgumentsContain(qApp, "--tracker")) {

        if(!gesture_recognizer) {
            gesture_recognizer = new dtkVrGestureRecognizer;
            gesture_recognizer->startConnection(QUrl(dtkApplicationArgumentsValue(qApp, "--tracker")));
        }

        gesture_recognizer->setView(view);
        gesture_recognizer->setReceiver(static_cast<medAbstractView *>(view)->receiverWidget());
    }

    // Update toolboxes
    QList<medToolBox *> toolboxes = d->toolbox_container->toolBoxes();
    foreach( medToolBox *tb, toolboxes)
        tb->update(view);
    
    connect (view, SIGNAL(lutChanged()), this, SLOT(updateTransferFunction()));

    this->updateTransferFunction();
}

//! Returns the currently displayed stack. 
/*! 
 *  A stack is a set a view containers for a given patient.
 */

medViewContainerStack *medViewerArea::currentStack(void)
{
    return d->view_stacks.value(d->current_patient);
}

//! Returns the currently displayed container of the currently displayed stack.
/*! 
 * 
 */

medViewContainer *medViewerArea::currentContainer(void)
{
    return d->view_stacks.value(d->current_patient)->current();
}

//! Returns the currently focused child container.
/*! 
 *  Note that container are hierarchical structures. This methods
 *  returns a container that can contain a view.
 */

medViewContainer *medViewerArea::currentContainerFocused(void)
{
    return d->view_stacks.value(d->current_patient)->current()->current();
}

// view settings
/*
void medViewerArea::setupForegroundLookupTable(QString table)
{
    if(!d->view_stacks.count())
        return;

    if ( medViewPool *pool = this->currentContainer()->pool() )
        pool->setViewProperty("LookupTable", table);

    this->updateTransferFunction();
}
void medViewerArea::setupLUTPreset(QString table)
{
    if(!d->view_stacks.count())
        return;
  
    if ( medViewPool *pool = this->currentContainer()->pool() ) {
        pool->setViewProperty("Preset", table);
    }

    this->updateTransferFunction();
}
*/

void medViewerArea::bringUpTransferFunction(bool checked)
{
    if (!checked)
    {
        if (d->transFun !=NULL )
        {
            delete d->transFun ;
            d->transFun=NULL;
        }
	return;
    }
    if(!d->view_stacks.count())
        return;
  
    if ( dtkAbstractView *view = this->currentContainerFocused()->view() ) {

      d->transFun = new medClutEditor(NULL);
      d->transFun->setWindowModality( Qt::WindowModal );
      d->transFun->setWindowFlags(Qt::Tool|Qt::WindowStaysOnTopHint);

      // d->transFun->setData(static_cast<dtkAbstractData *>(view->data()));
      d->transFun->setView(dynamic_cast<medAbstractView*>(view));

      d->transFun->show();
    }
}

void medViewerArea::updateTransferFunction()
{
    dtkAbstractView * view = this->currentContainerFocused()->view();
    if ( d->transFun && view ) {
	// d->transFun->setData( static_cast<dtkAbstractData *>( view->data() ) );
	d->transFun->setView( dynamic_cast<medAbstractView *>( view ), true );
	d->transFun->update();
    }
}


void medViewerArea::setupConfiguration(QString name)
{
    if (d->current_configuration_name == name)
        return;
    
    medViewerConfiguration *conf = NULL;
    
    if (d->configurations.contains(name))
        conf = d->configurations[name];
    else {
        if (conf = medViewerConfigurationFactory::instance()->createConfiguration(name)) {
            connect(d->toolboxPatient, SIGNAL(patientIndexChanged(int)), conf, SLOT(patientChanged(int)));
            d->configurations.insert(name, conf);
        }
        else
            qDebug()<< "Configuration" << name << "couldn't be created";
    }

    if (!conf)
        return;

    d->current_configuration = conf;
    d->current_configuration_name = name;

    //clean toolboxes
    d->toolbox_container->hide();
    d->toolbox_container->clear();
    
    //setup layout
    switchToLayout (conf->layoutType());
    
    // setup layout type
    if (d->view_stacks.contains(d->current_patient)) {
        conf->setupViewContainerStack( d->view_stacks[d->current_patient] );
    }
    
    switchToContainer(conf->viewLayoutType());

    if (conf->viewLayoutType() == medViewContainer::Custom) {
        switchToContainerPreset(conf->customLayoutType());
    }

    //setup database visibility
    d->navigator_container->setVisible( conf->isDatabaseVisible() );

    // add toolboxes
    medToolBox * prevbox = 0;
    foreach (medToolBox * toolbox, conf->toolBoxes() ) {
        this->addToolBox(toolbox);
        toolbox->show();
    }
    
    //setup layout Toolbox Visibility
    conf->isLayoutToolBoxVisible()?conf->showLayoutToolBox():conf->hideLayoutToolBox();

    d->toolbox_container->setVisible( conf->areToolBoxesVisible() );

    /*
     if (d->toolbox_container->toolBoxes().count()) {
     QPropertyAnimation *animation = new QPropertyAnimation(d->toolbox_container, "geometry");
     animation->setDuration(500);
     if (d->toolbox_container->orientation()==medToolBoxContainer::Vertical)  {
     animation->setStartValue(QRect(d->toolbox_container->x(), 1000, d->toolbox_container->width(), d->toolbox_container->height()));
     animation->setEndValue(QRect(d->toolbox_container->x(), 0, d->toolbox_container->width(), d->toolbox_container->height()));
     }
     else {
     animation->setStartValue(QRect(1000, d->toolbox_container->y(), d->toolbox_container->width(), d->toolbox_container->height()));
     animation->setEndValue(QRect(0, d->toolbox_container->y(), d->toolbox_container->width(), d->toolbox_container->height()));
     }
     animation->setEasingCurve(QEasingCurve::OutQuad);
     animation->start();
	}*/

    connect(conf, SIGNAL(layoutModeChanged(int)),     this, SLOT(switchToContainer(int)));
    connect(conf, SIGNAL(layoutSplit(int,int)),       this, SLOT(split(int,int)));
    connect(conf, SIGNAL(layoutPresetClicked(int)),   this, SLOT(switchToContainerPreset(int)));
    connect(conf, SIGNAL(toolboxAdded(medToolBox*)),  this, SLOT(addToolBox(medToolBox*)));
    connect(conf, SIGNAL(toolboxRemoved(medToolBox*)),this, SLOT(removeToolBox(medToolBox*)));
}

void medViewerArea::switchToLayout (medViewerConfiguration::LayoutType layout)
{
    if (d->current_layout==layout)
        return;

    d->current_layout = layout;

    //setup orientation
    switch (layout){
        case medViewerConfiguration::TopDbBottomTb:
        case medViewerConfiguration::TopTbBottomDb:
           {
            d->splitter->setOrientation(Qt::Vertical);
	     d->navigator_container_layout->removeWidget ( d->toolboxPatient );
	     d->navigator_container_layout->removeWidget ( d->navigator );

	     d->navigator->setOrientation (Qt::Horizontal);
	     
	     d->navigator_container_layout->addWidget (d->toolboxPatient, 0, 0);
	     d->navigator_container_layout->addWidget (d->navigator, 0, 1);
	      
//	     d->layout->removeWidget ( d->navigator_container );
//	     d->layout->removeWidget ( d->view_container );
//	     d->layout->removeWidget ( d->toolbox_container );

             d->navigator_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
             d->navigator_container->setMinimumHeight(186);
             d->navigator_container->setMinimumWidth(QWIDGETSIZE_MAX);
	     
	     d->toolbox_container->setOrientation(Qt::Horizontal);
             d->toolbox_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
             d->toolbox_container->setMinimumHeight(200);
             d->toolbox_container->setMinimumWidth(QWIDGETSIZE_MAX);
           }
            break;
            
        case medViewerConfiguration::LeftDbRightTb:
        case medViewerConfiguration::LeftTbRightDb:
        default:
           {
             d->splitter->setOrientation(Qt::Horizontal);
	     d->navigator_container_layout->removeWidget ( d->toolboxPatient );
	     d->navigator_container_layout->removeWidget ( d->navigator );

	     d->navigator->setOrientation (Qt::Vertical);
	     
	     d->navigator_container_layout->addWidget (d->toolboxPatient, 0, 0);
	     d->navigator_container_layout->addWidget (d->navigator, 1, 0);

//	     d->layout->removeWidget ( d->navigator_container );
//	     d->layout->removeWidget ( d->view_container );
//	     d->layout->removeWidget ( d->toolbox_container );

             d->navigator_container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
             d->navigator_container->setMinimumWidth(186);
             d->navigator_container->setMinimumHeight(QWIDGETSIZE_MAX);
	      
	     d->toolbox_container->setOrientation(Qt::Vertical);
             d->toolbox_container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
             d->toolbox_container->setMinimumWidth(320);
             d->toolbox_container->setMinimumHeight (QWIDGETSIZE_MAX);
           }
    }


    switch (layout){
        case medViewerConfiguration::TopDbBottomTb:
        case medViewerConfiguration::LeftDbRightTb:
//	    d->layout->addWidget ( d->navigator_container, 0, 0);
//	    d->layout->addWidget ( d->view_container, 1, 0);
//	    d->layout->addWidget ( d->toolbox_container, 2, 0);
            d->splitter->insertWidget(0,d->navigator_container);
            d->splitter->insertWidget(2,d->toolbox_container);
	    break;
	    
        case medViewerConfiguration::TopTbBottomDb:
        case medViewerConfiguration::LeftTbRightDb:
        default:
//	    d->layout->addWidget ( d->toolbox_container, 0, 0);
//	    d->layout->addWidget ( d->view_container, 1, 0);
//	    d->layout->addWidget ( d->navigator_container, 2, 0);
            d->splitter->insertWidget(0,d->toolbox_container);
            d->splitter->insertWidget(2,d->navigator_container);
	    break;
	    
//	case medViewerConfiguration::LeftTbRightDb:
//	    d->layout->addWidget ( d->toolbox_container, 0, 0);
//	    d->layout->addWidget ( d->view_container, 0, 1);
//	    d->layout->addWidget ( d->navigator_container, 0, 2);
//	    break;
	  
//        case medViewerConfiguration::LeftDbRightTb:
//        default:
//	    d->layout->addWidget ( d->navigator_container, 0, 0);
//	    d->layout->addWidget ( d->view_container, 0, 1);
//	    d->layout->addWidget ( d->toolbox_container, 0, 2);
    }
}
