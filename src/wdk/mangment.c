

#include "wdk.h"
#include "cdb.h"


#define HTTD_CONF "/tmp/httpd.conf"



/*

	Translate format  from:
		user=admin&pass=21232F297A57A5A743894A0E4A801FC3
	To:
		/:admin:21232F297A57A5A743894A0E4A801FC3

*/
static void translate_format(char *string_in, char *string_out)
{
	char *p1 = NULL;  
	char *p2 = NULL;  
//	char *p3 = string_in + strlen(string_in);

	p1 = strstr(string_in, "user=");
	p2 = strstr(string_in, "pass=");
	*(p2 - 1) = 0;
	
	if ((p1 != NULL) && (p2 != NULL)) {
		strcat(string_out, "/:");
		strcat(string_out, p1 + 5);
		strcat(string_out, ":");
		strcat(string_out, p2 + 5);
	}

}



/*
	refresh user:pass to /tmp/httpd.conf
*/
int wdk_mangment(int argc, char **argv)
{
    char httpd_config[100] = {0};
    char cdb_config[100] = {0};
    char cdb_config_new[100] = {0};


	if (argc != 0) {
		LOG("no argument needed");
		return -1;
	}
	

	if (0 == access(HTTD_CONF, 0)) { 
		readline(HTTD_CONF, httpd_config, sizeof(httpd_config));
		cdb_get("$sys_user1", cdb_config);
		translate_format(cdb_config, cdb_config_new);
		
		LOG("From http.conf=%s\n cdb=%s\n  cdb_new=%s",
			httpd_config, cdb_config, cdb_config_new);

		if (0 == strcmp(cdb_config_new, httpd_config)) {
			LOG("Configuration are same. nothing to be done");
		} else {
			LOG("overriding http.conf...");
			writeline(HTTD_CONF, cdb_config_new);
		}
	} else {
		LOG("%s not found!!!", HTTD_CONF);
	}

	return 0;
}






