#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wordexp.h>
#include <inttypes.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <glib.h>

#include "ble_api.h"
#include "util.h"
#include "uuid.h"
#include "agent.h"
#include "display.h"
#include "gatt.h"

void print_adapter(GDBusProxy *proxy, const char *description)
{
	DBusMessageIter iter;
	const char *address, *name;

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &address);

	if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
		dbus_message_iter_get_basic(&iter, &name);
	else
		name = "<unknown>";

	rl_printf("%s%s%sController %s %s %s\n",
				description ? "[" : "",
				description ? : "",
				description ? "] " : "",
				address, name,
				default_ctrl &&
				default_ctrl->proxy == proxy ?
				"[default]" : "");

}

gboolean device_is_child(GDBusProxy *device, GDBusProxy *master)
{
	DBusMessageIter iter;
	const char *adapter, *path;

	if (!master)
		return FALSE;

	if (g_dbus_proxy_get_property(device, "Adapter", &iter) == FALSE)
		return FALSE;

	dbus_message_iter_get_basic(&iter, &adapter);
	path = g_dbus_proxy_get_path(master);

	if (!strcmp(path, adapter))
		return TRUE;

	return FALSE;
}

gboolean service_is_child(GDBusProxy *service)
{
	GList *l;
	DBusMessageIter iter;
	const char *device, *path;

	if (g_dbus_proxy_get_property(service, "Device", &iter) == FALSE)
		return FALSE;

	dbus_message_iter_get_basic(&iter, &device);

	if (!default_ctrl)
		return FALSE;

	for (l = default_ctrl->devices; l; l = g_list_next(l)) {
		struct GDBusProxy *proxy = l->data;

		path = g_dbus_proxy_get_path(proxy);

		if (!strcmp(path, device))
			return TRUE;
	}

	return FALSE;
}

struct adapter *find_parent(GDBusProxy *device)
{
	GList *list;

	for (list = g_list_first(ctrl_list); list; list = g_list_next(list)) {
		struct adapter *adapter = list->data;

		if (device_is_child(device, adapter->proxy) == TRUE)
			return adapter;
	}
	return NULL;
}

void set_default_device(GDBusProxy *proxy, const char *attribute)
{
	char *desc = NULL;
	DBusMessageIter iter;
	const char *path;

	default_dev = proxy;

	if (proxy == NULL) {
		default_attr = NULL;
		goto done;
	}

	if (!g_dbus_proxy_get_property(proxy, "Alias", &iter)) {
		if (!g_dbus_proxy_get_property(proxy, "Address", &iter))
			goto done;
	}

	path = g_dbus_proxy_get_path(proxy);

	dbus_message_iter_get_basic(&iter, &desc);
	desc = g_strdup_printf(COLOR_BLUE "[%s%s%s]" COLOR_OFF "# ", desc,
				attribute ? ":" : "",
				attribute ? attribute + strlen(path) : "");

done:
	rl_set_prompt(desc ? desc : PROMPT_ON);
	printf("\r");
	rl_on_new_line();
	g_free(desc);
}

void device_added(GDBusProxy *proxy)
{
	DBusMessageIter iter;
	struct adapter *adapter = find_parent(proxy);

	if (!adapter) {
		/* TODO: Error */
		return;
	}

	adapter->devices = g_list_append(adapter->devices, proxy);
	print_device(proxy, COLORED_NEW);

	if (default_dev)
		return;

	if (g_dbus_proxy_get_property(proxy, "Connected", &iter)) {
		dbus_bool_t connected;

		dbus_message_iter_get_basic(&iter, &connected);

		if (connected)
			set_default_device(proxy, NULL);
	}
}

void adapter_added(GDBusProxy *proxy)
{
	struct adapter *adapter = g_malloc0(sizeof(struct adapter));

	adapter->proxy = proxy;
	ctrl_list = g_list_append(ctrl_list, adapter);

	if (!default_ctrl)
		default_ctrl = adapter;

	print_adapter(proxy, COLORED_NEW);
}

