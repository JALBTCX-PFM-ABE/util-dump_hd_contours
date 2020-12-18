
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.

*********************************************************************************************/

#include <time.h>

#include "nvutility.h"

#include "pfm.h"

#include "llz.h"

#include "version.h"


int main (int argc, char *argv[])
{
  int32_t              pfm_hnd = -1, llz_hnd = -1, last, list, file_number = -1, row, col, rec, recnum, percent = 0, old_percent = -1, total, count = 0;
  NV_I32_COORD2        coord;
  int16_t              type;
  PFM_OPEN_ARGS        open_args;
  BIN_RECORD           bin;
  DEPTH_RECORD         *depth;
  char                 llz_file[1024], input_file[1024];
  LLZ_REC              llz_rec;
  LLZ_HEADER           llz_header;


  printf ("\n\n %s \n\n", VERSION);


  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s PFM_FILE_NAME [OUTPUT_FILE_NAME]\n\n", argv[0]);
      fprintf (stderr, "Where:\n");
      fprintf (stderr, "\tPFM_FILE_NAME is a PFM handle file for a PFM that contains\n");
      fprintf (stderr, "\t\thand-drawn contours (must have been MISPed).\n");
      fprintf (stderr, "\tOUTPUT_FILE_NAME is an optional output LLZ file name.\n");
      fprintf (stderr, "\t\tIf OUTPUT_FILE_NAME is not supplied it will be set to\n");
      fprintf (stderr, "\t\tPFM_FILE_NAME.llz.\n\n");
      exit (-1);
    }


  /* Process the input file on the command line. */

  strcpy (open_args.list_path, argv[1]);
  open_args.checkpoint = 0;
  if ((pfm_hnd = open_existing_pfm_file (&open_args)) < 0) pfm_error_exit (pfm_error);


  if (argc < 3)
    {
      strcpy (llz_file, open_args.list_path);
      strcpy (&llz_file[strlen (llz_file) - 4], ".llz");
    }
  else
    {
      strcpy (llz_file, argv[2]);
      if (strcmp (&llz_file[strlen (llz_file) - 4], ".llz")) strcat (llz_file, ".llz");
    }


  /*  Boilerplate LLZ header for contour data.  */

  sprintf (llz_header.source, "Derived from %s", gen_basename (open_args.list_path));
  llz_header.time_flag = NVFalse;
  llz_header.uncertainty_flag = NVFalse;
  llz_header.depth_units = 0;


  if ((llz_hnd = create_llz (llz_file, llz_header)) < 0)
    {
      perror (llz_file);
      exit (-1);
    }


  /*  Find the hand-drawn contour pseudo file number.  */

  last = get_next_list_file_number (pfm_hnd);

  for (list = 0 ; list < last ; list++)
    {
      read_list_file (pfm_hnd, list, input_file, &type);

      if (!strcmp (input_file, "pfmView_hand_drawn_contour"))
        {
          file_number = list;
          break;
        }
    }


  if (file_number < 0)
    {
      fprintf (stderr, "\n\nNo hand-drawn contours in file %s\n\n", open_args.list_path);
      exit (-1);
    }


  total = open_args.head.bin_width * open_args.head.bin_height;


  for (row = 0 ; row < open_args.head.bin_height ; row++)
    {
      coord.y = row;

      for (col = 0 ; col < open_args.head.bin_width ; col++)
        {
          coord.x = col;

          if (!read_bin_record_index (pfm_hnd, coord, &bin))
            {
              if (bin.num_soundings)
                {
                  if (!read_depth_array_index (pfm_hnd, coord, &depth, &recnum))
                    {
                      for (rec = 0 ; rec < recnum ; rec++)
                        {
                          if (!(depth[rec].validity & (PFM_INVAL | PFM_DELETED | PFM_REFERENCE)) && depth[rec].file_number == file_number)
                            {
                              count++;

                              llz_rec.xy.lat = depth[rec].xyz.y;
                              llz_rec.xy.lon = depth[rec].xyz.x;
                              llz_rec.depth = depth[rec].xyz.z;
                              llz_rec.status = 0;

                              append_llz (llz_hnd, llz_rec);
                            }
                        }

                      free (depth);
                    }
                }
            }

          percent = NINT (((float) (row * open_args.head.bin_width + col) / (float) (total)) * 100.0);
          if (percent != old_percent)
            {
              printf ("%03d%% processed\r", percent);
              fflush (stdout);
              old_percent = percent;
            }
        }
    }


  printf ("%d records output to LLZ file %s\n\n", count, llz_file);


  close_llz (llz_hnd);
  close_pfm_file (pfm_hnd);

  return (0);
}
