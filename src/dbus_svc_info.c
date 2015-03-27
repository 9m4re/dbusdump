/*
 * dbus_svc_info.c - maintain service process id and process path
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


#include "dbusdump_config.h"
#include <string.h>
#include "sysfs.h"
#include "dbus_svc_info.h"
#include "dbus_hf_ext.h"

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif

#define G_LOG_DOMAIN "DBUS_SVC_INFO"


struct _DbusSvcInfoPrivate {
//    GBusType bus_type;
//    GDBusConnection *dbus_connection;
    GDBusProxy *dbus_proxy;
    GHashTable *svc_lut;
    guint name_owner_changed_subscription_id;

GStaticMutex svc_lut_mutex;

};


enum {
//    PROP_DBUS_CONNECTION = 1
    PROP_DBUS_PROXY= 1
};

static void initable_iface_init (
    gpointer g_class,
    gpointer unused);

G_DEFINE_TYPE_WITH_CODE (DbusSvcInfo, dbus_svc_info, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init);
    )


static void
dbus_svc_info_init (DbusSvcInfo *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, DBUS_SVC_INFO_TYPE,
      DbusSvcInfoPrivate);
}

static void dbus_svc_info_get_property (
    GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  DbusSvcInfo *self = DBUS_SVC_INFO (object);
  DbusSvcInfoPrivate *priv = self->priv;

  switch (property_id)
    {
//      case PROP_DBUS_CONNECTION:
//        g_value_set_object (value, priv->dbus_connection);
//        break;
      case PROP_DBUS_PROXY:
          g_value_set_object (value, priv->dbus_proxy);
          break;



      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void dbus_svc_info_set_property (
    GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
    DbusSvcInfo *self = DBUS_SVC_INFO (object);
    DbusSvcInfoPrivate *priv = self->priv;

  switch (property_id)
    {
//      case PROP_DBUS_CONNECTION:
//        priv->dbus_connection = g_value_get_object(value);
//        break;

      case PROP_DBUS_PROXY:
          priv->dbus_proxy= g_value_get_object(value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void dbus_svc_info_dispose (GObject *object)
{
    DbusSvcInfo *self = DBUS_SVC_INFO (object);
    DbusSvcInfoPrivate *priv = self->priv;
    GObjectClass *parent_class = dbus_svc_info_parent_class;
    DbusSvcInfoItem *value;
    GHashTableIter iter;


    if (parent_class->dispose != NULL)
        parent_class->dispose (object);

    /* Make sure we're all closed up. */
    dbus_svc_info_stop (self);

    /* clean up hash table */
    g_static_mutex_lock(&priv->svc_lut_mutex);

    g_hash_table_iter_init (&iter, priv->svc_lut);
    while (g_hash_table_iter_next (&iter, NULL, (void *) &value))
    {
        if (NULL != value)
        {
            g_free(value);
        }
    }

    g_hash_table_destroy(priv->svc_lut);
    priv->svc_lut = NULL;
    g_static_mutex_unlock(&priv->svc_lut_mutex);


//    g_object_unref(priv->dbus_connection)

    /* TODO: dispose mutex, dispose hashtable, dispose memory */

}

static void dbus_svc_info_finalize (GObject *object)
{
    DbusSvcInfo *self = DBUS_SVC_INFO (object);
    DbusSvcInfoPrivate *priv = self->priv;
    GObjectClass *parent_class = dbus_svc_info_parent_class;


    if (parent_class->finalize != NULL)
        parent_class->finalize (object);

    g_object_unref(priv->dbus_proxy);
}

static void
dbus_svc_info_class_init (DbusSvcInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;


  object_class->get_property = dbus_svc_info_get_property;
  object_class->set_property = dbus_svc_info_set_property;
  object_class->dispose = dbus_svc_info_dispose;
  object_class->finalize = dbus_svc_info_finalize;

  g_type_class_add_private (klass, sizeof (DbusSvcInfoPrivate));

#define THRICE(x) x, x, x

  param_spec = g_param_spec_object(THRICE ("dbus-proxy"),
          G_TYPE_DBUS_PROXY,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_DBUS_PROXY, param_spec);

}

