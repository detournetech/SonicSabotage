#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <wordexp.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <inttypes.h>

#include "app_api.h"
#include "display.h"
#include "gatt.h"
#include "wifi.h"

const static char * pass_char_path;
const static char * mode_char_path;
const static char * buzz_char_path;
const static char * randint_char_path;
const static char * fixedint_char_path;
const static char * solarmin_char_path;
const static char * rssimin_char_path;
const static char * solarval_char_path;
const static char * rssival_char_path;

static uint8_t solarmin = 0;
static uint8_t rssimin = 0;

void fill_uuids()
{
	GList *l;
	const char *path;

	for (l = gatt_get_charpaths(); l; l = g_list_next(l)) {
		GDBusProxy *proxy = l->data;
		path = g_dbus_proxy_get_path(proxy);
		//rl_printf("+%s\n",path);
		if(strstr(path, "service0028/char0029") != NULL) {
			buzz_char_path = path;
		}
		else if(strstr(path, "service0028/char0031") != NULL) {
			rssival_char_path = path;
		}
		else if(strstr(path, "service0028/char0035") != NULL) {
			solarval_char_path = path;
		}
		else if(strstr(path, "service0028/char0033") != NULL) {
			rssimin_char_path = path;
		}
		else if(strstr(path, "service0028/char0037") != NULL) {
			solarmin_char_path = path;
		}
		else if(strstr(path, "service0028/char002f") != NULL) {
			randint_char_path = path;
		}
		else if(strstr(path, "service0028/char002d") != NULL) {
			fixedint_char_path = path;
		}
		else if(strstr(path, "service0028/char002b") != NULL) {
			mode_char_path = path;
		}
		else if(strstr(path, "service0028/char0039") != NULL) {
			pass_char_path = path;
		}
	}
}

void cmd_scan_burst(const char *arg)
{
	cmd_power("on");
	cmd_scan("on");
	sleep(5);
	cmd_scan("off");
}

void cmd_connect_bond(const char *arg)
{
	cmd_power("on");
    if(is_paired(arg)) {
		rl_printf("was paired\n");
		cmd_connect(arg);
	} else {
		rl_printf("was not paired\n");
		cmd_agent("DisplayYesNo");
		cmd_default_agent(NULL);
		cmd_pair(arg);
	}
	fill_uuids();
}

void cmd_buzz(const char *arg)
{
	dbus_bool_t enable;

	if (parse_argument_on_off(arg, &enable) == FALSE)
		return;

	if (check_default_ctrl() == FALSE)
    	return;

	fill_uuids();
	if(buzz_char_path != NULL) {
		cmd_select_attribute(buzz_char_path);
		if(enable)
			cmd_write("0x01");
		else
			cmd_write("0x00");	
	}
}

void cmd_rssistats(const char *arg)
{
	dbus_bool_t enable;

	if (parse_argument_on_off(arg, &enable) == FALSE)
		return;
	
	fill_uuids();
	if(mode_char_path != NULL) {
		cmd_select_attribute(mode_char_path);
		cmd_write("0x02"); //mode loop	
	}

	if(rssival_char_path != NULL) {
		cmd_select_attribute(rssival_char_path);
	}

	if (!default_attr) {
		rl_printf("No attribute selected\n");
		return;
	}

	gatt_notify_attribute(default_attr, enable ? true : false);
}

void cmd_solarstats(const char *arg)
{
	dbus_bool_t enable;

	if (parse_argument_on_off(arg, &enable) == FALSE)
		return;
	
	fill_uuids();
	if(mode_char_path != NULL) {
		cmd_select_attribute(mode_char_path);
		cmd_write("0x02"); //mode loop	
	}

	if(solarval_char_path != NULL) {
		cmd_select_attribute(solarval_char_path);
	}

	if (!default_attr) {
		rl_printf("No attribute selected\n");
		return;
	}

	gatt_notify_attribute(default_attr, enable ? true : false);
}

void cmd_randint(const char *arg)
{
	uint8_t val = (uint8_t)strtol(arg, NULL, 10);
	if (val < 0 || val > 100)
		return;

	if (check_default_ctrl() == FALSE)
    	return;

	fill_uuids();
	if(mode_char_path != NULL) {
		cmd_select_attribute(mode_char_path);
		if(val == 0)
			cmd_write("0x00"); //mode idle
		else
			cmd_write("0x02"); //mode loop	
	}

	if(randint_char_path != NULL) {
		cmd_select_attribute(randint_char_path);
		cmd_write(arg);	//loop randint
	}
}

