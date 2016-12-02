
#include "wdk.h"
#include "include/mtc_linklist.h"

#define UHTTPD_NAME     "uhttpd"
#define UHTTPD_CMDS     "/usr/sbin/"UHTTPD_NAME

#define STOR_ROOT_PATH  "/media"
#define STOR_ROOT_PATH_LEN  (sizeof(STOR_ROOT_PATH)-1)
#define DISK_INFO_FILE  "/var/diskinfo.txt"

struct iNodeItem {
    unsigned char type;
    char *name;
    char *size;
};

typedef struct {
    struct list_head list;
    struct iNodeItem item;
}nodeListNode;

LIST_HEAD(dirs);
LIST_HEAD(files);


off_t get_file_size(char *dir, char *filename)
{
    char disk_dir_full[PATH_MAX];
    struct stat st;

    snprintf(disk_dir_full, sizeof(disk_dir_full), "%s/%s", dir, filename);
    if (!stat(disk_dir_full, &st))
        return st.st_size;
    else
        return 0;
}

int get_file_size_string(char *dir, char *filename, char*out_string)
{
    off_t file_size = get_file_size(dir, filename);

    //GB,TB...
    if (file_size > 1000*1000*1000) {
        sprintf(out_string,"%jdGB", (intmax_t)file_size/(1000*1000*1000));
    //MB
    } else if (file_size > 1000*1000) {
        sprintf(out_string,"%jdMB", (intmax_t)file_size/(1000*1000));
    //KB
    } else if (file_size > 1000) {
        sprintf(out_string,"%jdKB", (intmax_t)file_size/(1000));
    } else {
        sprintf(out_string,"%jdB", (intmax_t)file_size);
    }

    return 0;
}

int add_inode(char *filename, unsigned char type, char *size)
{
    struct list_head *lh = &dirs;
    struct list_head *pos, *next;
    nodeListNode *node, *node1;

    if (type == DT_REG) {
        lh = &files;
    }

    if ((node = (nodeListNode *)calloc(sizeof(nodeListNode), 1)) != NULL) {
        node->list.next = node->list.prev = &node->list;
        node->item.type = type;
        node->item.name = strdup(filename);
        node->item.size = strdup(size);

        if (list_empty(lh)) {
            list_add_tail(&node->list, lh);
        } else {
            list_for_each_safe(pos, next, lh) {
                node1 = (nodeListNode *)list_entry(pos, nodeListNode, list);
                if (strcmp(node->item.name, node1->item.name) <= 0) {
                    list_add(&node->list, pos->prev, pos);
                    return 0;
                }
            }
            list_add_tail(&node->list, lh);
        }
    }

    return 0;
}

int del_inodes(unsigned char type)
{
    struct list_head *lh = &dirs;
    struct list_head *pos, *next;
    nodeListNode *node;

    if (type == DT_REG) {
        lh = &files;
    }

    if (!list_empty(lh)) {
        list_for_each_safe(pos, next, lh) {
            node = (nodeListNode *)list_entry(pos, nodeListNode, list);
            STDOUT("%s/%s/%s/", (node->item.type == DT_DIR) ? "1" : "0", node->item.size, node->item.name);
            list_del(&node->list);
            free(node->item.name);
            free(node->item.size);
            free(node);
        }
    }

    return 0;
}

static int stor_ls_dir(char *disk_dir)
{
    DIR *d;
    struct dirent *dir;
    char file_length[100] = {0};

    LOG("Opening %s!!!", disk_dir);

    if ((d = opendir(disk_dir)) != NULL) {
        STDOUT("1/0/../");
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.')
                continue;
            /* file type*/
            if (dir->d_type == DT_REG) {
                get_file_size_string(disk_dir, dir->d_name, file_length); 
                add_inode(dir->d_name, DT_REG, file_length);
            } else if (dir->d_type == DT_DIR) {
                add_inode(dir->d_name, DT_DIR, "0");
            }
        }
        closedir(d);
        del_inodes(DT_DIR);
        del_inodes(DT_REG);
    } else {
        LOG("No such dir %s!!!", disk_dir);
    }

    return 0;
}

