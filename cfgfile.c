#include "cfgfile.h"

#if 0
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
#endif

/*
 *
 */
vm_meta_snap_t * cfg_file_open (const gchar * filename)
{
    vm_meta_snap_t * vm = NULL;
    GKeyFile * fd = NULL;
    GError * error = NULL;

    gdouble dbl_value = 0.0;
    gint int_value = 0;
    gchar * str_value = NULL;

    gint i, j;
    gint nb_vdi = 0;
    gint nb_crc = 0;

    /* Check if file exists */
    if (! g_file_test (filename, G_FILE_TEST_IS_REGULAR)) {
        g_warning ("Filename %s not found.\n", filename);
        goto error;
    }

    /* Allocate data structure */
    vm = g_new0 (vm_meta_snap_t, 1);
    if (!vm) goto error;

    /* Parse file and fill data structure. */
    fd = g_key_file_new ();
    if (! g_key_file_load_from_file (fd, filename,
    		G_KEY_FILE_KEEP_TRANSLATIONS/*, G_KEY_FILE_NONE*/, &error)) {
        g_warning ("Invalid VM file format ('%s').\n", filename);
        g_debug ("%s", error->message);
        goto error_and_free;
    }

    //g_debug("%s", g_key_file_to_data(fd, &i, NULL/* &error */));

    // Fill structure by parsing the key-paired struct.
    dbl_value = g_key_file_get_double(fd,
									  "Global",
									  "Version",
									  &error);
//    g_printf ("Version read = %2.2f\n", dbl_value);

    int_value = g_key_file_get_integer(fd,
									  "Global",
									  "NbVdi",
									  &error);
    nb_vdi = int_value;
//    g_printf ("Nb VDI = %d\n", nb_vdi);

    for (i = 0; i < nb_vdi; i++)
    {
    	gchar * vdi_key = NULL;
    	vdi_file_t * cur = NULL;

    	//g_sprintf(vdi_key, 15, "[%06d]", i);
    	vdi_key = g_strdup_printf("F%d", i);
//    	g_printf ("looking in '%s'\n", vdi_key);

    	cur = g_new0 (vdi_file_t, 1);
    	cur->filename = g_key_file_get_string (fd,
    										   vdi_key,
											   "VdiFile",
											   &error);
    	if (error) {
    		g_debug ("%s", error->message);
    		g_error_free (error); error = NULL;
    	}
//    	if (error != NULL) {
//    		g_debug ("VdiFile catch error : %s\n", error->message);
//    	}
//    	g_printf ("%s\n", cur->filename);
    	cur->filesize = g_key_file_get_integer(fd,
    										   vdi_key,
											   "VdiSize",
											   &error);
    	if (error) {
    		g_debug ("%s", error->message);
    		g_error_free (error); error = NULL;
    	}
//    	g_printf ("%d\n", cur->filesize);
    	cur->filemtime = g_key_file_get_integer(fd,
    										   vdi_key,
											   "VdiMTim",
											   &error);
    	if (error) {
    		g_debug ("%s", error->message);
    		g_error_free (error); error = NULL;
    	}
//    	g_printf ("%d\n", cur->filemtime);

        int_value = g_key_file_get_integer(fd,
										  vdi_key,
    									  "NbCrc",
    									  &error);
        nb_crc = int_value;
    	if (error) {
    		g_debug ("%s", error->message);
    		g_error_free (error); error = NULL;
    	}
//        g_printf ("Nb CRC = %d\n", nb_crc);

        for (j = 0; j < nb_crc; j++)
        {
        	gchar crc_key[16] = {0};
        	g_snprintf(crc_key, 15, "crc[%06d]", j);

        	gchar * crc_str = NULL;

        	crc_str = g_key_file_get_string (fd,
											   vdi_key,
											   crc_key,
											   &error);
        	if (error) {
        		g_debug ("%s", error->message);
        		g_error_free (error); error = NULL;
        	}

//        	g_printf ("%s\n", crc_str);
        	cur->filecrclist = g_slist_append (cur->filecrclist, crc_str);
//        	g_free (crc_str);

        }

        g_free (vdi_key);

    	vm->vdifiles = g_slist_append(vm->vdifiles, cur);
    }

    // DEBUG
    if (0){
		gsize num_groups, num_keys;
		gchar **groups, **keys, *value;
		groups = g_key_file_get_groups(fd, &num_groups);
		for(i = 0;i < num_groups;i++)
		{
		g_printf("group %u/%u: \t'%s'\n", i, num_groups - 1, groups[i]);
		}

    }

    g_key_file_free (fd);

    return vm;

error_and_free:
    g_free (vm);

error:
    return NULL;
}

/*
 * if ok, return 0;
 */
gboolean cfg_file_write (const gchar * filename, vm_meta_snap_t * vm)
{
    FILE * fd = NULL;
    gint vdi_idx = 0, crc_idx = 0;
    GSList * vdi = NULL;
    GSList * crclist = NULL;

    /* Check if file exists */
//    if (! g_file_test (filename, G_FILE_TEST_IS_REGULAR)) {
//        g_warning ("Filename %s not found.\n", filename);
//        goto error;
//    }

    fd = g_fopen (filename, "w+");

    if (!fd) {
        g_error ("fopen() failed\n");
        goto error;
    }

    g_fprintf (fd, "# This is auto-generated file. Do Not Edit !\n");
    g_fprintf (fd, "#\n");
    g_fprintf (fd, "[Global]\n");
    g_fprintf (fd, "Version = 1.0\n");
    g_fprintf (fd, "NbVdi = %d\n", g_slist_length(vm->vdifiles));
    g_fprintf (fd, "\n");

    for (vdi = vm->vdifiles, vdi_idx = 0;
    	 vdi != NULL;
         vdi = g_slist_next(vdi), vdi_idx ++) {
    	vdi_file_t * file = (vdi_file_t *) vdi->data;
    	GDateTime *  mtime = NULL;

    	g_fprintf (fd, "[F%d]\n", vdi_idx);
        g_fprintf (fd, "VdiFile = %s\n", file->filename);
        g_fprintf (fd, "VdiSize = %ld\n", file->filesize);

        mtime = g_date_time_new_from_unix_local (file->filemtime);
        g_fprintf (fd, "VdiMStr = %s\n", g_date_time_format (mtime, "%F %k:%M:%S (UTC%z)"));
        g_fprintf (fd, "VdiMTim = %ld\n", file->filemtime);
        g_fprintf (fd, "NbCrc = %u\n", g_slist_length(file->filecrclist));

        for (crclist = file->filecrclist, crc_idx = 0;
        	 crclist != NULL;
        	 crclist = g_slist_next(crclist), crc_idx ++) {
       	    g_fprintf (fd, "crc[%06d] = %s\n", crc_idx, (gchar *) crclist->data);
        }
        g_fprintf (fd, "\n");
    }

    fclose (fd);

    return FALSE;

error:
    return TRUE;    
}