void cmd_fixedint(const char *arg)
{
	uint8_t val = (uint8_t)strtol(arg, NULL, 10);
	if (val < 0 || val > 100)
		return;

	if (check_default_ctrl() == FALSE)
    	return;

	fill_uuids();
	if(mode_char_path != NULL) {
		cmd_select_attribute(mode_char_path);
		if(val == 0)
			cmd_write("0x00"); //mode idle
		else
			cmd_write("0x02"); //mode loop	
	}

	if(fixedint_char_path != NULL) {
		cmd_select_attribute(fixedint_char_path);
		cmd_write(arg);	//loop fixedint
	}
}

void cmd_solarmin(const char *arg) 
{
	uint8_t val = (uint8_t)strtol(arg, NULL, 10);
	if (val < 0 || val > 100)
		return;

	solarmin = val;
	rssimin = 0;

	if (check_default_ctrl() == FALSE)
    	return;

	fill_uuids();
	if(mode_char_path != NULL) {
		cmd_select_attribute(mode_char_path);
		if(val == 0)
			cmd_write("0x00"); //mode idle
		else
			cmd_write("0x02"); //mode loop	
	}

	if(solarmin_char_path != NULL) {
		cmd_select_attribute(solarmin_char_path);
		cmd_write(arg);	//loop solar
	}
}

void cmd_rssimin(const char *arg) 
{
	uint8_t val = (uint8_t)strtol(arg, NULL, 10);
	if (val < 1 || val > 100)
		return;

	rssimin = val;
	solarmin = 0;

	if (check_default_ctrl() == FALSE)
    	return;

	fill_uuids();
	if(mode_char_path != NULL) {
		cmd_select_attribute(mode_char_path);
		if(val == 0)
			cmd_write("0x00"); //mode idle
		else
			cmd_write("0x02"); //mode loop	
	}

	if(rssimin_char_path != NULL) {
		cmd_select_attribute(rssimin_char_path);
		cmd_write(arg);	//loop rssi
	}
}


static int sendall(int s, char *buf, uint64_t *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

static void send_fw_file(char *value, char *fw_file_path)
{
	int i;
	rl_printf("Connect to ssid DTCAP with pass %s\n",(char*)value);

	char * ssid_result = (char*)malloc(IW_ESSID_MAX_SIZE+1);

	i = 0;
	do {
		i++; //1 min timeout
		domain(ssid_result);
		if(i%30==0)
			rl_printf("%s\n",ssid_result);

		if(i==120) {
			rl_printf("Timed out.\n");
			free(ssid_result);
			return;
		}
		sleep(1);
	} while(strcmp(ssid_result,"DTCAP")!=0);

	free(ssid_result);

	rl_printf("Associated to AP\n");
	sleep(7); // lucky 7

	//connect
	struct sockaddr_in server;
	char message[1000] , server_reply[2000];
     
    //Create socket
    int sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) { rl_printf("Could not create socket\n"); }
     
    server.sin_addr.s_addr = inet_addr("192.168.1.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 5000 );
 
    //Connect to remote server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return;
    }
   
    rl_printf("Connected to device server\n");
	//send filesize
	struct stat st;
	stat(fw_file_path, &st);
	//size_t size = st.st_size;
	rl_printf("%s ok\n\n",fw_file_path);

	uint64_t fsize = st.st_size; // Put your value
	rl_printf("%" PRIu64 "%" PRIu64 " ok\n\n",st.st_size,fsize);

	char fs_ptr[8] = { 0 };
	for(int k = 0; k < 8; k++) {
		fs_ptr[7-k] = (char)(fsize >> k*8) & 0xff; 
	}
	//memcpy(fs_ptr, (char*)&fsize, 8);
	if(send(sock, fs_ptr, 8, 0) < 0) 
	{
		rl_printf("Send length failed\n");
		return;
	}

	//char filebuf[fsize] = { 0 };
	char filebuf[fsize + 1];
	FILE *fp = fopen(fw_file_path, "rb");
	if (fp != NULL) {
    	size_t newLen = fread(filebuf, sizeof(char), fsize, fp);
    	if ( ferror( fp ) != 0 ) {
        	rl_printf("Error reading file\n");
    	} else {
        	filebuf[newLen++] = '\0';
    	}
    	fclose(fp);
	}

	//send file
	if(sendall(sock, filebuf, &fsize) < 0) 
	{
		rl_printf("Send file failed\n");
		return;
	}

	close(sock);
	free(fw_file_path);
}