/*
    # input:
    #       stor ls /
    #       stor ls /folder/
    # output:
    #       1/4096/Montagecloud/1/4096/folder/0/12500/WiFi.doc/0/490*714/levels.jpg/0/6940117/Miles Away-Madonna.mp    3/NoneDisk 
    #       1/0/../0/6940117/Miles Away-Madonna Backup.mp3/NoneDisk 

    Define:

    Property1/size1/filename1/Property2/size2/filename2/....../NoneDisk

    |Property   |   Size                |   filename    |
    |1: Foldler |   0                   |   Folder name |
    |0: File    |   real size of file   |   File name   |

========================> File type
    DT_BLK      This is a block device.
    DT_CHR      This is a character device.
    DT_DIR      This is a directory.
    DT_FIFO     This is a named pipe (FIFO).
    DT_LNK      This is a symbolic link.
    DT_REG      This is a regular file.
    DT_SOCK     This is a UNIX domain socket.
    DT_UNKNOWN  The file type is unknown.

*/
static int stor_ls(char *path)
{
    FILE *fp;
    char *name;
    char info[PATH_MAX];
    char phys[PATH_MAX];
    int hasDisk = 0;

    if (f_exists(DISK_INFO_FILE) && ((fp = fopen(DISK_INFO_FILE, "r")) != NULL)) {
        while (new_fgets(info, sizeof(info), fp) != NULL) {
            name = info;
            if (!strncmp(info, STOR_ROOT_PATH, STOR_ROOT_PATH_LEN)) {
                name += STOR_ROOT_PATH_LEN;
                if (!strcmp(path, "/")) {
                    STDOUT("1/0/%s/", ++name);
                    hasDisk = 1;
                } else if (!strncmp(path, name, strlen(name))) {
                    /* Other case, list all files */
                    sprintf(info, "%s%s", STOR_ROOT_PATH, path);
                    memset(phys, 0, sizeof(phys));
                    if (realpath(info, phys) && !strncmp(phys, STOR_ROOT_PATH, STOR_ROOT_PATH_LEN)) {
                        stor_ls_dir(phys);
                        hasDisk = 1;
                    }
                }
            }
        }
    }

    STDOUT("NoneDisk");

    if (!hasDisk) {
        LOG("Disk not present");
    }

    return 0;
}

/*
    # input:
    #       stor rm /path/ name1/name2/name3/
    # output:
    #

Remove argument format:

ARG0=rm
ARG1=Dir file1/file2/file3/.../

ARG0=rm
ARG1=/sda1/Software/ ubuntu-14.04.5-desktop-i386.iso/openssl-1.0.2j.tar.gz/11AC_8812AU_8812AE_Sniffer_20140424.rar/


*/
static int stor_rm(char *path, char *files)
{
    FILE *fp;
    int num, i;
    char *argv[100]; // support delete max files each request
    char *name;
    char info[PATH_MAX];
    char phys[PATH_MAX];

    num = str2argv(files, argv, '/');
    if (num < 1) {
        return 0;
    }

    if (f_exists(DISK_INFO_FILE) && ((fp = fopen(DISK_INFO_FILE, "r")) != NULL)) {
        while (new_fgets(info, sizeof(info), fp) != NULL) {
            name = info;
            if (!strncmp(info, STOR_ROOT_PATH, STOR_ROOT_PATH_LEN)) {
                name += STOR_ROOT_PATH_LEN;
                if (!strncmp(path, name, strlen(name))) {
                    for (i=0; i<num; i++) {
                        sprintf(info, "%s/%s/%s", STOR_ROOT_PATH, path, argv[i]);
                        memset(phys, 0, sizeof(phys));
                        if (realpath(info, phys) && !strncmp(phys, STOR_ROOT_PATH, STOR_ROOT_PATH_LEN)) {
                            LOG("%s: unlink [%s]\n", __func__, phys);
                            unlink(phys);
                        }
                    }
                }
            }
        }
    }
    sync();

    return 0;
}


/*
    may need to convert %20 to space and others convert... 
    uhttpd -d [args]
*/


/*

After disk is mounted, hotplug.d/block/90-storage will save diskinfo to /var/diskinfo.txt
THis program have to functions:
    1.ls - list the dir of SD/USBDisk root
    2.rm - delete some file with specific dir of SD/USBDisk root

The second argument must be provided "ls" or "rm"
*/
int wdk_stor(int argc, char **argv)
{
    int ret = -1;

    if (argc < 2) {
        LOG("ARG too less\n");
    } else if (argc < 3) {
        LOG("ARG0=%s.ARG1=%s\n", argv[0], argv[1]);
        if (!strcmp(argv[0], "ls")) {
            ret = stor_ls(argv[1]);
        } else if (!strcmp(argv[0], "rm")) {
            char *file = argv[1];
            for (; *file!=0; file++) {
                if ((*file == ' ') && (*(file-1) == '/')) {
                    *file++ = 0;
                    break;
                }
            }
            LOG("path=[%s].files=[%s]\n", argv[1], file);
            ret = stor_rm(argv[1], file);
        }
    } else if (argc < 4) {
        LOG("ARG0=%s.ARG1=%s.ARG2=%s\n", argv[0], argv[1], argv[2]);
        ret = stor_rm(argv[1], argv[2]);
    }

    return ret;
}

