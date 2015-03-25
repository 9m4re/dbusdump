/*
 * dbus_svc_info.h - maintain service process id and process path
 * Copyright © 2015 Lucas Hong Tran .
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

#ifndef DBUS_SVC_INFO_H
#define DBUS_SVC_INFO_H

#include <glib-object.h>
#include <gio/gio.h>

#define PROCESS_CMD_LINE_MAX_LENGTH     (255)
#define DBUS_MAXIMUM_NAME_LENGTH   255

typedef struct _DbusSvcInfo DbusSvcInfo;
typedef struct _DbusSvcInfoClass DbusSvcInfoClass;
typedef struct _DbusSvcInfoPrivate DbusSvcInfoPrivate;
typedef struct _DbusSvcInfoItem DbusSvcInfoItem;

struct _DbusSvcInfoClass {
    GObjectClass parent_class;
};

struct _DbusSvcInfo{
    GObject parent;

    DbusSvcInfoPrivate *priv;
};

GType dbus_svc_info_get_type (void);

DbusSvcInfo *dbus_svc_info_new (
        GDBusProxy *dbus_proxy,
        GError **error);

struct _DbusSvcInfoItem
{
    guint32 pid;
    gchar svc_name[DBUS_MAXIMUM_NAME_LENGTH];
    /* TODO fix me with define or gstring */
    gchar cmdline[PROCESS_CMD_LINE_MAX_LENGTH];
};

void dbus_svc_info_stop (DbusSvcInfo *self);

void dbus_svc_info_inject_pid_path(DbusSvcInfo *dbus_svc_info, GDBusMessage *message, GError **error);

/* TYPE MACROS */
#define DBUS_SVC_INFO_TYPE \
  (dbus_svc_info_get_type ())
#define DBUS_SVC_INFO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), DBUS_SVC_INFO_TYPE, DbusSvcInfo))
#define DBUS_SVC_INFO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), DBUS_SVC_INFO_TYPE,\
                           DbusSvcInfoClass))
#define DBUS_IS_SVC_INFO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), DBUS_SVC_INFO_TYPE))
#define DBUS_IS_SVC_INFO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), DBUS_SVC_INFO_TYPE))
#define DBUS_SVC_INFO_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DBUS_SVC_INFO_TYPE, \
                              DbusSvcInfoClass))

#endif /* DBUS_SVC_INFO_H */
