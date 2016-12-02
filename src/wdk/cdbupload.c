
#include "wdk.h"



enum file_format {
	CDB_UNCOMPRESSED = 0,		//normal,	#CDB
	CDB_GZIP = 0,			//gzip - zcat,	1f8b
	CDB_BZIP2 = 1,			//bzip2 - bzcat,	425a

};

static int try_to_uncompress(char *path)
{

    unsigned char header[2] = {0};
    FILE *fp = NULL;

    fp = fopen(path,"rb"); /* read binary mode */
    if( fp == NULL ) {
        LOG("Error open and read file");
        return -1;
    }
    header[0] = fgetc(fp)&0xff;
    header[1] = fgetc(fp)&0xff;
    fclose(fp);

    if ((header[0]  == '#') && (header[1]  == 'C')) {
        LOG("CDB normal file");
    } else if ((header[0]  ==0x1f) && (header[1]  ==0x8b)) {
        LOG("GZIP format, using zcat");
        exec_cmd2("mv %s /tmp/config.1", path);
        exec_cmd2("zcat /tmp/config.1 > %s", path);
    } else if ((header[0]  ==0x42) && (header[1]  ==0x5a)) {
        LOG("BZIP2 format, using bzcat");
        exec_cmd2("mv %s /tmp/config.1", path);
        exec_cmd2("bzcat /tmp/config.1 > %s", path);
    } else {
        LOG("not a cdb file!");
        return -1;
    }

	return 0;
}



/*
	Write /tmp/config.cdb to cdb
	config.cdb can be compressed
*/
int wdk_cdbupload(int argc, char **argv)
{

	if (argc > 0)
		return -1;

	update_upload_state(WEB_STATE_ONGOING);
	if (f_exists("/tmp/config.cdb")) {
		LOG("/tmp/config.cdb found, start to update");

		if (0 == try_to_uncompress("/tmp/config.cdb")) {
			/* del \r */
			exec_cmd("tr -d \"\r\" < /tmp/config.cdb > /tmp/tmp.cdb");
			exec_cmd("cp /tmp/tmp.cdb /tmp/config.cdb");
			unlink("/tmp/tmp.cdb");
			/* update to cdb */
			cdb_load("/tmp/config.cdb");
			cdb_commit();
			update_upload_state(WEB_STATE_SUCCESS);
		} else {
			update_upload_state(WEB_STATE_FAILURE);
		}
	} else {
		LOG("/tmp/config.cdb not found!");
		update_upload_state(WEB_STATE_FAILURE);
	}

	return 0;
}













