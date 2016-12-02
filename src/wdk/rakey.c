

#include "wdk.h"



#define PLAYLIST "/playlists"

/*
	1=internet radio
	2=airplay&dlna
	3=local player
*/
static int ra_func;


enum mpd_state {
    MPD_STATE_UNKNOWN = 0,
    MPD_STATE_STOP = 1,
    MPD_STATE_PLAY = 2,
    MPD_STATE_PAUSE = 3,
};




/*
	for example, wanted_index = 1
	so, full name = /playlists/1-ParisOneTrance.m3u
	so, m3u_name = 1-ParisOneTrance
	so, m3u_name_suffix = ParisOneTrance
*/
static void find_matched_m3u(int wanted_index, char *m3u_name, char *m3u_name_suffix)
{
    DIR *d;
    struct dirent *dir;
    char m3u_content[100] = {0};
    char matched_header[10] = {0};


    memset(matched_header, 0, sizeof(matched_header));
    sprintf(matched_header, "%d-", wanted_index);


    d = opendir("/playlists/");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.')
                continue;
            /* file type*/
            if (dir->d_type == DT_REG) {
                /* progress here */
                //LOG("found m3u = %s", dir->d_name);


                /*
                	if not found, continue, and the matched_header must match from start
                	of the string, avoid "1-" matches "11-"
                */
                if ((strstr(dir->d_name, matched_header) == NULL) ||
                    (strstr(dir->d_name, matched_header) != dir->d_name) )
                    continue;


                /* judge we use the content or use the file name to load it.*/
                readline2("/playlists/", dir->d_name, m3u_content, sizeof(m3u_content));
                if ((strstr(m3u_content, ".pls")) ||(strstr(m3u_content, ".m3u"))) {
                    char *tmp = NULL;
                    /*
                    	What's this for? it is verified ok, but what does it do?
                    */
                    tmp = strstr(m3u_content, ".m3u");
                    strncpy(m3u_name, m3u_content, tmp - m3u_content);
                    strcpy(m3u_name_suffix, m3u_name + strlen(matched_header));
                    LOG("Using  %s content = %s, suffix= %s", dir->d_name, m3u_name, m3u_name_suffix);
                } else {
                    char * tmp = NULL;
                    tmp = strstr(dir->d_name, ".m3u");
                    strncpy(m3u_name, dir->d_name, tmp - dir->d_name);
                    strcpy(m3u_name_suffix, m3u_name + strlen(matched_header) );
                    LOG("Using  %s , suffix= %s", m3u_name, m3u_name_suffix);
                }
                /* after we have done*/
                break;
            } else if (dir->d_type ==DT_DIR) {
                /* is a directoty, pass */
            }
        }
        closedir(d);
    } else {
        LOG("No such dir %s!!!", "/playlists/");
    }

    return;
}


static int mpd_alive()
{
    char mpd_pid[10] = {0};

    exec_cmd3(mpd_pid, sizeof(mpd_pid), "pidof mpdaemon");
    if (strlen(mpd_pid) == 0)
        return 0;
    else
        return 1;
}

static int mpd_get_status()
{
    char mpd_result[300];
    exec_cmd3(mpd_result, sizeof(mpd_result), "mpc | grep playing");
    if (strstr(mpd_result, "playing") != NULL)
        return MPD_STATE_PLAY;
    else
        return MPD_STATE_STOP;

}

static int get_ralist_size()
{
    char ra_size[10] = {0};
    exec_cmd3(ra_size, sizeof(ra_size), "cat /etc/ralist | wc -l");
    return atoi(ra_size);
}

static int playable(int play_index)
{
    char m3u_name[100] = {0};
    char m3u_name_suffix[100] = {0};
    find_matched_m3u(play_index, m3u_name, m3u_name_suffix);
    if (strlen(m3u_name) > 0)
        return 1;
    else
        return 0;
}

/*

Verified.


Passed in index, for example 1, wil play 1-ParisOneTrance.m3u
in the case, ra_func must be 1(internet radio)


*/
static void play(int play_index)
{

    char ra_func[10] = {0};
    char ra_cur[10] = {0};
    char mpd_curplay[100] = {0};
    char ra_cur_mpd_curplay[100] = {0};
    char m3u_name[100] = {0};
    char m3u_name_suffix[100] = {0};


    cdb_get("$ra_func", ra_func);
    cdb_get("$ra_cur", ra_cur); /* the current play_index in /playlists/ */
    cdb_get("$mpd_curplay", mpd_curplay);	/* the current play name in  /playlists/*/


    /* 	if ra function in cdb not match, return now */
    if ((atoi(ra_func) != 1) && (atoi(ra_func) != 4) )
        return;

    /* if mpd does not exists, wont play here */
    if (mpd_alive() == 0)
        return;

    /* search the matched playlist */
    find_matched_m3u(play_index, m3u_name, m3u_name_suffix);
    if (strlen(m3u_name) == 0)
        return;

    /* if we are already playing the same playlist, exit */
    strcat(ra_cur_mpd_curplay, ra_cur);
    strcat(ra_cur_mpd_curplay, "-");
    strcat(ra_cur_mpd_curplay, mpd_curplay);
    if ((strcmp(ra_cur_mpd_curplay, m3u_name) == 0) && (mpd_get_status() == MPD_STATE_PLAY)) {
        LOG("Still playing the same, exit");
        return;
    }

    exec_cmd("mpc pause");
    exec_cmd("mpc clear");
    exec_cmd2(


        "mpc load \"%s\"", m3u_name);
    exec_cmd("mpc play");

    if (f_exists("/usr/bin/st7565p_lcd_cli")) {
        exec_cmd2("/usr/bin/st7565p_lcd_cli p %s", m3u_name_suffix);
    }

    cdb_set("$mpd_curplay", m3u_name_suffix);
    cdb_set_int("$ra_cur", play_index);
    cdb_set_int("$mpd_curfunc", 1);
    exec_cmd("cdb commit");

}


