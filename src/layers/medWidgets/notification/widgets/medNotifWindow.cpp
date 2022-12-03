/*
 * medInria
 * Copyright (c) INRIA 2013. All rights reserved.
 * 
 * medInria is under BSD-2-Clause license. See LICENSE.txt for details in the root of the sources or:
 * https://github.com/medInria/medInria-public/blob/master/LICENSE.txt
 * 
 * This software is distributed WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "medNotifWindow.h"

#include <medNotifSys.h>
#include <medNotif.h>

#include <medNotifWidget.h>

#include <algorithm>

#include <QPropertyAnimation>
#include <QPauseAnimation>
#include <QSequentialAnimationGroup>
#include <QMainWindow>
#include <QStatusBar>

#include <QPushButton>
#include <QGraphicsOpacityEffect>

medNotificationPaneWidget::medNotificationPaneWidget(medNotifSys * pi_pNotifSys, QMainWindow * pi_parent) : QWidget(pi_parent)
{
    m_notificationsListWidget = new QListWidget(this);

    m_notificationsListWidget->setAcceptDrops(false);
    m_notificationsListWidget->setAutoScroll(true);
    m_notificationsListWidget->setResizeMode(QListView::Adjust);
    m_notificationsListWidget->setMinimumWidth(400);
    m_notificationsListWidget->setStyleSheet("QListWidget::item:selected {background:gray;}");
    m_animation = new QPropertyAnimation(this, "geometry", this);

    connect(pi_pNotifSys, &medNotifSys::notification, this, &medNotificationPaneWidget::addNotification);
    connect(pi_pNotifSys, &medNotifSys::removed, this, &medNotificationPaneWidget::removeNotification);
    setParent(pi_parent);
    showPane(false);

    m_geoAnimation = nullptr;
}

void medNotificationPaneWidget::notifWidgetAskDeletion(medUsrNotif notif)
{
    medNotifSys::unregisterNotif(notif);
}

void medNotificationPaneWidget::setParent(QMainWindow * pi_parent)
{
    QWidget::setParent(pi_parent);
    m_parent = pi_parent;
    if (m_parent)
    {
        m_winSize = m_parent->size();
    }
}

void medNotificationPaneWidget::removeNotification(medUsrNotif notif)
{
    auto *pItem = m_notificationToItem.take(notif.get());
    m_notificationsListWidget->removeItemWidget(pItem);
    delete pItem;
}

void medNotificationPaneWidget::addNotification(medUsrNotif notif)
{
    QListWidgetItem *widgetItem = new QListWidgetItem();
    medNotifWidget *newNotifiWidget = new medNotifWidget(notif, this);
    m_notificationsListWidget->addItem(widgetItem);
    m_notificationsListWidget->setItemWidget(widgetItem, newNotifiWidget);
    m_notificationToItem[notif.get()] = widgetItem;

    // Initial size of item
    QSize initialSize = newNotifiWidget->size();
    widgetItem->setSizeHint(initialSize);

    m_notificationsListWidget->setSpacing(2);



    //medNotifWidget *newNotifiWidgetPopup = new medNotifWidget(notif, nullptr);
    //
    //newNotifiWidgetPopup->setFixedSize(QSize(600, 300));
    //newNotifiWidgetPopup->move(QPoint(10, 10));
    //
    //newNotifiWidgetPopup->setParent(m_parent);
    //newNotifiWidgetPopup->show();
    //
    //QAnimationGroup * animationGroup = new QSequentialAnimationGroup(newNotifiWidgetPopup);
    //
    //
    //m_geoAnimation = new QPropertyAnimation(newNotifiWidgetPopup, "pos");
    //QPoint point_A(10 - 600, 10);
    //QPoint point_B(10 , 10);
    //m_geoAnimation->setDuration(3000);
    //m_geoAnimation->setStartValue(point_A);
    //m_geoAnimation->setEndValue(point_B);
    //
    //QPauseAnimation *pauseAnimation = new QPauseAnimation(3000, this);
    //
    //int duration_ms = 3000;
    //QGraphicsOpacityEffect * show_effect = new QGraphicsOpacityEffect(newNotifiWidgetPopup);
    //show_effect->setOpacity(1);
    //m_alphaAnimation = new QPropertyAnimation(show_effect, "opacity");
    //newNotifiWidgetPopup->setGraphicsEffect(show_effect);
    //m_alphaAnimation->setStartValue(1);
    //m_alphaAnimation->setEndValue(0);
    //m_alphaAnimation->setDuration(duration_ms);
    //
    //
    //animationGroup->addAnimation(m_geoAnimation);
    //animationGroup->addAnimation(pauseAnimation);
    //animationGroup->addAnimation(m_alphaAnimation);
    //
    //animationGroup->start();
}

void medNotificationPaneWidget::showPane(bool show)
{
    auto paneWidth = std::max(300, m_winSize.width() / 4);
    int statusBarHeight = 0;
    if (m_parent)
    {
        statusBarHeight = m_parent->statusBar()->height();
    }
    QRect rect_A(m_winSize.width(), 0, 0, m_winSize.height() - statusBarHeight);
    QRect rect_B(m_winSize.width() - paneWidth, 0, paneWidth, m_winSize.height() - statusBarHeight);
    if (show)
    {
        m_animation->setStartValue(rect_A);
        m_animation->setEndValue(rect_B);
    }
    else
    {
        m_animation->setStartValue(rect_B);
        m_animation->setEndValue(rect_A);
    }
    m_animation->start();
}

void medNotificationPaneWidget::windowGeometryUpdate(QRect const & geo)
{
    m_winSize = geo.size();
    if (width())
    {
        int statusBarHeight = 0;
        if (m_parent)
        {
            statusBarHeight = m_parent->statusBar()->height();
        }
        auto paneWidth = std::max(300, m_winSize.width() / 4);
        QRect rect_B(m_winSize.width() - paneWidth, 0, paneWidth, m_winSize.height() - statusBarHeight);
        setGeometry(rect_B);
    }
}

void medNotificationPaneWidget::showAndHigligth(medUsrNotif notif)
{
    if (m_notificationToItem.contains(notif.get()))
    {
        showPane(true);
        m_notificationsListWidget->setCurrentItem(m_notificationToItem[notif.get()]);
    }
}

void medNotificationPaneWidget::swithVisibility()
{
    showPane(!(bool)width());
}