void proxy_added(GDBusProxy *proxy, void *user_data)
{
	const char *interface;
	const char *proxy_path;

	interface = g_dbus_proxy_get_interface(proxy);

	if (!strcmp(interface, "org.bluez.Device1")) {
		device_added(proxy);
	} else if (!strcmp(interface, "org.bluez.Adapter1")) {
		adapter_added(proxy);
	} else if (!strcmp(interface, "org.bluez.AgentManager1")) {
		if (!agent_manager) {
			agent_manager = proxy;

			if (auto_register_agent)
				agent_register(dbus_conn, agent_manager,
							auto_register_agent);
		}
	} else if (!strcmp(interface, "org.bluez.GattService1")) {
		if (service_is_child(proxy))
			gatt_add_service(proxy);
	} else if (!strcmp(interface, "org.bluez.GattCharacteristic1")) {
		gatt_add_characteristic(proxy);
		proxy_path = g_dbus_proxy_get_path(proxy);
		//rl_printf("----> %s\n", proxy_path);
/*
		if(strstr(proxy_path, "service0028/char0029") != NULL) {
			buzz_char_path = proxy_path;
			//rl_printf("its here buzz\n");
		}
		else if(strstr(proxy_path, "service0028/char0031") != NULL) {
			rssival_char_path = proxy_path;
			//rl_printf("its here rssival\n");
		}
		else if(strstr(proxy_path, "service0028/char0035") != NULL) {
			solarval_char_path = proxy_path;
			//rl_printf("its here solarval\n");
		}
		else if(strstr(proxy_path, "service0028/char0033") != NULL) {
			rssimin_char_path = proxy_path;
			//rl_printf("its here rssimin\n");
		}
		else if(strstr(proxy_path, "service0028/char0037") != NULL) {
			solarmin_char_path = proxy_path;
			//rl_printf("its here solarmin\n");
		}
		else if(strstr(proxy_path, "service0028/char002f") != NULL) {
			randint_char_path = proxy_path;
			//rl_printf("its here randint\n");
		}
		else if(strstr(proxy_path, "service0028/char002d") != NULL) {
			fixedint_char_path = proxy_path;
			//rl_printf("its here fixedint\n");
		}
		else if(strstr(proxy_path, "service0028/char002b") != NULL) {
			mode_char_path = proxy_path;
			//rl_printf("its here mode\n");
		}
		else if(strstr(proxy_path, "service0028/char0039") != NULL) {
			pass_char_path = proxy_path;
			//rl_printf("its here pass\n");
		}
*/
	} else if (!strcmp(interface, "org.bluez.GattDescriptor1")) {
		gatt_add_descriptor(proxy);
	} else if (!strcmp(interface, "org.bluez.GattManager1")) {
		gatt_add_manager(proxy);
	}
}

void set_default_attribute(GDBusProxy *proxy)
{
	const char *path;

	default_attr = proxy;

	path = g_dbus_proxy_get_path(proxy);

	set_default_device(default_dev, path);
}

void device_removed(GDBusProxy *proxy)
{
	struct adapter *adapter = find_parent(proxy);
	if (!adapter) {
		/* TODO: Error */
		return;
	}

	adapter->devices = g_list_remove(adapter->devices, proxy);

	print_device(proxy, COLORED_DEL);

	if (default_dev == proxy)
		set_default_device(NULL, NULL);
}

void adapter_removed(GDBusProxy *proxy)
{
	GList *ll;

	for (ll = g_list_first(ctrl_list); ll; ll = g_list_next(ll)) {
		struct adapter *adapter = ll->data;

		if (adapter->proxy == proxy) {
			print_adapter(proxy, COLORED_DEL);

			if (default_ctrl && default_ctrl->proxy == proxy) {
				default_ctrl = NULL;
				set_default_device(NULL, NULL);
			}

			ctrl_list = g_list_remove_link(ctrl_list, ll);
			g_list_free(adapter->devices);
			g_free(adapter);
			g_list_free(ll);
			return;
		}
	}
}