/*
	1:up
	0:down

*/
static void vol_ctl(int up)
{

    int ra_vol = 0;
    ra_vol = cdb_get_int("$ra_vol", 0);
    if (up) {
        ra_vol +=5;
        if (ra_vol > 100)
            ra_vol = 100;
        if (ra_func == 2)
            exec_cmd("air_cli d volumeup");
    } else {
        ra_vol -=5;
        if (ra_vol < 0)
            ra_vol = 0;
        if (ra_func == 2)
            exec_cmd("air_cli d volumedown");
    }

    if (f_exists("/usr/bin/st7565p_lcd_cli")) {
        exec_cmd2("/usr/bin/st7565p_lcd_cli v %d", ra_vol);
    }

    exec_cmd2("mpc volume %d", ra_vol);
    exec_cmd("cdb commit");

}


static void forward()
{

    int ra_cur = 0;

    if (mpd_alive() == 0)
        return;


    switch(ra_func) {
    case 1:
        ra_cur = cdb_get_int("$ra_cur", 0); /* the current play_index in /playlists/ */
        while (1) {
            if (0 == get_ralist_size())
                return;

            ra_cur++;
            /* increase or rotate the list */
            if (ra_cur > get_ralist_size())
                ra_cur = 1;

            if (playable(ra_cur))
                break;
        }
        LOG("forward, will play %d", ra_cur);
        play(ra_cur);
        break;
    case 2:
        exec_cmd("air_cli d nextitem	");
        exec_cmd("mpc next");
        break;
    case 3:
        exec_cmd("mpc next");
        break;

    }
}


static void backward()
{
    char ra_cur = 0;

    if (mpd_alive() == 0)
        return;


    switch(ra_func) {
    case 1:
        ra_cur = cdb_get_int("$ra_cur", 0); /* the current play_index in /playlists/ */
        while (1) {
            if (0 == get_ralist_size())
                return;

            /* increase or rotate the list */
            if (ra_cur > get_ralist_size())
                ra_cur = 1;
            ra_cur--;
            if (ra_cur < 1)
                ra_cur = get_ralist_size();

            if (playable(ra_cur))
                break;
        }
        LOG("backward, will play %d", ra_cur);
        play(ra_cur);
        break;
    case 2:
        exec_cmd("air_cli d previtem");
        exec_cmd("mpc prev");
        break;
    case 3:
        exec_cmd("mpc prev");
        break;

    }



}

static void toggle()
{
    exec_cmd("mpc toggle");
    if (ra_func == 2)
        exec_cmd("air_cli d playpause");
}

static void switch_func(int count)
{
    int mode = 0;
    if (count == 0)
        return;

    exec_cmd("mpc stop");
    mode = ra_func + count;
    if (mode > 3)
        mode = mode %3;

    cdb_set_int("$ra_func", mode);

    if ((ra_func == 1) && (count ==2))
        exec_cmd("cdb commit");
    else if((ra_func == 3) && (count ==1))
        exec_cmd("cdb commit");
    else
        exec_cmd("/lib/wdk/commit");

    if (f_exists("/usr/bin/st7565p_lcd_cli")) {
        exec_cmd2("/usr/bin/st7565p_lcd_cli m %d", mode);
        sleep(1);
        exec_cmd2("/usr/bin/st7565p_lcd_cli s");
    }

}



/*
	Functions:

	Play: play the playlist of /playlists/, if already playing, skip, cdb value is set for state value.
	volup/down: set the volume of mpd of shairport, cdb value is set for state value.


	Basic definition:
	mcc = Montage Control Center
	rakey = radio key
	rafunc = radio function


*/
int wdk_rakey(int argc , char **argv)
{
    if ((argc > 2) || (argc == 0))
        return -1;

    ra_func = cdb_get_int("$ra_func", 0);


    if ((atoi(argv[0]) >=1) && (atoi(argv[0]) <=6)) {
        play(atoi(argv[0]));
    } else if(str_equal("volup", argv[0])) {
        vol_ctl(1);
    } else if(str_equal("voldown", argv[0])) {
        vol_ctl(0);
    } else if(str_equal("forward", argv[0])) {
        forward();
    } else if(str_equal("backward", argv[0])) {
        backward();
    } else if(str_equal("key1", argv[0])) {
        forward();
    } else if(str_equal("key2", argv[0])) {
        backward();
    }  else if(str_equal("key3", argv[0])) {
        toggle();
    }  else if(str_equal("switch", argv[0])) {
        if (argc == 2)
            switch_func(atoi(argv[1]));
        else
            switch_func(1);
    }

    return 0;
}





