



/*

	Example:


	
		/lib/wdk/smbc upload /mmcblk0/test_19700101084245.avi //SambaServerIPAddress/SharedFolder usr1 123456
		/lib/wdk/smbc upload_status
		


*/

#include "wdk.h"


#define STATE_UPLOAD "/tmp/smbc/upload"


int wdk_smbc(int argc, char **argv)
{

	char upload_content[10] = {0};
	char *file_full_dir = argv[1];
	char *server_addr = argv[2];
	char *user_name = argv[3];
	char *pass_word = argv[4];

	char file_dir[100] = {0};
	char file_name[100] = {0};
	char smb_command[100] = {0};
	char smb_response[100] = {0};

	int i = 0;
	

	if (argc == 0)
		return -1;

	if (str_equal(argv[0], "upload")) {
		/*parse file_name and file_dir*/
		strcpy(file_dir, "/media/");
		for (i = strlen(file_full_dir) -1; i > 1; i--) {
			if (file_full_dir[i] == '/') {
				strcpy(file_name, file_full_dir+i+1);
				file_full_dir[i] = 0x00;
				strcat(file_dir, file_full_dir);
				break;
			}
		}

		LOG("dir=%s name=%s user=%s pass=%s",
			file_dir, file_name, user_name, pass_word);
		writeline(STATE_UPLOAD, "0");
		chdir(file_dir);

		sprintf(smb_command, "smbclient %s %s -U %s -c \"put %s \"",
			server_addr, pass_word, user_name, file_name);
		LOG("Samba command=%s", smb_command);
		exec_cmd3(smb_response, sizeof(smb_response), smb_command);
		LOG("Samba response=%s", smb_response);
		writeline(STATE_UPLOAD, "1");

    } else if (str_equal(argv[0], "upload_status")) {

        readline(STATE_UPLOAD , upload_content, sizeof(upload_content));
        STDOUT("%s", upload_content);
    } else {

        return -1;
    }

	return 0;
}