static guint32 dbus_svc_info_get_pid(GDBusProxy *dbus_proxy, const char *svc_name_owner, GError **error)
{
    GVariant *ret;
    guint32 owner_pid = 0 ;

    g_assert (G_IS_DBUS_PROXY (dbus_proxy));

    ret = g_dbus_proxy_call_sync(dbus_proxy,
                    "GetConnectionUnixProcessID",
                    g_variant_new("(s)", svc_name_owner),
                    G_DBUS_CALL_FLAGS_NONE,
                    -1,
                    NULL,
                    error);


    if (ret == NULL)
      {
        g_prefix_error (error, "GetConnectionUnixProcessID(%s) return null", svc_name_owner);
        g_debug("GetConnectionUnixProcessID(%s) return null", svc_name_owner);
        owner_pid = (guint32) -1;
      }
    else
    {
        g_variant_get(ret, "(u)", &owner_pid);
        g_debug("svc name owner %s with pid %d\n", svc_name_owner, owner_pid);
        g_variant_unref(ret);
    }

    return owner_pid;
}

static gboolean dbus_svc_info_get_item_by_name(DbusSvcInfo *self, const char *svc_name, DbusSvcInfoItem *svc_info_item)
{
    DbusSvcInfoPrivate *priv = self->priv;
    DbusSvcInfoItem *svc_info_item_tmp;
    gboolean ret = FALSE;
    
    g_static_mutex_lock(&priv->svc_lut_mutex);
    svc_info_item_tmp = g_hash_table_lookup(priv->svc_lut, svc_name);
    if (NULL != svc_info_item_tmp)
    {
        svc_info_item->pid = svc_info_item_tmp->pid;
        strncpy((char *)&svc_info_item->cmdline,
                svc_info_item_tmp->cmdline,
                sizeof(svc_info_item_tmp->cmdline));

        g_debug ("dbus_svc_info_get_item_by_name svcname %s, pid %d, path='%s'",
                svc_name,
                svc_info_item->pid,
                svc_info_item->cmdline);
        ret = TRUE;
    }
    else
    {
        g_debug ("not found item for svc name %s", svc_name);
        ret = FALSE;
    }
    g_static_mutex_unlock(&priv->svc_lut_mutex);

    return ret;
}

static void dbus_svc_info_cmdline_by_pid(
        const guint32 pid,
        gchar *cmdline,
        guint32 cmdline_max_len,
        GError **error)
{
    gchar proc_path[64] = "";
    int ret;
    int i;


    g_snprintf(proc_path, sizeof(proc_path), "/proc/%u/cmdline", pid);
    ret = sysfs_read(proc_path, cmdline, cmdline_max_len);
    if (ret < 0)
    {
        g_prefix_error(error, "error reading process name from %s: %d",
            proc_path, ret);

        strncpy(cmdline, "UNKNOWN", cmdline_max_len);
    }

    /* each param are separated by \0, merge all params to single string */
    for (i=0; i < ret - 2; i++)
    {
        if ('\0' == cmdline[i])
            cmdline[i] = ' ';

        /* cmdline[ret-1] is \0 */
    }
}


static
void dbus_svc_info_svc_lut_delete_item(DbusSvcInfo *self,const char *svc_name  /*, GError **error*/ )
{
    DbusSvcInfoItem *svc_info_item_temp;
    DbusSvcInfoPrivate *priv = self->priv;
    GHashTable *svc_lut = priv->svc_lut;


    g_static_mutex_lock(&priv->svc_lut_mutex);

    svc_info_item_temp = g_hash_table_lookup(svc_lut, svc_name);
    g_hash_table_remove(svc_lut, svc_name);
 
   g_static_mutex_unlock(&priv->svc_lut_mutex);

    if (NULL != svc_info_item_temp)
    {
      g_debug("Found old item for svc name %s, free it", svc_name);
      g_free(svc_info_item_temp);
    }

    return;
}

