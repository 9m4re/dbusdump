/*
 * dbus_pcap.h - monitors a bus and dumps messages to a pcap file
 * Copyright © 2014 Lucas Hong Tran <hongtd2k@gmail.com>
 * Copyright © 2011-2012 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef DBUS_PCAP_H
#define DBUS_PCAP_H

#include <glib-object.h>
#include <gio/gio.h>

typedef struct _DbusPcapMonitor DbusPcapMonitor;
typedef struct _DbusPcapMonitorClass DbusPcapMonitorClass;
typedef struct _DbusPcapMonitorPrivate DbusPcapMonitorPrivate;


struct _DbusPcapMonitorClass {
    GObjectClass parent_class;
};

struct _DbusPcapMonitor {
    GObject parent;

    DbusPcapMonitorPrivate *priv;
};

GType dbus_pcap_monitor_get_type (void);

DbusPcapMonitor *dbus_pcap_monitor_new (
    GBusType bus_type,
    const gchar *filename,
    gboolean is_dump_stdout ,
    gboolean is_inject_dbus_ext_hdr,
    GLogFunc log_handler,
    GError **error);
void dbus_pcap_monitor_stop (
    DbusPcapMonitor *self);

/* TYPE MACROS */
#define DBUS_PCAP_MONITOR_TYPE \
  (dbus_pcap_monitor_get_type ())
#define DBUS_PCAP_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), DBUS_PCAP_MONITOR_TYPE, DbusPcapMonitor))
#define dbus_pcap_monitor_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), DBUS_PCAP_MONITOR_TYPE,\
                           DbusPcapMonitorClass))
#define DBUS_IS_PCAP_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), DBUS_PCAP_MONITOR_TYPE))
#define DBUS_IS_PCAP_MONITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), DBUS_PCAP_MONITOR_TYPE))
#define dbus_pcap_monitor_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DBUS_PCAP_MONITOR_TYPE, \
                              DbusPcapMonitorClass))

#endif /* DBUS_PCAP_H */
