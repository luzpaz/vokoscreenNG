/*
 * Copyright © 2016 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 */

#include "portal_wl.h"
#include "global.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusUnixFileDescriptor>

#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusObjectPath>
#include <QVariantMap>
#include <QLatin1String>
#include <QDebug>

Q_DECLARE_METATYPE(Portal_wl::Stream);
Q_DECLARE_METATYPE(Portal_wl::Streams);

const QDBusArgument &operator >> (const QDBusArgument &arg, Portal_wl::Stream &stream)
{
    arg.beginStructure();
    arg >> stream.node_id;

    arg.beginMap();
    while (!arg.atEnd()) {
        QString key;
        QVariant map;
        arg.beginMapEntry();
        arg >> key >> map;
        arg.endMapEntry();
        stream.map.insert(key, map);
    }
    arg.endMap();
    arg.endStructure();

    return arg;
}


Portal_wl::Portal_wl() : m_sessionTokenCounter(0), m_requestTokenCounter(0)
{
}


Portal_wl::~Portal_wl()
{
}


void Portal_wl::requestScreenSharing( int value, int mouseOnOff )
{
    Selection_Screen_Window_Area = value;
    record_mouse_onOff = mouseOnOff;

    if(!QDBusConnection::sessionBus().isConnected()) {
        qCritical() << "Cannot connect to the D-Bus session bus!";
    }

    QDBusMessage message = QDBusMessage::createMethodCall( QLatin1StringView( "org.freedesktop.portal.Desktop" ),
                                                           QLatin1StringView( "/org/freedesktop/portal/desktop" ),
                                                           QLatin1StringView( "org.freedesktop.portal.ScreenCast" ),
                                                           QLatin1StringView( "CreateSession" ) );

    message << QVariantMap { { QLatin1StringView("session_handle_token"), getSessionToken() }, { QLatin1StringView( "handle_token" ), getRequestToken() } };

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall( message );
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher( pendingCall);
//    connect( watcher, &QDBusPendingCallWatcher::finished, this, [=] (QDBusPendingCallWatcher *watcher )
    connect( watcher, &QDBusPendingCallWatcher::finished, this, [=]()
    {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if ( reply.isError() ) {
            qWarning() << "Couldn't get reply";
            qWarning() << "Error: " << reply.error().message();
        } else {
            qDebug().noquote() << global::nameOutput << "1111111111 Begin create portal session";

            bool bo = QDBusConnection::sessionBus().connect(QLatin1StringView("org.freedesktop.portal.Desktop"),
                                                            reply.value().path(),
                                                            QLatin1StringView( "org.freedesktop.portal.Request" ),
                                                            QLatin1StringView( "Response" ),
                                                            this,
                                                            SLOT( slot_gotCreateSessionResponse(uint,QVariantMap) ) );
            qDebug().noquote() << global::nameOutput << "2222222222 QDBusConnection::sessionBus().connect: " << bo;
        }
    });
}


void Portal_wl::slot_gotCreateSessionResponse( uint response, const QVariantMap &results )
{
    qDebug().noquote() << global::nameOutput << "3333333333 slot_gotCreateSessionResponse --Begin--";

    if ( response != 0 ) {
        qWarning() << "Failed to create session: " << response;
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(QLatin1StringView("org.freedesktop.portal.Desktop"),
                                                          QLatin1StringView("/org/freedesktop/portal/desktop"),
                                                          QLatin1StringView("org.freedesktop.portal.ScreenCast"),
                                                          QLatin1StringView("SelectSources"));

    m_session = results.value(QLatin1StringView("session_handle")).toString();

    message << QVariant::fromValue(QDBusObjectPath(m_session))
            << QVariantMap { { QLatin1StringView("multiple"), false},
                             { QLatin1StringView("types"), (uint)Selection_Screen_Window_Area },
                             { QLatin1StringView("cursor_mode"), (uint)record_mouse_onOff },
                             { QLatin1StringView("handle_token"), getRequestToken() } };
    qDebug().noquote() << global::nameOutput << "4444444444 slot_gotCreateSessionResponse():" << message;

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
//    bool bo = connect(watcher, &QDBusPendingCallWatcher::finished, this, [=] (QDBusPendingCallWatcher *watcher)
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [=]()
    {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            qDebug() << "Couldn't get reply";
            qDebug() << "Error: " << reply.error().message();
        } else {
            qDebug().noquote() << global::nameOutput << "5555555555 slot_gotCreateSessionResponse() into else" << "reply.value().path() :" << reply.value().path();
            bool bo = QDBusConnection::sessionBus().connect(QLatin1StringView("org.freedesktop.portal.Desktop"),
                                                            reply.value().path(),
                                                            QLatin1StringView("org.freedesktop.portal.Request"),
                                                            QLatin1StringView("Response"),
                                                            this,
                                                            SLOT( slot_gotSelectSourcesResponse(uint,QVariantMap)));
            qDebug() << "8888888888 test test" << bo; // Hahaha, ist true und geht nicht
        }
    });
}


