/* vokoscreenNG - A desktop recorder
 * Copyright (C) 2017-2022 Volker Kohaupt
 *
 * Author:
 *      Volker Kohaupt <vkohaupt@volkoh.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * --End_License--
 */

#include <QTranslator>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QStyleFactory>
#include <QSettings>
#include <QFile>

#include "mainwindow.h"
#include "global.h"
#include "QvkLogController.h"

#ifdef Q_OS_UNIX
#include "mainWindow_wl.h"
#include "QvkWaylandRoutines.h"
#endif

#ifdef Q_OS_WIN
#include "QvkSettings.h"
#endif

#include <gst/gst.h>

int main(int argc, char *argv[])
{
    QLoggingCategory::defaultCategory()->setEnabled( QtDebugMsg, true );
    QApplication::setStyle( QStyleFactory::create( "Fusion" ) );
    QApplication app(argc, argv);

    QString help;
    help.append( "  -h or --help        Print this message\n" );
    help.append( "  -v or --version     Print version \n" );
    help.append( "\n" );
    QStringList arguments = QApplication::instance()->arguments();
    if ( !arguments.empty() and ( arguments.count() == 2 ) )
    {
        QStringList arguments = QApplication::instance()->arguments();
        help:
        if ( ( arguments.at(1) == "--help" ) or
             ( arguments.at(1) == "-h"     ) )
        {
            qDebug().noquote() << help;
            return 0;
        }

        if ( ( arguments.at(1) == "--version" ) or ( arguments.at(1) == "-v" ) )
        {
            qDebug().noquote() << global::name << global::version;
            return 0;
        } {
            arguments.clear();
            arguments.append( " ");
            arguments.append( "--help" );
            goto help;
        }
    }

    // This is for X11 and Windows. Wayland has a own logger.
    global::textBrowserLog = new QTextBrowser;
    new QvkLogController;

#ifdef Q_OS_UNIX
    QString separator;
    separator = ":";
#endif
#ifdef Q_OS_WIN
    QString separator;
    separator = ";";
#endif

    // qmake options example:
    // DEFINES+=FOR_MY_LINUX_INSTALLER
#if defined( Q_OS_WIN ) || defined( FOR_MY_LINUX_INSTALLER )

    QvkSettings vkSettings;
    QFileInfo dirPathProfile( vkSettings.getFileName() );
    QString pathProfile = dirPathProfile.absolutePath();
    QString programPath = QDir::currentPath();

    // Die GStreamer plugins werden über die GST_PLUGIN_PATH_1_0 variable gefunden
    QString pluginPath;
    pluginPath.append( programPath );
    pluginPath.append( separator );
    pluginPath.append( programPath + "/gstreamer/gstreamer_lib" );
    qputenv( "GST_PLUGIN_PATH_1_0", pluginPath.toUtf8() );

    // Der openh264 Codec wird über die PATH variable gefunden
    QString pathPath;
    pathPath.append( programPath );
    pathPath.append( separator );
    pathPath.append( programPath + "/gstreamer/gstreamer_bin" );
    pathPath.append( separator );
    QFileInfo h264Profile( vkSettings.getOpenh264ProfilePathWithFilename() );
    pathPath.append( h264Profile.absolutePath() );
    qputenv( "PATH", pathPath.toUtf8() );

    QString pathRegistry;
    pathRegistry.append( pathProfile );
    pathRegistry.append( "/gstreamer.registry" );
    qputenv( "GST_REGISTRY_1_0", pathRegistry.toUtf8() );

    // Registry löschen
    QFile file;
    file.setFileName( pathRegistry );
    if ( file.exists() == true ) {
        qDebug().noquote() << global::nameOutput << "File gstreamer.registry exists";
        if ( file.remove() == true ) {
            qDebug().noquote() << global::nameOutput << "File gstreamer.registry removed successfully";
        } else {
            qDebug().noquote() << global::nameOutput << "Unable to remove gstreamer.registry file";
        }
    } else {
        qDebug().noquote() << global::nameOutput << "File gstreamer.registry not exists";
    }
#endif

    // Gstreamer debug begin
    // Write Gstreamer debug level in a file
    QvkSettings vkSettingsGstDebug;
    QFileInfo fileInfo( vkSettingsGstDebug.getFileName() );
    QString pathAndFilename = fileInfo.absoluteFilePath();
    //qDebug() << "pathAndFilename:" << pathAndFilename;

    QString pathToProfile = fileInfo.absolutePath();
    //qDebug() << "pathProfile" << pathToProfile;

    QSettings setingsGstDebug( pathAndFilename, QSettings::IniFormat );
    QString debugLevel = setingsGstDebug.value( "sliderGstDebugLevel", "0" ).toString();
    //qDebug() << "debugLevel:" << debugLevel;

    if ( debugLevel.contains( "0" ) == false ) {
        qputenv( "GST_DEBUG", debugLevel.toUtf8() );
        QString debugPathProfile;
        debugPathProfile.append( pathToProfile );
        debugPathProfile.append( "/GST-Debuglevel-" + debugLevel + ".txt" );
        qputenv( "GST_DEBUG_FILE", debugPathProfile.toUtf8() );
    }
    // Gstreamer debug end

    gst_init (&argc, &argv);

#ifdef Q_OS_UNIX
    if ( QvkWaylandRoutines::is_Wayland_Display_Available() == false ) {
        QvkMainWindow *w = new QvkMainWindow;
        w->show();
    } else {
        QvkMainWindow_wl *wl = new QvkMainWindow_wl;
        wl->show();
    }
#endif

#ifdef Q_OS_WIN
    QvkMainWindow *w = new QvkMainWindow;
    w->show();
#endif
    return app.exec();
}