static
void dbus_svc_info_svc_lut_update_item(DbusSvcInfo *self, const char *svc_name, GError **error)
{
    DbusSvcInfoItem *svc_info_item;
    DbusSvcInfoPrivate *priv = self->priv;
    GHashTable *svc_lut = priv->svc_lut;
    GDBusProxy *bus = priv->dbus_proxy;
    guint32 pid;


    /*g_dbus_is_unique_name return false for such service name as ':1.9'*/
    /* TODO: check g_dbus_is_unique_name */
    if (
    //              !g_dbus_is_unique_name (svc_name) &&
          (0 != strcmp (svc_name, "org.freedesktop.DBus"))
          )
    {
      pid = dbus_svc_info_get_pid(bus, svc_name, error);
      if (pid != -1)
      {
          svc_info_item = (DbusSvcInfoItem *)g_malloc(sizeof(DbusSvcInfoItem));

          svc_info_item->pid = pid;

          dbus_svc_info_cmdline_by_pid(svc_info_item->pid,
                  (gchar *)&svc_info_item->cmdline,
                  sizeof(svc_info_item->cmdline),
                  error);

          strncpy((char *)&svc_info_item->svc_name,
                          svc_name,
                          sizeof(svc_info_item->svc_name));

          if ((svc_info_item->pid == -1) || (svc_info_item->pid > 70000))
                    g_warning("svc_info_item->pid=%d, svc_info_item->cmdline=%s", svc_info_item->pid,
                            svc_info_item->svc_name);

          dbus_svc_info_svc_lut_delete_item(self, svc_name);
          /*  if the key already exists in the GHashTable its current value is replaced with the new value */
          g_static_mutex_lock(&priv->svc_lut_mutex);
          g_hash_table_insert(svc_lut, svc_info_item->svc_name, svc_info_item);
          g_static_mutex_unlock(&priv->svc_lut_mutex);

          g_debug("pid=%d, cmdline='%s'", svc_info_item->pid, svc_info_item->cmdline);
      }
    }
    else
    {
      g_debug("ignore svc name '%s'", svc_name);
    }
}



static gboolean dbus_svc_info_svc_lut_refresh (DbusSvcInfo *self,
    GError **error)
{
  GVariant *ret;
  gchar **svc_names;
  DbusSvcInfoPrivate *priv = self->priv;
  GDBusProxy *bus = priv->dbus_proxy;


  g_assert (G_IS_DBUS_PROXY (bus));

  ret = g_dbus_proxy_call_sync (bus, "ListNames", NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, error);
  if (ret == NULL)
    {
      g_prefix_error (error, "Couldn't ListNames: ");
      return FALSE;
    }


  for (g_variant_get_child (ret, 0, "^a&s", &svc_names);
       *svc_names != NULL;
       svc_names++)
    {
      gchar *svc_name = *svc_names;
      g_debug("Get service name owner '%s'", svc_name);

      dbus_svc_info_svc_lut_update_item(self, svc_name, error);

    }

  g_variant_unref (ret);
  return TRUE;
}

void dbus_svc_info_dump_svc_lut(DbusSvcInfo *dbus_svc_info, GHashTable *svc_lut)
{
    char *key;
    DbusSvcInfoItem *value;
    GHashTableIter iter;

    g_static_mutex_lock(&dbus_svc_info->priv->svc_lut_mutex);
    g_hash_table_iter_init (&iter, svc_lut);
    while (g_hash_table_iter_next (&iter, (void *)&key, (void *) &value))
    {
        g_debug("'%s' \t | %d \t '%s'",  key, value->pid, value->cmdline);
    }
    g_static_mutex_unlock(&dbus_svc_info->priv->svc_lut_mutex);

    return;
}

void dbus_svc_info_inject_pid_path(DbusSvcInfo *dbus_svc_info, GDBusMessage *message, GError **error)
{
    const char *svc_name;
    GVariant *inject_value;
    DbusSvcInfoItem svc_info_item;
    gboolean is_item_exist;

    dbus_svc_info_dump_svc_lut(dbus_svc_info, dbus_svc_info->priv->svc_lut);

    /* Inject sender pid/path*/
    svc_name = g_dbus_message_get_sender (message);
    /*TODO: check g_dbus_is_unique_name */
    if ((NULL != svc_name) &&
//            !g_dbus_is_unique_name (svc_name) &&
            (0 != strcmp (svc_name, "org.freedesktop.DBus")))

    {
        is_item_exist = dbus_svc_info_get_item_by_name(dbus_svc_info, svc_name, &svc_info_item);
        if (is_item_exist)
        {
            inject_value = g_variant_new_uint32(svc_info_item.pid);
            g_dbus_message_set_header (message, DBUS_HEADER_FIELD_EXT_SENDER_PID, inject_value);

            inject_value = g_variant_new_string((char *)&svc_info_item.cmdline);
            g_dbus_message_set_header (message, DBUS_HEADER_FIELD_EXT_SENDER_CMDLINE, inject_value);
            g_debug("dbus_svc_info_inject_pid_path inject svc name='%s', pid=%d, cmdline='%s'",
                    svc_name,
                    svc_info_item.pid,
                    svc_info_item.cmdline);
        }
    }

    /* Inject sender pid/path*/
    svc_name = g_dbus_message_get_destination (message);

    if ((NULL != svc_name) &&
//                !g_dbus_is_unique_name (svc_name) &&
                (0 != strcmp (svc_name, "org.freedesktop.DBus")))
    {
        is_item_exist = dbus_svc_info_get_item_by_name(dbus_svc_info, svc_name, &svc_info_item);
        if (is_item_exist)
        {

            inject_value = g_variant_new_uint32(svc_info_item.pid);
            g_dbus_message_set_header (message, DBUS_HEADER_FIELD_EXT_DEST_PID, inject_value);

            inject_value = g_variant_new_string((char *)&svc_info_item.cmdline);
            g_dbus_message_set_header (message, DBUS_HEADER_FIELD_EXT_DEST_CMDLINE, inject_value);
            g_debug("dbus_svc_info_inject_pid_path inject svc name='%s', pid=%d, cmdline='%s'",
                            svc_name,
                            svc_info_item.pid,
                            svc_info_item.cmdline);
        }
    }

    return;
}