void Portal_wl::slot_gotSelectSourcesResponse(uint response, const QVariantMap &results)
{
    Q_UNUSED(results);

    qDebug().noquote() << global::nameOutput << "9999999999 Got response from portal SelectSources";

    if (response != 0) {
        qWarning() << "Failed to select sources: " << response;
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(QLatin1StringView("org.freedesktop.portal.Desktop"),
                                                          QLatin1StringView("/org/freedesktop/portal/desktop"),
                                                          QLatin1StringView("org.freedesktop.portal.ScreenCast"),
                                                          QLatin1StringView("Start"));

    message << QVariant::fromValue(QDBusObjectPath(m_session))
            << QString() // parent_window
            << QVariantMap { { QLatin1StringView("handle_token"), getRequestToken() } };

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
//    connect(watcher, &QDBusPendingCallWatcher::finished, this, [=] (QDBusPendingCallWatcher *watcher)
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [=]()
    {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            qWarning() << "Couldn't get reply";
            qWarning() << "Error: " << reply.error().message();
        } else {
            QDBusConnection::sessionBus().connect(QLatin1StringView("org.freedesktop.portal.Desktop"),
                                                  reply.value().path(),
                                                  QLatin1StringView("org.freedesktop.portal.Request"),
                                                  QLatin1StringView("Response"),
                                                  this,
                                                  SLOT(slot_gotStartResponse(uint,QVariantMap)));
        }
    });
}


void Portal_wl::slot_gotStartResponse(uint response, const QVariantMap &results)
{
    Q_UNUSED(results);

    qDebug().noquote() << global::nameOutput << "Got response from portal Start";

    if ( response != 0 ) {
        // The system Desktop dialog was canceled
        qDebug().noquote() << global::nameOutput << "Failed to start or cancel dialog: " << response;
        emit signal_portal_cancel( response );
        return;
    }

    Streams streams = qdbus_cast<Streams>(results.value(QLatin1StringView("streams")));
    Stream stream = streams.last();

    QDBusMessage message = QDBusMessage::createMethodCall(QLatin1StringView("org.freedesktop.portal.Desktop"),
                                                          QLatin1StringView("/org/freedesktop/portal/desktop"),
                                                          QLatin1StringView("org.freedesktop.portal.ScreenCast"),
                                                          QLatin1StringView("OpenPipeWireRemote"));

    message << QVariant::fromValue(QDBusObjectPath(m_session)) << QVariantMap();

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    pendingCall.waitForFinished();
    QDBusPendingReply<QDBusUnixFileDescriptor> reply = pendingCall.reply();
    if ( reply.isError() ) {
        qWarning() << "Failed to get fd for node_id";
    }

    QString vk_fd = QString::number( reply.value().fileDescriptor() );
    QString vk_path = QString::number( stream.node_id );

    emit signal_portal_fd_path( vk_fd, vk_path );
}


QString Portal_wl::getSessionToken()
{
    m_sessionTokenCounter += 1;
    return QString("u%1").arg(m_sessionTokenCounter);
}

QString Portal_wl::getRequestToken()
{
    m_requestTokenCounter += 1;
    return QString("u%1").arg(m_requestTokenCounter);
}