static void read_pass_reply(DBusMessage *message, void *user_data)
{
	DBusError error;
	DBusMessageIter iter, array;
	uint8_t *value;
	int len, i;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		rl_printf("Failed to read: %s\n", error.name);
		dbus_error_free(&error);
		return;
	}

	dbus_message_iter_init(message, &iter);

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
		rl_printf("Invalid response to read\n");
		return;
	}

	dbus_message_iter_recurse(&iter, &array);
	dbus_message_iter_get_fixed_array(&array, &value, &len);

	if (len < 0) {
		rl_printf("Unable to parse value\n");
		return;
	}

	send_fw_file(value, user_data);
}

static void read_pass_setup(DBusMessageIter *iter, void *user_data)
{
	DBusMessageIter dict;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
					DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					DBUS_TYPE_STRING_AS_STRING
					DBUS_TYPE_VARIANT_AS_STRING
					DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
					&dict);
	/* TODO: Add offset support */
	dbus_message_iter_close_container(iter, &dict);
}

static void cmd_ota_internal(GDBusProxy *proxy, const char * arg)
{
	const char *iface;
	char *f_path;
	f_path = (char*)malloc(strlen(arg));
	strcpy(f_path, arg);//, strlen(arg));

	iface = g_dbus_proxy_get_interface(proxy);
	if (!strcmp(iface, "org.bluez.GattCharacteristic1") ||
		!strcmp(iface, "org.bluez.GattDescriptor1"))
		{
			if(g_dbus_proxy_method_call(proxy, "ReadValue", 
										read_pass_setup, read_pass_reply,
										(void*)f_path, NULL) == FALSE) 
			{
				rl_printf("Failed to read\n");
				return;
			}
		}
}

void cmd_ota(const char *arg)
{
	//check if connected
	if (check_default_ctrl() == FALSE)
		return;

	//check arg (filepath)
	if(arg == NULL || strlen(arg) < 2 || arg[0] != '/') {
		rl_printf("Requires absolute path to fw bin\n");
		return;
	}

 	struct stat buffer;   
	if(stat(arg, &buffer) != 0) {
		rl_printf("File doesn't exist\n");
		return;
	}
	
	rl_printf("cmd_ota %s\n", arg);

	fill_uuids();
	//put device in fw update mode
	if(mode_char_path != NULL) {
		cmd_select_attribute(mode_char_path);
		cmd_write("0x01");	
	}

	//read the pass characteristic off of device, prompt user to connect
	if(pass_char_path != NULL) {
		cmd_select_attribute(pass_char_path);
		rl_printf("\n");
		cmd_ota_internal(default_attr, arg);
		rl_printf("\n");
	}
}

cmd_table_entry cmd_table[] = {
	{ "scan",		NULL,	cmd_scan_burst, "scan for devices" },
	{ "devices",	NULL,	cmd_devices, "List available devices" },
	{ "connect",	"<dev>",cmd_connect_bond, "Connect device",dev_generator},
	{ "disconnect",	"[dev]",cmd_disconn, "Disconnect device", dev_generator},
	{ "remove",		"<dev>",cmd_remove, "Remove device", dev_generator },

	{ "beep",		"<on|off>",	cmd_buzz,		"continuous beep on / off" },
	{ "randint",	"[0-100]",	cmd_randint,	"set random beep interval" },
	{ "fixedint",	"[0-100]",	cmd_fixedint,	"set fixed beep interval" },
	{ "rssimin",	"[0-100]",	cmd_rssimin,	"rssi reading threshold %" },
	{ "rssistats", 	"<on|off>",	cmd_rssistats,	"show/hide rssi stats" },
	{ "solarmin",  	"[0-100]",	cmd_solarmin, 	"light sensor threshold %" },
	{ "solarstats",	"<on|off>",	cmd_solarstats, "show/hide light stats" },
	{ "ota_update",	"[file_path]", cmd_ota, 	"update fw from abs. path" },

	{ "list",		NULL,	cmd_list, "List ble interfaces" },
	{ "select",		"<if>",	cmd_select, "Select ble interface", ctrl_generator},
	{ "quit",		NULL,	cmd_quit, "Quit program" },
	{ "exit",		NULL,	cmd_quit },
	{ "help" },
	{ }
};


void init_client(void)
{
	rl_printf("-------------------------------------------------------\n");
	rl_printf("- S - O - N - I - C - - S - A - B - O - T - A - G - E -\n");
	rl_printf("-------------------------------------------------------\n");
}

