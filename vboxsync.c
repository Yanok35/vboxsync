#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "cfgfile.h"

#define VBOX_IMG_DIR "/home/yannick/VirtualBox VMs/WinXP"

/* Big file chunk size is 640 KB */
#define CHUNK_SIZE (655360)


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
//                && (g_str_has_suffix (direntry, ".vdi") || g_str_has_suffix (direntry, ".vbox"))) {
        		&& g_str_has_suffix (direntry, ".vdi")) {
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

/* TODO */
GSList * file_scan_for_crc_list (vdi_file_t * vdi, gulong chunksize)
{
    gint i;
    GSList * crclist = NULL;

    GMappedFile * fdmapped = NULL;
    gchar * fptr = NULL;

    gchar * crc = NULL;
    gulong remain = 0;
    gulong nb_chunk = 0;

    /* mmap file */
    fdmapped = g_mapped_file_new (vdi->filename, FALSE /*RO access*/, NULL);
    fptr = g_mapped_file_get_contents (fdmapped);

    remain = vdi->filesize;
    nb_chunk = vdi->filesize / chunksize;

    printf ("filesize = %ld\n", vdi->filesize);
    printf ("chunksize = %ld\n", chunksize);
    printf ("nb_chunk = %ld\n", nb_chunk);

    for (i = 0; i < nb_chunk; i++)
    {
        crc = g_compute_checksum_for_data (G_CHECKSUM_MD5,
                                           &fptr[chunksize * i],
                                           chunksize);

        remain -= chunksize;

        crclist = g_slist_append (crclist, crc);
//        printf ("crc[%06d] = %s\n", i, crc);
    }

    printf ("bytes remain = %ld\n", remain);

    if (remain)
    {
        crc = g_compute_checksum_for_data (G_CHECKSUM_MD5,
                                           &fptr[chunksize * nb_chunk],
                                           remain);
//        printf ("crc[%06ld] = %s\n", nb_chunk, crc);
        crclist = g_slist_append (crclist, crc);
    }

    printf ("length (crclist) = %u\n", g_slist_length (crclist));

    g_mapped_file_unref (fdmapped); fdmapped = NULL;

    //exit (0);
    return crclist;
}


/* ************************ */
/* *** Main entry point *** */
/* ************************ */

int main(int argc, char * argv[])
{
    struct timeval tot_start, tot_end;
    long tot_mtime, tot_secs, tot_usecs;
    long total_filesize = 0;
    vm_meta_snap_t * vm = NULL;

    GSList * file_list = NULL, * list_ptr = NULL;

    //VBOX_IMG_DIR
    if (0){
        vm_meta_snap_t _vm = {
        		"test.cfg", NULL,
        };
        vm_meta_snap_t * vm_out = NULL;

        vdi_file_t f[] = {
    			{ "file1.vdi", 100, 1, NULL },
    			{ "file2.vdi",  50, 2, NULL },
    			{ "file3.vdi",  60, 3, NULL },
    	};
		vm = &_vm;
		vm->vdifiles = g_slist_append(vm->vdifiles, &f[0]);
		vm->vdifiles = g_slist_append(vm->vdifiles, &f[1]);
		vm->vdifiles = g_slist_append(vm->vdifiles, &f[2]);

		f[0].filecrclist = g_slist_append(f[0].filecrclist,"932f7dca53f2f79a1a0e445c58d6db8e");
		f[1].filecrclist = g_slist_append(f[1].filecrclist,"12341234123412341234123412341234");
		f[0].filecrclist = g_slist_append(f[0].filecrclist,"12341234123412341234123412341234");
		f[1].filecrclist = g_slist_append(f[1].filecrclist,"932f7dca53f2f79a1a0e445c58d6db8e");

		if (cfg_file_write ("test.cfg", vm))
		{
			//FIXME error catch.
		}

		vm_out = cfg_file_open ("test.cfg");
		cfg_file_write ("test_out.cfg", vm_out);
	    exit (0);
    }

//    setuid( 0 );
//    setenv( "DISPLAY", ":0", 1 );
//    system( "python ./rsync_to_iceland.py" );

    vm = g_new0 (vm_meta_snap_t, 1);
    vm->cfgfile = g_strdup (VBOX_IMG_DIR);

    gettimeofday(&tot_start, NULL);
    file_list = dir_scan_for_vdi (VBOX_IMG_DIR, NULL);
    vm->vdifiles = file_list;

    g_printf ("nb vdi file to scan = %d\n", g_slist_length(vm->vdifiles));
    guint nb_chunk_total = 0;
    for (list_ptr = file_list;
    		list_ptr != NULL;
    		list_ptr = g_slist_next(list_ptr)) {
    	guint nb_chunk = 0;
    	vdi_file_t * vdi = (vdi_file_t *) list_ptr->data;

    	nb_chunk = vdi->filesize / CHUNK_SIZE;
    	nb_chunk++; // round it up if remainings bytes...
    	nb_chunk_total += nb_chunk;
    }
    g_printf ("nb_chunk_total = %d\n", nb_chunk_total);

//    exit (0);

    // DBG:
    for (list_ptr = file_list;
    		list_ptr != NULL;
    		list_ptr = g_slist_next(list_ptr)) {
        vdi_file_t * element = (vdi_file_t *) list_ptr->data;
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
#if 0
            crc = g_compute_checksum_for_data (G_CHECKSUM_MD5,
                                               g_mapped_file_get_contents (fdmapped),
                                               g_mapped_file_get_length (fdmapped));
#else
            // md5 crc string example: "932f7dca53f2f79a1a0e445c58d6db8e"
            element->filecrclist = file_scan_for_crc_list (element, CHUNK_SIZE);
            //crc = g_strdup ("12341234123412341234123412341234");
#endif
            g_mapped_file_unref (fdmapped); fdmapped = NULL;
            gettimeofday(&end, NULL);
            secs  = end.tv_sec  - start.tv_sec;
            usecs = end.tv_usec - start.tv_usec;
            _mtime = ((secs) * 1000 + usecs/1000.0) + 0.5;
  
            //printf ("CRC = %s\n", crc);
            //g_free (crc); crc = NULL;

            printf("CRC computation Elapsed time: %ld millisecs, or %ld secs\n", _mtime, _mtime/1000);
            if (_mtime) printf("CRC computation bandwidth = %ld MBytes/secs\n", element->filesize / ((_mtime/1000) *1024*1024));
            total_filesize += element->filesize;

//            g_free (element->filename); element->filename = NULL;
//            g_free (element);
        }
    }

    gettimeofday(&tot_end, NULL);
    tot_secs  = tot_end.tv_sec  - tot_start.tv_sec;
    tot_usecs = tot_end.tv_usec - tot_start.tv_usec;
    tot_mtime = ((tot_secs) * 1000 + tot_usecs/1000.0) + 0.5;

    printf("Total elapsed time: %ld millisecs, or %ld secs, or %ld min\n", tot_mtime, tot_mtime/1000, tot_mtime/60000);
    if (tot_mtime) printf("Average computation bandwidth = %ld MBytes/secs\n", total_filesize / ((tot_mtime/1000) *1024*1024));

    cfg_file_write("VBox.cfg", vm);

    return 0;

error:
    return 1;
}