void proxy_removed(GDBusProxy *proxy, void *user_data)
{
	const char *interface;

	interface = g_dbus_proxy_get_interface(proxy);

	if (!strcmp(interface, "org.bluez.Device1")) {
		device_removed(proxy);
	} else if (!strcmp(interface, "org.bluez.Adapter1")) {
		adapter_removed(proxy);
	} else if (!strcmp(interface, "org.bluez.AgentManager1")) {
		if (agent_manager == proxy) {
			agent_manager = NULL;
			if (auto_register_agent)
				agent_unregister(dbus_conn, NULL);
		}
	} else if (!strcmp(interface, "org.bluez.GattService1")) {
		gatt_remove_service(proxy);

		if (default_attr == proxy)
			set_default_attribute(NULL);
	} else if (!strcmp(interface, "org.bluez.GattCharacteristic1")) {
		gatt_remove_characteristic(proxy);

		if (default_attr == proxy)
			set_default_attribute(NULL);
	} else if (!strcmp(interface, "org.bluez.GattDescriptor1")) {
		gatt_remove_descriptor(proxy);

		if (default_attr == proxy)
			set_default_attribute(NULL);
	} else if (!strcmp(interface, "org.bluez.GattManager1")) {
		gatt_remove_manager(proxy);
	}
}

void property_changed(GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data)
{
	const char *interface;

	interface = g_dbus_proxy_get_interface(proxy);

	if (!strcmp(interface, "org.bluez.Device1")) {
		if (default_ctrl && device_is_child(proxy,
					default_ctrl->proxy) == TRUE) {
			DBusMessageIter addr_iter;
			char *str;

			if (g_dbus_proxy_get_property(proxy, "Address",
							&addr_iter) == TRUE) {
				const char *address;

				dbus_message_iter_get_basic(&addr_iter,
								&address);
				str = g_strdup_printf("[" COLORED_CHG
						"] Device %s ", address);
			} else
				str = g_strdup("");

			if (strcmp(name, "Connected") == 0) {
				dbus_bool_t connected;

				dbus_message_iter_get_basic(iter, &connected);

				if (connected && default_dev == NULL)
					set_default_device(proxy, NULL);
				else if (!connected && default_dev == proxy)
					set_default_device(NULL, NULL);
			}

			//print_iter(str, name, iter);
			g_free(str);
		}
	} else if (!strcmp(interface, "org.bluez.Adapter1")) {
		DBusMessageIter addr_iter;
		char *str;

		if (g_dbus_proxy_get_property(proxy, "Address",
						&addr_iter) == TRUE) {
			const char *address;

			dbus_message_iter_get_basic(&addr_iter, &address);
			str = g_strdup_printf("[" COLORED_CHG
						"] Controller %s ", address);
		} else
			str = g_strdup("");

		//print_iter(str, name, iter);
		g_free(str);
	} else if (proxy == default_attr) {
		char *str;
		char * attr_str;
		unsigned char byte;


		str = g_strdup_printf("[" COLORED_CHG "] Attribute %s ",
						g_dbus_proxy_get_path(proxy));

		//rl_printf("--> %s\n",name);
		//rl_printf("-+> %s\n",str);
		//print_stat(str, name, iter);
		g_free(str);
	}
}

struct adapter *find_ctrl_by_address(GList *source, const char *address)
{
	GList *list;

	for (list = g_list_first(source); list; list = g_list_next(list)) {
		struct adapter *adapter = list->data;
		DBusMessageIter iter;
		const char *str;

		if (g_dbus_proxy_get_property(adapter->proxy,
					"Address", &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &str);

		if (!strcmp(str, address))
			return adapter;
	}

	return NULL;
}

GDBusProxy *find_proxy_by_address(GList *source, const char *address)
{
	GList *list;

	for (list = g_list_first(source); list; list = g_list_next(list)) {
		GDBusProxy *proxy = list->data;
		DBusMessageIter iter;
		const char *str;

		if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &str);

		if (!strcmp(str, address))
			return proxy;
	}

