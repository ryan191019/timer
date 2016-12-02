
/**
 * @file mtc_client.c
 *
 * @author
 * @date
 */

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "include/mtc_client.h"

const MtcCli mtcCli[] = {
    { OPC_KEY_SHORT_A,      2, 0, "as"        },
    { OPC_KEY_SHORT_B,      2, 0, "bs"        },
    { OPC_KEY_SHORT_C,      2, 0, "cs"        },
    { OPC_KEY_SHORT_D,      2, 0, "ds"        },
    { OPC_KEY_HWHEEL_LEFT,  3, 0, "hl"        },
    { OPC_KEY_HWHEEL_RIGHT, 3, 0, "hr"        },

    { OPC_CMD_ALSA,         3, 1, "alsa"      },
    { OPC_CMD_WDK,          3, 1, "wdk"       },
    { OPC_CMD_USB,          3, 1, "usb"       },
    { OPC_CMD_LCD,          3, 1, "lcd"       },
    { OPC_CMD_BOOT,         3, 1, "boot"      },
#ifdef CONFIG_PACKAGE_libdbus    
    { OPC_CMD_DBUS,         3, 1, "dbus"      },
#endif    
    { OPC_CMD_AVAHI,        3, 1, "avahi"     },
    { OPC_CMD_CROND,        3, 1, "crond"     },
    { OPC_CMD_CDB,          3, 1, "cdb"       },
    { OPC_CMD_TELNET,       3, 1, "telnet"    },
//    { OPC_CMD_UARTD,        3, 1, "uartd"     },
//   { OPC_CMD_MPD,       	3, 1, "mpd"    	  },
//    { OPC_CMD_POSTPLAYLIST,       	3, 1, "postplaylist"    	  },
    { OPC_CMD_OCFG,         3, 1, "ocfg"      },
    { OPC_CMD_DONE,         3, 1, "done"      },
    { OPC_CMD_SYSCTL,       3, 1, "sysctl"    },
    { OPC_CMD_UMOUNT,       3, 1, "umount"    },
    { OPC_CMD_LOG,          3, 1, "log"       },
    { OPC_CMD_OP,           3, 1, "op"        },
    { OPC_CMD_VLAN,         3, 1, "vlan"      },
    { OPC_CMD_LAN,          3, 1, "lan"       },
    { OPC_CMD_LAN6,         3, 1, "lan6"      },
    { OPC_CMD_WL,           3, 1, "wl"        },
    { OPC_CMD_WAN,          3, 1, "wan"       },
    { OPC_CMD_FW,           3, 1, "fw"        },
    { OPC_CMD_NAT,          3, 1, "nat"       },
    { OPC_CMD_ROUTE,        3, 1, "route"     },
    { OPC_CMD_DDNS,         3, 1, "ddns"      },
    { OPC_CMD_DLNA,         3, 1, "dlna"      },
    { OPC_CMD_HTTP,         3, 1, "http"      },
    { OPC_CMD_DNS,          3, 1, "dns"       },
    { OPC_CMD_SMB,          3, 1, "smb"       },
    { OPC_CMD_FTP,          3, 1, "ftp"       },
    { OPC_CMD_PRN,          3, 1, "prn"       },
    { OPC_CMD_QOS,          3, 1, "qos"       },
    { OPC_CMD_SYS,          3, 1, "sys"       },
    { OPC_CMD_ANTIBB,       3, 1, "antibb"    },
    { OPC_CMD_IGMP,         3, 1, "igmp"      },
    { OPC_CMD_LLD2,         3, 1, "lld2"      },
    { OPC_CMD_UPNP,         3, 1, "upnp"      },
    { OPC_CMD_RA,           3, 1, "ra"        },
    { OPC_CMD_OMICFG,       3, 1, "omicfg"    },

    { OPC_CMD_COMMIT,       2, 0, "commit"    },
    { OPC_CMD_SAVE,         2, 0, "save"      },
    { OPC_CMD_SHUTDOWN,     2, 0, "shutdown"  },
    { OPC_CMD_WANIPUP,      2, 0, "wanipup"   },
    { OPC_CMD_WANIPDOWN,    2, 0, "wanipdown" },
    { OPC_CMD_OCFGARG,      2, 0, "ocfgarg"   },
    { OPC_DAT_INFO,         2, 0, "info"      },
};
const int mtcCliCount = ARRAY_SIZE(mtcCli);

int mtc_client_call(MtcPkt *packet, client_callback ccb)
{
    struct sockaddr_un addr;
    char tpath[PATH_MAX];
    char rbuf[1024];
    int sockfd;
    int len;
    int ret;

    if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < RET_OK) {
        perror("Create socket failed!!");
        return RET_ERR;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;

    sprintf(tpath, "%s%d", MTC_CLIENT_SOCK, getpid());
    strcpy(addr.sun_path, tpath);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

    unlink(tpath);

    if(bind(sockfd, (struct sockaddr *)&addr, len) < RET_OK) {
        perror("Bind failed!!");
        ret = RET_ERR;
        goto err;
    }

    if(chmod(addr.sun_path, S_IRWXU) < RET_OK) {
        perror("Change mode failed!!");
        ret = RET_ERR;
        goto err;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, MTCSOCK);

    len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

    if(connect(sockfd, (struct sockaddr *)&addr, len) < RET_OK) {
        perror("Connect failed!!");
        ret = RET_ERR;
        goto err;
    }

    if (write(sockfd, (char *)packet, (sizeof(PHdr) + packet->head.len)) < RET_OK) {
        perror("Write failed!!");
        ret = RET_ERR;
        goto err;
    }

    if ((packet->head.ifc == INFC_DAT) && ccb) {
        while ((len = read(sockfd, rbuf, sizeof(rbuf))) > 0) {
            rbuf[len] = 0;
            ccb(rbuf, len);
        }
    }

    ret = RET_OK;

err:
    close(sockfd);
    unlink(tpath);

    return ret;
}

int mtc_client_simply_call(int op_code, client_callback ccb)
{
    MtcPkt packet;
    MtcPkt *pkt = &packet;
    int i;

    for (i=0; i<mtcCliCount; i++) {
        if ((mtcCli[i].args == 2) && (mtcCli[i].op == op_code)) {
            pkt->head.opc = mtcCli[i].op;

            if (pkt->head.opc < OPC_KEY_MIN)
                return RET_ERR;
            else if (pkt->head.opc <= OPC_KEY_MAX)
                pkt->head.ifc = INFC_KEY;
            else if (pkt->head.opc <= OPC_CMD_MAX)
                pkt->head.ifc = INFC_CMD;
            else if (pkt->head.opc <= OPC_DAT_MAX)
                pkt->head.ifc = INFC_DAT;

            return mtc_client_call(pkt, ccb);
        }
    }

    return RET_ERR;
}

