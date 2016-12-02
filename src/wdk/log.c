

#include "wdk.h"


struct log_mail {
		int log_mail_en;
		char  log_sys_ip[18];
		char  log_mail_ip[18];		/* Server IP */
		char log_mail_addr[100];	/* To */
		char log_mail_send[100];	/* From */
		char log_mail_auth[100];	/* username&password */
		char auth_en[100];
		char username[100];
		char password[100];

};

#define LOGMSG "/var/log/messages"
static struct log_mail g_log_mail;




static void init_log()
{
	g_log_mail.log_mail_en = cdb_get_int("$log_mail_en", 0);
	cdb_get("$log_sys_ip", g_log_mail.log_sys_ip);
	cdb_get("$log_mail_ip", g_log_mail.log_mail_ip);
	cdb_get("$log_mail_addr", g_log_mail.log_mail_addr);
	cdb_get("$log_mail_send", g_log_mail.log_mail_send);
	cdb_get("$log_mail_auth", g_log_mail.log_mail_auth);

	/* 
		parse value from log_mail_auth 
		assume there are values when en=1
	*/
	if (strstr(g_log_mail.log_mail_auth, "en=1") != NULL) {
		char *ps = NULL;
		char *un = NULL;
		g_log_mail.log_mail_en = 1;
		ps = strstr(g_log_mail.log_mail_auth, "ps=");
		if (NULL == ps)
			return;
		strcpy(g_log_mail.password, ps+3);
		*(ps -1) = 0x00;
		un = strstr(g_log_mail.log_mail_auth, "un=");
		if (NULL == un)
			return;
		strcpy(g_log_mail.username, un+3);

	}
	
}

static void  append_single_log(char *temp_file_name, char *file_name)
{
	exec_cmd2("echo \"%s\" >> %s", file_name, temp_file_name);
	exec_cmd2("cat %s >> %s", file_name, temp_file_name);
}


static void  append_syslog(char *temp_file_name)
{

	int index = 0;
	char file_name[100] = {0};

	if (f_exists(LOGMSG))
		append_single_log(temp_file_name, LOGMSG);

	while (1) {
		memset(file_name, 0, sizeof(file_name));
		sprintf(file_name, "%s.%d", LOGMSG, index);
		if (f_exists(file_name)) {
			append_single_log(temp_file_name, file_name);
		} else {
			break;
		}
		index++;
	}

}




/*
	cdb example:
	
	root@router:/lib/wdk# cdb set log_mail_auth "en=1&un=test@montage-test.com&ps=1"
	!OK(0)
	root@router:/lib/wdk# cdb set log_mail_ip 192.168.1.213						I have set up a hmail server
	!OK(0)
	root@router:/lib/wdk# cdb set log_mail_addr troy.tan@montage-tech.com
	!OK(0)
	root@router:/lib/wdk# cdb set log_mail_send test@montage-test.com
	!OK(0)


*/

static void send_log()
{
	LOG("---->sending log");

	char temp_file_name[100] = {0};
	char file_header[1000] = {0};
	char date[100] = {0};
	char mail_send_result[100] = {0};


	LOG("mail_en=%d username=%s password=%s", 
		g_log_mail.log_mail_en, g_log_mail.username, g_log_mail.password);


	if (g_log_mail.log_mail_en != 1)
		return;

	exec_cmd3(date, sizeof(date), "date");

	/* generate a temp file with a random file name*/
	exec_cmd3(temp_file_name, sizeof(temp_file_name), "mktemp /tmp/mail.XXXXXXXX");
	LOG("got temp file name %s", temp_file_name);
	sprintf(file_header, "To: %s\nFrom: %s\nDate: %s\nSubject: Send Syslog message\n\n"
		"========= Syslog message starts here =========\n",
		g_log_mail.log_mail_addr,
		g_log_mail.log_mail_send,
		date);


	LOG("header content = %s", file_header);
	writeproc(temp_file_name, file_header);


	append_syslog(temp_file_name);


	/* 
		when the mail server need a username and password,
		almost every mail server in the world need the account
	*/
	if (g_log_mail.auth_en) {

		exec_cmd3(mail_send_result, sizeof(mail_send_result), 
			"/usr/sbin/sendmail -f  %s  -S %s  -au%s  -ap%s  -t < %s", 
			g_log_mail.log_mail_send,  g_log_mail.log_mail_ip,
			g_log_mail.username, g_log_mail.password, temp_file_name);
		if (strlen(mail_send_result) > 0)
			LOG("mail failed, result = %s", mail_send_result);
	} else {
		LOG("This method is not tested, no support!!!");
#if 0
		exec_cmd2("/usr/sbin/sendmail -f %s -S %s -t < %s",
			g_log_mail.log_mail_send, g_log_mail.log_mail_ip, temp_file_name);
#endif
	}

	remove(temp_file_name);
}



static void show_single_log(char *log_file)
{
	LOG("---->dump log:%s", log_file);
#define CHUNK 1024 /* read 1024 bytes at a time */
    char buf[CHUNK];
    FILE *file = NULL;
    size_t nread;
    file = fopen(log_file, "r");
    if (file) {
        while ((nread = fread(buf, 1, sizeof(buf), file)) > 0)
            STDOUT("%s", buf);
        if (ferror(file))
            LOG("read log error");
        fclose(file);
    }
}



static void show_log()
{
	int index = 0;
	char file_name[100] = {0};

	if (f_exists(LOGMSG))
		show_single_log(LOGMSG);

	while (1) {
		memset(file_name, 0, sizeof(file_name));
		sprintf(file_name, "%s.%d", LOGMSG, index);
		if (f_exists(file_name)) {
			show_single_log(file_name);
		} else {
			break;
		}
		index++;
	}
}


static void clean_log()
{
	int index = 0;
	char file_name[100] = {0};

	if (f_exists(LOGMSG)) {
		if (remove(LOGMSG) == 0) {
			LOG("remove file %s success", LOGMSG);
		} else {
			LOG("remove file %s failed", LOGMSG);
		}
	}

	while (1) {
		memset(file_name, 0, sizeof(file_name));
		sprintf(file_name, "%s.%d", LOGMSG, index);
		if (f_exists(file_name)) {
			if (remove(file_name) == 0) {
				LOG("remove file %s success", file_name);
			} else {
				LOG("remove file %s failed", file_name);
			}
		} else {
			break;
		}
		index++;
	}

	exec_cmd("touch /var/log/messages");
}


int wdk_log(int argc, char **argv)
{
	if (argc != 1)
		return -1;

	init_log();

	if (str_equal(argv[0] ,"-s"))
		send_log();
	else if (str_equal(argv[0] ,"-xymta"))
		show_log();
	else if (str_equal(argv[0] ,"clean"))
		clean_log();
	else
		LOG("wdk log failed");


	return 0;
}