	return NULL;
}

gboolean check_default_ctrl(void)
{
	if (!default_ctrl) {
		rl_printf("No default controller available\n");
		return FALSE;
	}

	return TRUE;
}

gboolean parse_argument_on_off(const char *arg, dbus_bool_t *value)
{
	if (!arg || !strlen(arg)) {
		rl_printf("Missing on/off argument\n");
		return FALSE;
	}

	if (!strcmp(arg, "on") || !strcmp(arg, "yes")) {
		*value = TRUE;
		return TRUE;
	}

	if (!strcmp(arg, "off") || !strcmp(arg, "no")) {
		*value = FALSE;
		return TRUE;
	}

	rl_printf("Invalid argument %s\n", arg);
	return FALSE;
}

gboolean parse_argument_agent(const char *arg, dbus_bool_t *value,
							const char **capability)
{
	const char * const *opt;

	if (arg == NULL || strlen(arg) == 0) {
		rl_printf("Missing on/off/capability argument\n");
		return FALSE;
	}

	if (strcmp(arg, "on") == 0 || strcmp(arg, "yes") == 0) {
		*value = TRUE;
		*capability = "";
		return TRUE;
	}

	if (strcmp(arg, "off") == 0 || strcmp(arg, "no") == 0) {
		*value = FALSE;
		return TRUE;
	}

	for (opt = agent_arguments; *opt; opt++) {
		if (strcmp(arg, *opt) == 0) {
			*value = TRUE;
			*capability = *opt;
			return TRUE;
		}
	}

	rl_printf("Invalid argument %s\n", arg);
	return FALSE;
}

void cmd_list(const char *arg)
{
	GList *list;

	for (list = g_list_first(ctrl_list); list; list = g_list_next(list)) {
		struct adapter *adapter = list->data;
		print_adapter(adapter->proxy, NULL);
	}
}

void cmd_show(const char *arg)
{
	struct adapter *adapter;
	GDBusProxy *proxy;
	DBusMessageIter iter;
	const char *address;

	if (!arg || !strlen(arg)) {
		if (check_default_ctrl() == FALSE)
			return;

		proxy = default_ctrl->proxy;
	} else {
		adapter = find_ctrl_by_address(ctrl_list, arg);
		if (!adapter) {
			rl_printf("Controller %s not available\n", arg);
			return;
		}
		proxy = adapter->proxy;
	}

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &address);
	rl_printf("Controller %s\n", address);

	print_property(proxy, "Name");
/*
	print_property(proxy, "Alias");
	print_property(proxy, "Class");
	print_property(proxy, "Powered");
	print_property(proxy, "Discoverable");
	print_property(proxy, "Pairable");
	print_uuids(proxy);
	print_property(proxy, "Modalias");
*/
	print_property(proxy, "Discovering");
}

void cmd_select(const char *arg)
{
	struct adapter *adapter;

	if (!arg || !strlen(arg)) {
		rl_printf("Missing controller address argument\n");
		return;
	}

	adapter = find_ctrl_by_address(ctrl_list, arg);
	if (!adapter) {
		rl_printf("Controller %s not available\n", arg);
		return;
	}

	if (default_ctrl && default_ctrl->proxy == adapter->proxy)
		return;

	default_ctrl = adapter;
	print_adapter(adapter->proxy, NULL);
}

void cmd_devices(const char *arg)
{
	GList *ll;

	if (check_default_ctrl() == FALSE)
		return;

	for (ll = g_list_first(default_ctrl->devices);
			ll; ll = g_list_next(ll)) {
		GDBusProxy *proxy = ll->data;
		print_device(proxy, NULL);
	}
}

