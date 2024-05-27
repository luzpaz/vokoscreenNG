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

#include "QvkLevelMeter.h"
#include "global.h"

#include <QDebug>
#include <string.h>
#include <math.h>


#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <gst/gst.h>


QvkLevelMeter::QvkLevelMeter()
{
}

QvkLevelMeter::~QvkLevelMeter()
{
}


static gboolean message_handler( GstBus * bus, GstMessage * message, gpointer data )
{
    Q_UNUSED(bus)

    if ( message->type == GST_MESSAGE_ELEMENT ) {
        const GstStructure *s = gst_message_get_structure( message );
        const gchar *name = gst_structure_get_name( s );

        if ( strcmp( name, "level" ) == 0 ) {
            gint channels;
            gdouble rms_dB;
            gdouble rms;
            const GValue *array_val;
            const GValue *value;
            GValueArray *rms_arr;

            // the values are packed into GValueArrays with the value per channel
            array_val = gst_structure_get_value( s, "rms" );
            rms_arr = (GValueArray *) g_value_get_boxed( array_val );

            // we can get the number of channels as the length of any of the value arrays
            channels = rms_arr->n_values;

            for ( gint i = 0; i < channels; ++i ) {
                value = g_value_array_get_nth( rms_arr, i );
                rms_dB = g_value_get_double( value );

                // converting from dB to normal gives us a value between 0.0 and 1.0
                rms = pow( 10, rms_dB / 20 );

                qint64 index = (qint64)data;
                if ( index == 0 ) {
                    global::lineEdit_00->setText( QString::number(rms) );
                }
                if ( index == 1 ) {
                    global::lineEdit_01->setText( QString::number(rms) );
                }
                if ( index == 2 ) {
                    global::lineEdit_02->setText( QString::number(rms) );
                }
                if ( index == 3 ) {
                    global::lineEdit_03->setText( QString::number(rms) );
                }
            }
        }
    }

    return TRUE;
}

void QvkLevelMeter::start( QString device, QString index )
{
    GstElement *audiotestsrc, *audioconvert, *level, *fakesink;
    GstElement *pipeline;
    GstCaps *caps;
    GstBus *bus;

    caps = gst_caps_from_string( "audio/x-raw,channels=2" );

    pipeline = gst_pipeline_new( NULL );
    g_assert (pipeline);
    audiotestsrc = gst_element_factory_make( "pulsesrc", NULL );
    g_assert (audiotestsrc);
    audioconvert = gst_element_factory_make( "audioconvert", NULL );
    g_assert (audioconvert);
    level = gst_element_factory_make( "level", NULL );
    g_assert (level);
    fakesink = gst_element_factory_make( "fakesink", NULL );
    g_assert (fakesink);

    gst_bin_add_many( GST_BIN( pipeline ), audiotestsrc, audioconvert, level, fakesink, NULL );
    if ( !gst_element_link( audiotestsrc, audioconvert ) ) {
        g_error( "Failed to link audiotestsrc and audioconvert" );
    }
    if (!gst_element_link_filtered( audioconvert, level, caps ) ) {
        g_error( "Failed to link audioconvert and level" );
    }
    if ( !gst_element_link( level, fakesink ) ) {
        g_error( "Failed to link level and fakesink" );
    }

    g_object_set( G_OBJECT( audiotestsrc ), "device", device.toUtf8().constData(), NULL );

    // make sure we'll get messages
    g_object_set( G_OBJECT( level ), "post-messages", TRUE, NULL );
    // run synced and not as fast as we can
    g_object_set( G_OBJECT( fakesink ), "sync", TRUE, NULL );

    bus = gst_element_get_bus (pipeline);

    gint64 msg = index.toInt();
    gst_bus_set_sync_handler( bus, (GstBusSyncHandler)message_handler, (gpointer)msg, NULL );

    gst_element_set_state( pipeline, GST_STATE_PLAYING );
}

