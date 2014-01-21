#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <glib.h>
#include <glib/gstdio.h>

#define VBOX_IMG_DIR "/home/yannick/VirtualBox VMs/Win7"
#define VBOX_NAME "Win7"

#if 0
           struct stat {
               dev_t     st_dev;     /* ID of device containing file */
               ino_t     st_ino;     /* inode number */
               mode_t    st_mode;    /* protection */
               nlink_t   st_nlink;   /* number of hard links */
               uid_t     st_uid;     /* user ID of owner */
               gid_t     st_gid;     /* group ID of owner */
               dev_t     st_rdev;    /* device ID (if special file) */
               off_t     st_size;    /* total size, in bytes */
               blksize_t st_blksize; /* blocksize for file system I/O */
               blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
               time_t    st_atime;   /* time of last access */
               time_t    st_mtime;   /* time of last modification */
               time_t    st_ctime;   /* time of last status change */
           };
#endif

typedef struct {
    gchar * filename;  // full path file name
    off_t   filesize;  // size in bytes
    time_t  filemtime; // modification time
    // checksum ?
} vdi_file_t;

/* Return GSList of newly allocated (vdi_file_t *) : to be freed by caller */
GSList * dir_scan_for_vdi (const gchar * dirname, GSList * list)
{
    GSList * ret_list = list;

    GDir * dir_handle = NULL;
    const gchar * direntry = NULL;
    
    dir_handle = g_dir_open (dirname, 0/* flags */, NULL/* (GError **) */);
    if (! dir_handle) return NULL;

    while (direntry = g_dir_read_name (dir_handle)) {
        gchar * fullpath = NULL;

        fullpath = g_strdup_printf ("%s%c%s", dirname, G_DIR_SEPARATOR, direntry);
        if (!fullpath) exit (-1); // out of mem.

        if (g_file_test (fullpath, G_FILE_TEST_IS_DIR))
        {
            ret_list = dir_scan_for_vdi (fullpath, ret_list);
        }
        else if (g_file_test (fullpath, G_FILE_TEST_IS_REGULAR)
                 && g_str_has_suffix (direntry, ".vdi")) {
//                 && g_str_has_suffix (direntry, ".log")) {
            vdi_file_t * vdi_entry = g_new0 (vdi_file_t, 1);
            GStatBuf stat_buf = {0};

            if (!vdi_entry) exit (-1); // out of mem.
            if (g_stat (fullpath, &stat_buf) < 0) exit (-1); //TODO: error catch.

            vdi_entry->filename = g_strdup (fullpath);
            vdi_entry->filesize = stat_buf.st_size;
            vdi_entry->filemtime = stat_buf.st_mtime;
            ret_list = g_slist_append (ret_list, vdi_entry);
        }
        else {
            /* Nothing to do */
        }

        // dbg...

        g_free (fullpath); fullpath = NULL;
    }

    g_dir_close (dir_handle); dir_handle = NULL;

    return ret_list;
}

int main(int argc, char * argv[])
{
    struct timeval tot_start, tot_end;
    long tot_mtime, tot_secs, tot_usecs;
    long total_filesize = 0;

    GSList * file_list = NULL, * list_ptr = NULL;

//    setuid( 0 );
//    setenv( "DISPLAY", ":0", 1 );
//    system( "python ./rsync_to_iceland.py" );

    gettimeofday(&tot_start, NULL);
    file_list = dir_scan_for_vdi (VBOX_IMG_DIR, NULL);

    // DBG:
    for (list_ptr = file_list; list_ptr != NULL; list_ptr = list_ptr->next) {
        vdi_file_t * element = (gchar *) list_ptr->data;
        GDateTime *  mtime = NULL;
        GMappedFile * fdmapped = NULL;
        gchar * crc = NULL;
        // for func profiling
        struct timeval start, end;
        long _mtime, secs, usecs;    

        if (element) {
            printf ("----\n");
            // Filename
            printf ("%s\n", element->filename);

            // Filesize
            printf ("%lu KB\t%lu MB\t%lu GB\n",
                    element->filesize / 1024,
                    element->filesize / (1024*1024),
                    element->filesize / (1024*1024*1024));

            // Catch last modification time
            mtime = g_date_time_new_from_unix_local (element->filemtime);
            printf ("Last modified: %s\n", g_date_time_format (mtime, "%F %k:%M:%S (UTC%z)"));
            g_date_time_unref (mtime); mtime = NULL;

            // Compute CRC
            gettimeofday(&start, NULL);
            fdmapped = g_mapped_file_new (element->filename, FALSE /*RO access*/, NULL);
            crc = g_compute_checksum_for_data (G_CHECKSUM_MD5,
                                               g_mapped_file_get_contents (fdmapped),
                                               g_mapped_file_get_length (fdmapped));
            g_mapped_file_unref (fdmapped); fdmapped = NULL;
            gettimeofday(&end, NULL);
            secs  = end.tv_sec  - start.tv_sec;
            usecs = end.tv_usec - start.tv_usec;
            _mtime = ((secs) * 1000 + usecs/1000.0) + 0.5;
  
            printf ("CRC = %s\n", crc);
            g_free (crc); crc = NULL;

            printf("CRC computation Elapsed time: %ld millisecs, or %ld secs\n", _mtime, _mtime/1000);
            printf("CRC computation bandwidth = %ld MBytes/secs\n", element->filesize / ((_mtime/1000) *1024*1024));
            total_filesize += element->filesize;

            g_free (element->filename); element->filename = NULL;
            g_free (element);
        }
    }

    gettimeofday(&tot_end, NULL);
    tot_secs  = tot_end.tv_sec  - tot_start.tv_sec;
    tot_usecs = tot_end.tv_usec - tot_start.tv_usec;
    tot_mtime = ((tot_secs) * 1000 + tot_usecs/1000.0) + 0.5;

    printf("Total elapsed time: %ld millisecs, or %ld secs, or %ld min\n", tot_mtime, tot_mtime/1000, tot_mtime/60000);
    printf("Average computation bandwidth = %ld MBytes/secs\n", total_filesize / ((tot_mtime/1000) *1024*1024));

    return 0;

error:
    return 1;
}


