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

    global::textBrowserLog = new QTextBrowser;
    QvkLogController *vklogController = new QvkLogController;

    QString help;
    help.append( "\n" );
    help.append( "Usage: " + global::name + " [Option] [File or URL]" + "\n" );
    help.append( "\n" );
    help.append( "Options: \n" );
    help.append( "  -h or --help        Print this message\n" );
    help.append( "  -v or --version     Print version \n" );
    help.append( "\n" );
    help.append( "File or Url:\n" );
    help.append( "  Play a video\n" );
    help.append( "    Example file:\n" );
    help.append( "      vokoscreenNG /path/video\n" );
    help.append( "\n" );
    help.append( "    Example URL:\n");
    help.append( "      vokoscreenNG http://www.rapconverter.com/SampleDownload/Sample320.mp4\n");
    help.append( "      vokoscreenNG http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4" );
    help.append( "\n" );

    QStringList arguments = QApplication::instance()->arguments();
    if ( !arguments.empty() and ( arguments.count() == 2 ) )
    {
        QStringList arguments = QApplication::instance()->arguments();
        if ( ( arguments.at(1) == "--help" ) or
             ( arguments.at(1) == "-h"     ) )
        {
            qDebug().resetFormat().noquote() << help;
            delete vklogController;
            return 0;
        }

        if ( ( arguments.at(1) == "--version" ) or ( arguments.at(1) == "-v" ) )
        {
            qDebug().noquote() << global::name << global::version;
            delete vklogController;
            return 0;
        }

        if ( arguments.at(1) == "gst_appsrc" ) {
            goto end;
        }

        // If call from terminal and local file or remote file not exists
        QFile file( arguments.at(1) );
        if ( ( file.exists() == false ) and
             ( arguments.at(1).contains( "http://" ) == false ) and
             ( arguments.at(1).contains( "https://" ) == false ) )
        {
            qDebug().noquote() << global::nameOutput << arguments.at(1) << "file not exists";
            qDebug().resetFormat().noquote() << help;
            qDebug();
            delete vklogController;
            return 1;
        }

    }
    end:

    // Initialize GStreamer
    // https://developer.gnome.org/gstreamer/stable/gst-running.html
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
/*
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
*/
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
    delete vklogController;
    return app.exec();
}
