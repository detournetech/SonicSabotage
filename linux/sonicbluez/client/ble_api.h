#ifndef BLE_API_H
#define BLE_API_H

#include "gdbus/gdbus.h"

/* String display constants */
#define COLORED_NEW COLOR_GREEN "NEW" COLOR_OFF
#define COLORED_CHG COLOR_YELLOW "CHG" COLOR_OFF
#define COLORED_DEL COLOR_RED "DEL" COLOR_OFF

#define PROMPT_ON   COLOR_BLUE "[bluetooth]" COLOR_OFF "# "
#define PROMPT_OFF  "Waiting to connect to bluetoothd..."

extern char *auto_register_agent;// = NULL;

extern GMainLoop *main_loop;
extern DBusConnection *dbus_conn;
extern GDBusProxy *agent_manager;

struct adapter {
    GDBusProxy *proxy;
    GList *devices;
};

extern struct adapter *default_ctrl;
extern GDBusProxy *default_dev;
extern GDBusProxy *default_attr;
extern GList *ctrl_list;

void print_adapter(GDBusProxy *proxy, const char *description);
//void print_device(GDBusProxy *proxy, const char *description);
//void print_iter(const char *label, const char *name, DBusMessageIter *iter);
//void print_property(GDBusProxy *proxy, const char *name);
//void print_uuids(GDBusProxy *proxy);
gboolean device_is_child(GDBusProxy *device, GDBusProxy *master);
gboolean service_is_child(GDBusProxy *service);
struct adapter *find_parent(GDBusProxy *device);
void set_default_device(GDBusProxy *proxy, const char *attribute);
void device_added(GDBusProxy *proxy);
void adapter_added(GDBusProxy *proxy);
void proxy_added(GDBusProxy *proxy, void *user_data);
void set_default_attribute(GDBusProxy *proxy);
void device_removed(GDBusProxy *proxy);
void adapter_removed(GDBusProxy *proxy);
void proxy_removed(GDBusProxy *proxy, void *user_data);
void property_changed(GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data);
struct adapter *find_ctrl_by_address(GList *source, const char *address);
GDBusProxy *find_proxy_by_address(GList *source, const char *address);
gboolean check_default_ctrl(void);
gboolean parse_argument_on_off(const char *arg, dbus_bool_t *value);
gboolean parse_argument_agent(const char *arg, dbus_bool_t *value,
							const char **capability);
void cmd_list(const char *arg);
void cmd_show(const char *arg);
void cmd_select(const char *arg);
void cmd_devices(const char *arg);
void cmd_paired_devices(const char *arg);
gboolean is_paired(const char *arg);
void generic_callback(const DBusError *error, void *user_data);
void cmd_system_alias(const char *arg);
void cmd_reset_alias(const char *arg);
void cmd_power(const char *arg);
void cmd_agent(const char *arg);
void cmd_default_agent(const char *arg);
void start_discovery_reply(DBusMessage *message, void *user_data);
void cmd_scan(const char *arg);
struct GDBusProxy *find_device(const char *arg);
void cmd_info(const char *arg);
void pair_reply(DBusMessage *message, void *user_data);
void cmd_pair(const char *arg);
void cmd_trust(const char *arg);
void cmd_untrust(const char *arg);
void remove_device_reply(DBusMessage *message, void *user_data);
void remove_device_setup(DBusMessageIter *iter, void *user_data);
void remove_device(GDBusProxy *proxy);
void cmd_remove(const char *arg);
void connect_reply(DBusMessage *message, void *user_data);
void cmd_connect(const char *arg);
void disconn_reply(DBusMessage *message, void *user_data);
void cmd_disconn(const char *arg);
void cmd_list_attributes(const char *arg);
void cmd_set_alias(const char *arg);
void cmd_select_attribute(const char *arg);
struct GDBusProxy *find_attribute(const char *arg);
void cmd_attribute_info(const char *arg);
void cmd_read(const char *arg);
void cmd_write(const char *arg);
void cmd_notify(const char *arg);
void cmd_register_profile(const char *arg);
void cmd_unregister_profile(const char *arg);
void cmd_version(const char *arg);
void cmd_quit(const char *arg);
char *generic_generator(const char *text, int state,
					GList *source, const char *property);
char *ctrl_generator(const char *text, int state);
char *dev_generator(const char *text, int state);
char *attribute_generator(const char *text, int state);
char *capability_generator(const char *text, int state);

#endif	/* BLE_API_H */