static void
on_name_owner_changed_signal (GDBusConnection *connection,
                       const gchar      *sender_name,
                       const gchar      *object_path,
                       const gchar      *interface_name,
                       const gchar      *signal_name,
                       GVariant         *parameters,
                       gpointer          user_data)
{
    DbusSvcInfo *self = (DbusSvcInfo*) user_data;
    DbusSvcInfoPrivate *priv = self->priv;
    const gchar *new_owner;
    const gchar *name;
    GError *error = NULL;

    g_variant_get (parameters,
          "(&s&s&s)",
          &name,
          NULL,
          &new_owner);

    g_debug("name owner changed signal '%s'  '%s'", name, new_owner);

    //  /* TODO: lock here */
    if ( 0 == strlen(new_owner) )
    {
      /* a service has been removed */
      dbus_svc_info_svc_lut_delete_item(self, name );
      dbus_svc_info_dump_svc_lut(self, priv->svc_lut);
    }
    else
    {
        /* new service introduced  */
        dbus_svc_info_svc_lut_update_item(self, name, &error );
        dbus_svc_info_dump_svc_lut(self, priv->svc_lut);
    }

//  /* TODO: lock here */


  return;
}


static gboolean
initable_init (
    GInitable *initable,
    GCancellable *cancellable,
    GError **error)
{
    DbusSvcInfo *self = DBUS_SVC_INFO (initable);
    DbusSvcInfoPrivate *priv = self->priv;
    GDBusProxy *dbus_proxy = priv->dbus_proxy;

    g_static_mutex_init(&priv->svc_lut_mutex);
    priv->svc_lut = g_hash_table_new(g_str_hash, g_str_equal);
    dbus_svc_info_svc_lut_refresh(self, error);

    priv->name_owner_changed_subscription_id =
          g_dbus_connection_signal_subscribe (g_dbus_proxy_get_connection (dbus_proxy),
                  "org.freedesktop.DBus",  /* name */
                  "org.freedesktop.DBus",  /* interface */
                  "NameOwnerChanged",      /* signal name */
                  "/org/freedesktop/DBus", /* path */
                  NULL,                    /* arg0 -> match all*/
                  G_DBUS_SIGNAL_FLAGS_NONE,
                  on_name_owner_changed_signal,
                  self,
                  NULL);

    return TRUE;

}

/* FIXME: make this async? */
void
dbus_svc_info_stop (
    DbusSvcInfo *self)
{
    DbusSvcInfoPrivate *priv = self->priv;
    GDBusProxy *dbus_proxy = priv->dbus_proxy;
    GDBusConnection *dbus_connection = g_dbus_proxy_get_connection(dbus_proxy);


    /* stop signal monitor */

    g_dbus_connection_signal_unsubscribe(dbus_connection,
            priv->name_owner_changed_subscription_id);

    return;
}

static void
initable_iface_init (
    gpointer g_class,
    gpointer unused)
{
  GInitableIface *iface = g_class;

  iface->init = initable_init;
}

DbusSvcInfo * dbus_svc_info_new (
    GDBusProxy *dbus_proxy,
    GLogFunc log_handler,
    GError **error)
{
  g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK,  log_handler, NULL);

  return g_initable_new (
      DBUS_SVC_INFO_TYPE, NULL, error,
      "dbus-proxy", dbus_proxy,
      NULL);
}

