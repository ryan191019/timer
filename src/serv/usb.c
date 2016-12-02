#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

typedef struct _musb_process_t
{
    const char *file;
    const char *cmd;
} musb_process;

static musb_process musb_proc[] = {
//    { "/etc/hotplug.d/usb/20-modeswitch", "/etc/init.d/musb modeswitch &" },
//    { "/etc/hotplug.d/tty/30-3g_modem", "/etc/init.d/musb modem &" },
//    { "/etc/hotplug.d/net/30-usbnic", "/etc/init.d/musb usbnic &" },
//    { "/etc/hotplug.d/usb/cdc-wdm", "/etc/init.d/musb cdcwdm &" },
    { "/etc/hotplug.d/block/90-storage", "/etc/init.d/musb storage &" },
//    { "/etc/hotplug.d/usb_device/soundcard", "/etc/init.d/musb soundcard &" },
//    { "/etc/hotplug.d/usb_device/webcam", "/etc/init.d/musb webcam &" },

    { .file = NULL }
};

static int start(void)
{
    int i;

    if(f_isdir("/proc/bus/usb"))
       	mount("usbdeffs", "/proc/bus/usb", "usbfs", MS_MGC_VAL, NULL);

    for(i=0; musb_proc[i].file != NULL; i++) {
        if(f_exists(musb_proc[i].file))
            exec_cmd(musb_proc[i].cmd);
    }

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int usb_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