void cmd_paired_devices(const char *arg)
{
	GList *ll;

	if (check_default_ctrl() == FALSE)
		return;

	for (ll = g_list_first(default_ctrl->devices);
			ll; ll = g_list_next(ll)) {
		GDBusProxy *proxy = ll->data;
		DBusMessageIter iter;
		dbus_bool_t paired;

		if (g_dbus_proxy_get_property(proxy, "Paired", &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &paired);
		if (!paired)
			continue;

		print_device(proxy, NULL);
	}
}

gboolean is_paired(const char *arg)
{
	GList *ll;
	DBusMessageIter iter;
	const char *address, *name;

	if (check_default_ctrl() == FALSE)
		return FALSE;

	for (ll = g_list_first(default_ctrl->devices);
			ll; ll = g_list_next(ll)) {
		GDBusProxy *proxy = ll->data;
		DBusMessageIter iter;
		dbus_bool_t paired;

		if (g_dbus_proxy_get_property(proxy, "Paired", &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &paired);
		if (!paired)
			continue;

	    if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
		    return FALSE;

	    dbus_message_iter_get_basic(&iter, &address);
		if(strcmp(address,arg)==0) {
			return TRUE;
		}
	}
	return FALSE;
}

void generic_callback(const DBusError *error, void *user_data)
{
	char *str = user_data;

	if (dbus_error_is_set(error))
		rl_printf("Failed to set %s: %s\n", str, error->name);
	else
		rl_printf("Changing %s succeeded\n", str);
}

void cmd_system_alias(const char *arg)
{
	char *name;

	if (!arg || !strlen(arg)) {
		rl_printf("Missing name argument\n");
		return;
	}

	if (check_default_ctrl() == FALSE)
		return;

	name = g_strdup(arg);

	if (g_dbus_proxy_set_property_basic(default_ctrl->proxy, "Alias",
					DBUS_TYPE_STRING, &name,
					generic_callback, name, g_free) == TRUE)
		return;

	g_free(name);
}

void cmd_reset_alias(const char *arg)
{
	char *name;

	if (check_default_ctrl() == FALSE)
		return;

	name = g_strdup("");

	if (g_dbus_proxy_set_property_basic(default_ctrl->proxy, "Alias",
					DBUS_TYPE_STRING, &name,
					generic_callback, name, g_free) == TRUE)
		return;

	g_free(name);
}

void cmd_power(const char *arg)
{
	dbus_bool_t powered;
	char *str;

	if (parse_argument_on_off(arg, &powered) == FALSE)
		return;

	if (check_default_ctrl() == FALSE)
		return;

	str = g_strdup_printf("power %s", powered == TRUE ? "on" : "off");

	if (g_dbus_proxy_set_property_basic(default_ctrl->proxy, "Powered",
					DBUS_TYPE_BOOLEAN, &powered,
					generic_callback, str, g_free) == TRUE)
		return;

	g_free(str);
}

void cmd_agent(const char *arg)
{
	dbus_bool_t enable;
	const char *capability;

	if (parse_argument_agent(arg, &enable, &capability) == FALSE)
		return;

	if (enable == TRUE) {
		g_free(auto_register_agent);
		auto_register_agent = g_strdup(capability);

		if (agent_manager)
			agent_register(dbus_conn, agent_manager,
						auto_register_agent);
		else
			rl_printf("Agent registration enabled\n");
	} else {
		g_free(auto_register_agent);
		auto_register_agent = NULL;

		if (agent_manager)
			agent_unregister(dbus_conn, agent_manager);
		else
			rl_printf("Agent registration disabled\n");
	}
}

void cmd_default_agent(const char *arg)
{
	agent_default(dbus_conn, agent_manager);
}

void start_discovery_reply(DBusMessage *message, void *user_data)
{
	dbus_bool_t enable = GPOINTER_TO_UINT(user_data);
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		rl_printf("Failed to %s discovery: %s\n",
				enable == TRUE ? "start" : "stop", error.name);
		dbus_error_free(&error);
		return;
	}

	rl_printf("Discovery %s\n", enable == TRUE ? "started" : "stopped");
}

void cmd_scan(const char *arg)
{
	dbus_bool_t enable;
	const char *method;

	if (parse_argument_on_off(arg, &enable) == FALSE)
		return;

	if (check_default_ctrl() == FALSE)
		return;

	if (enable == TRUE)
		method = "StartDiscovery";
	else
		method = "StopDiscovery";

	if (g_dbus_proxy_method_call(default_ctrl->proxy, method,
				NULL, start_discovery_reply,
				GUINT_TO_POINTER(enable), NULL) == FALSE) {
		rl_printf("Failed to %s discovery\n",
					enable == TRUE ? "start" : "stop");
		return;
	}
}

struct GDBusProxy *find_device(const char *arg)
{
	GDBusProxy *proxy;

	if (!arg || !strlen(arg)) {
		if (default_dev)
			return default_dev;
		rl_printf("Missing device address argument\n");
		return NULL;
	}

	if (check_default_ctrl() == FALSE)
		return NULL;

	proxy = find_proxy_by_address(default_ctrl->devices, arg);
	if (!proxy) {
		rl_printf("Device %s not available\n", arg);
		return NULL;
	}

	return proxy;
}

void cmd_info(const char *arg)
{
	GDBusProxy *proxy;
	DBusMessageIter iter;
	const char *address;

	proxy = find_device(arg);
	if (!proxy)
		return;

	if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &address);
	rl_printf("Device %s\n", address);

	print_property(proxy, "Name");
	print_property(proxy, "Alias");
	print_property(proxy, "Class");
	print_property(proxy, "Appearance");
	print_property(proxy, "Icon");
	print_property(proxy, "Paired");
	print_property(proxy, "Trusted");
	print_property(proxy, "Blocked");
	print_property(proxy, "Connected");
	print_property(proxy, "LegacyPairing");
	//print_uuids(proxy);
	print_property(proxy, "Modalias");
	print_property(proxy, "ManufacturerData");
	print_property(proxy, "ServiceData");
	print_property(proxy, "RSSI");
	print_property(proxy, "TxPower");
}

