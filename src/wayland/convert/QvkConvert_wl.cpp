/* vokoscreenNG - A desktop recorder
 * Copyright (C) 2017-2024 Volker Kohaupt
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

#include "QvkConvert_wl.h"
#include "QvkFileDialog.h"

#include <QDebug>
#include <QStandardPaths>

QvkConvert_wl::QvkConvert_wl( QvkMainWindow_wl *vkMainWindow, Ui_formMainWindow_wl *vk_ui )
{
    ui = vk_ui;
    connect( ui->toolButton_convert_mkv, SIGNAL( clicked(bool) ), this, SLOT( slot_openFileConvert(bool) ) );
}


QvkConvert_wl::~QvkConvert_wl()
{
}


void QvkConvert_wl::slot_openFileConvert(bool)
{
    QApplication::setDesktopSettingsAware( false );

    QString pathFile;
    QvkFileDialog vkFileDialog( this );
    QStringList list( { "video/x-matroska" } );
    vkFileDialog.setMimeTypeFilters( list );
    vkFileDialog.setModal( true );
    vkFileDialog.setVideoPath( QStandardPaths::writableLocation( QStandardPaths::MoviesLocation ) );
    if ( vkFileDialog.exec() == QDialog::Accepted ) {
        if ( !vkFileDialog.selectedFiles().empty() ) {
            pathFile = vkFileDialog.selectedFiles().at(0);
            ui->lineEditConvert->setText( pathFile );
        }
    }

    QApplication::setDesktopSettingsAware( true );
}
