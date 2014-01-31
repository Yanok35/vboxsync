#ifndef CFGFILE_H
#define CFGFILE_H

#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <glib.h>
#include <glib/gstdio.h>

//#define TRUE 1
//#define FALSE (!TRUE)

/* **************** */
/* *** Typedefs *** */
/* **************** */

typedef struct {
    gchar * filename;    // full path file name
    off_t   filesize;    // size in bytes
    time_t  filemtime;   // modification time
    GSList *filecrclist; // checksum
} vdi_file_t;

typedef struct {
    gchar * cfgfile;     // the whole .vbox file of the VM
    GSList * vdifiles;   // list of (vdi_file_t*) entries
} vm_meta_snap_t;

/* ****************** */
/* *** Prototypes *** */
/* ****************** */

vm_meta_snap_t * cfg_file_open (const gchar * filename);
gboolean cfg_file_rewrite (vm_meta_snap_t * vm);

#endif /* CFGFILE_H */