void pair_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		rl_printf("Failed to pair: %s\n", error.name);
		dbus_error_free(&error);
		return;
	}

	rl_printf("Pairing successful\n");
}

void cmd_pair(const char *arg)
{
	GDBusProxy *proxy;

	proxy = find_device(arg);
	if (!proxy)
		return;

	if (g_dbus_proxy_method_call(proxy, "Pair", NULL, pair_reply,
							NULL, NULL) == FALSE) {
		rl_printf("Failed to pair\n");
		return;
	}

	rl_printf("Attempting to pair with %s\n", arg);
}

void cmd_trust(const char *arg)
{
	GDBusProxy *proxy;
	dbus_bool_t trusted;
	char *str;

	proxy = find_device(arg);
	if (!proxy)
		return;

	trusted = TRUE;

	str = g_strdup_printf("%s trust", arg);

	if (g_dbus_proxy_set_property_basic(proxy, "Trusted",
					DBUS_TYPE_BOOLEAN, &trusted,
					generic_callback, str, g_free) == TRUE)
		return;

	g_free(str);
}

void cmd_untrust(const char *arg)
{
	GDBusProxy *proxy;
	dbus_bool_t trusted;
	char *str;

	proxy = find_device(arg);
	if (!proxy)
		return;

	trusted = FALSE;

	str = g_strdup_printf("%s untrust", arg);

	if (g_dbus_proxy_set_property_basic(proxy, "Trusted",
					DBUS_TYPE_BOOLEAN, &trusted,
					generic_callback, str, g_free) == TRUE)
		return;

	g_free(str);
}

void remove_device_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		rl_printf("Failed to remove device: %s\n", error.name);
		dbus_error_free(&error);
		return;
	}

	rl_printf("Device has been removed\n");
}

void remove_device_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = user_data;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

void remove_device(GDBusProxy *proxy)
{
	char *path;

	path = g_strdup(g_dbus_proxy_get_path(proxy));

	if (!default_ctrl)
		return;

	if (g_dbus_proxy_method_call(default_ctrl->proxy, "RemoveDevice",
						remove_device_setup,
						remove_device_reply,
						path, g_free) == FALSE) {
		rl_printf("Failed to remove device\n");
		g_free(path);
	}
}

void cmd_remove(const char *arg)
{
	GDBusProxy *proxy;

	if (!arg || !strlen(arg)) {
		rl_printf("Missing device address argument\n");
		return;
	}

	if (check_default_ctrl() == FALSE)
		return;

	if (strcmp(arg, "*") == 0) {
		GList *list;

		for (list = default_ctrl->devices; list;
						list = g_list_next(list)) {
			GDBusProxy *proxy = list->data;

			remove_device(proxy);
		}
		return;
	}

	proxy = find_proxy_by_address(default_ctrl->devices, arg);
	if (!proxy) {
		rl_printf("Device %s not available\n", arg);
		return;
	}

	remove_device(proxy);
}

void connect_reply(DBusMessage *message, void *user_data)
{
	GDBusProxy *proxy = user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		rl_printf("Failed to connect: %s\n", error.name);
		dbus_error_free(&error);
		return;
	}

	rl_printf("Connection successful\n");

	set_default_device(proxy, NULL);
}

void cmd_connect(const char *arg)
{
	GDBusProxy *proxy;

	if (!arg || !strlen(arg)) {
		rl_printf("Missing device address argument\n");
		return;
	}

	if (check_default_ctrl() == FALSE)
		return;

	proxy = find_proxy_by_address(default_ctrl->devices, arg);
	if (!proxy) {
		rl_printf("Device %s not available\n", arg);
		return;
	}

	if (g_dbus_proxy_method_call(proxy, "Connect", NULL, connect_reply,
							proxy, NULL) == FALSE) {
		rl_printf("Failed to connect\n");
		return;
	}

	rl_printf("Attempting to connect to %s\n", arg);
}

void disconn_reply(DBusMessage *message, void *user_data)
{
	GDBusProxy *proxy = user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		rl_printf("Failed to disconnect: %s\n", error.name);
		dbus_error_free(&error);
		return;
	}

	rl_printf("Successful disconnected\n");

	if (proxy != default_dev)
		return;

	set_default_device(NULL, NULL);
}

void cmd_disconn(const char *arg)
{
	GDBusProxy *proxy;

	proxy = find_device(arg);
	if (!proxy)
		return;

	if (g_dbus_proxy_method_call(proxy, "Disconnect", NULL, disconn_reply,
							proxy, NULL) == FALSE) {
		rl_printf("Failed to disconnect\n");
		return;
	}
	if (strlen(arg) == 0) {
		DBusMessageIter iter;

		if (g_dbus_proxy_get_property(proxy, "Address", &iter) == TRUE)
			dbus_message_iter_get_basic(&iter, &arg);
	}
	rl_printf("Attempting to disconnect from %s\n", arg);
}

void cmd_list_attributes(const char *arg)
{
	GDBusProxy *proxy;

	proxy = find_device(arg);
	if (!proxy)
		return;

	gatt_list_attributes(g_dbus_proxy_get_path(proxy));
}

void cmd_set_alias(const char *arg)
{
	char *name;

	if (!arg || !strlen(arg)) {
		rl_printf("Missing name argument\n");
		return;
	}

	if (!default_dev) {
		rl_printf("No device connected\n");
		return;
	}

	name = g_strdup(arg);

	if (g_dbus_proxy_set_property_basic(default_dev, "Alias",
					DBUS_TYPE_STRING, &name,
					generic_callback, name, g_free) == TRUE)
		return;

	g_free(name);
}

void cmd_select_attribute(const char *arg)
{
	GDBusProxy *proxy;

	if (!arg || !strlen(arg)) {
		rl_printf("Missing attribute argument\n");
		return;
	}

	if (!default_dev) {
		rl_printf("No device connected\n");
		return;
	}

	proxy = gatt_select_attribute(arg);
	if (proxy)
		set_default_attribute(proxy);
}

struct GDBusProxy *find_attribute(const char *arg)
{
	GDBusProxy *proxy;

	if (!arg || !strlen(arg)) {
		if (default_attr)
			return default_attr;
		rl_printf("Missing attribute argument\n");
		return NULL;
	}

	proxy = gatt_select_attribute(arg);
	if (!proxy) {
		rl_printf("Attribute %s not available\n", arg);
		return NULL;
	}

	return proxy;
}

void cmd_attribute_info(const char *arg)
{
	GDBusProxy *proxy;
	DBusMessageIter iter;
	const char *iface, *uuid, *text;

	proxy = find_attribute(arg);
	if (!proxy)
		return;

	if (g_dbus_proxy_get_property(proxy, "UUID", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &uuid);

}

void cmd_read(const char *arg)
{
	if (!default_attr) {
		rl_printf("No attribute selected\n");
		return;
	}
	gatt_read_attribute(default_attr, arg);
}

void cmd_write(const char *arg)
{
	if (!arg || !strlen(arg)) {
		rl_printf("Missing data argument\n");
		return;
	}

	if (!default_attr) {
		rl_printf("No attribute selected\n");
		return;
	}

	gatt_write_attribute(default_attr, arg);
}

void cmd_notify(const char *arg)
{
	dbus_bool_t enable;

	if (parse_argument_on_off(arg, &enable) == FALSE)
		return;

	if (!default_attr) {
		rl_printf("No attribute selected\n");
		return;
	}

	gatt_notify_attribute(default_attr, enable ? true : false);
}

void cmd_register_profile(const char *arg)
{
	wordexp_t w;

	if (check_default_ctrl() == FALSE)
		return;

	if (wordexp(arg, &w, WRDE_NOCMD)) {
		rl_printf("Invalid argument\n");
		return;
	}

	if (w.we_wordc == 0) {
		rl_printf("Missing argument\n");
		return;
	}

	gatt_register_profile(dbus_conn, default_ctrl->proxy, &w);

	wordfree(&w);
}

void cmd_unregister_profile(const char *arg)
{
	if (check_default_ctrl() == FALSE)
		return;

	gatt_unregister_profile(dbus_conn, default_ctrl->proxy);
}

void cmd_version(const char *arg)
{
	rl_printf("Version %s\n", VERSION);
}

void cmd_quit(const char *arg)
{
	g_main_loop_quit(main_loop);
}

char *generic_generator(const char *text, int state,
					GList *source, const char *property)
{
	static int index, len;
	GList *list;

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	for (list = g_list_nth(source, index); list;
						list = g_list_next(list)) {
		GDBusProxy *proxy = list->data;
		DBusMessageIter iter;
		const char *str;

		index++;

		if (g_dbus_proxy_get_property(proxy, property, &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &str);

		if (!strncmp(str, text, len))
			return strdup(str);
        }

	return NULL;
}

char *ctrl_generator(const char *text, int state)
{
	static int index = 0;
	static int len = 0;
	GList *list;

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	for (list = g_list_nth(ctrl_list, index); list;
						list = g_list_next(list)) {
		struct adapter *adapter = list->data;
		DBusMessageIter iter;
		const char *str;

		index++;

		if (g_dbus_proxy_get_property(adapter->proxy,
					"Address", &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &str);

		if (!strncmp(str, text, len))
			return strdup(str);
	}

	return NULL;
}

char *dev_generator(const char *text, int state)
{
	return generic_generator(text, state,
			default_ctrl ? default_ctrl->devices : NULL, "Address");
}

char *attribute_generator(const char *text, int state)
{
	return gatt_attribute_generator(text, state);
}

char *capability_generator(const char *text, int state)
{
	static int index, len;
	const char *arg;

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	while ((arg = agent_arguments[index])) {
		index++;

		if (!strncmp(arg, text, len))
			return strdup(arg);
	}

	return NULL;
}

