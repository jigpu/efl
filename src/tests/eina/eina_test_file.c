/* EINA - EFL data type library
 * Copyright (C) 2008 Cedric Bail
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "eina_suite.h"
#include "Eina.h"
#include "eina_safety_checks.h"

static int default_dir_rights = 0777;

#ifdef EINA_SAFETY_CHECKS
struct log_ctx {
   const char *msg;
   const char *fnc;
   Eina_Bool did;
};

/* tests should not output on success, just uncomment this for debugging */
//#define SHOW_LOG 1

static void
_eina_test_safety_print_cb(const Eina_Log_Domain *d, Eina_Log_Level level, const char *file, const char *fnc, int line, const char *fmt, void *data, va_list args EINA_UNUSED)
{
   struct log_ctx *ctx = data;
   va_list cp_args;
   const char *str;

   va_copy(cp_args, args);
   str = va_arg(cp_args, const char *);
   va_end(cp_args);

   ck_assert_int_eq(level, EINA_LOG_LEVEL_ERR);
   ck_assert_str_eq(fmt, "%s");
   ck_assert_str_eq(ctx->msg, str);
   ck_assert_str_eq(ctx->fnc, fnc);
   ctx->did = EINA_TRUE;

#ifdef SHOW_LOG
   eina_log_print_cb_stderr(d, level, file, fnc, line, fmt, NULL, args);
#else
   (void)d;
   (void)file;
   (void)line;
#endif
}
#endif


START_TEST(eina_file_split_simple)
{
   Eina_Array *ea;

   eina_init();

#ifdef EINA_SAFETY_CHECKS
#ifdef SHOW_LOG
   fprintf(stderr, "you should have a safety check failure below:\n");
#endif
   struct log_ctx ctx;

#define TEST_MAGIC_SAFETY(fn, _msg)             \
   ctx.msg = _msg;                              \
   ctx.fnc = fn;                                \
   ctx.did = EINA_FALSE

   eina_log_print_cb_set(_eina_test_safety_print_cb, &ctx);

   TEST_MAGIC_SAFETY("eina_file_split", "safety check failed: path == NULL");
   ea = eina_file_split(NULL);
   fail_if(ea);
   fail_if(eina_error_get() != EINA_ERROR_SAFETY_FAILED);
   fail_unless(ctx.did);

   eina_log_print_cb_set(eina_log_print_cb_stderr, NULL);
#undef TEST_MAGIC_SAFETY
#endif

#ifdef _WIN32
   ea = eina_file_split(strdup("\\this\\is\\a\\small\\test"));
#else
   ea = eina_file_split(strdup("/this/is/a/small/test"));
#endif

   fail_if(!ea);
   fail_if(eina_array_count(ea) != 5);
   fail_if(strcmp(eina_array_data_get(ea, 0), "this"));
   fail_if(strcmp(eina_array_data_get(ea, 1), "is"));
   fail_if(strcmp(eina_array_data_get(ea, 2), "a"));
   fail_if(strcmp(eina_array_data_get(ea, 3), "small"));
   fail_if(strcmp(eina_array_data_get(ea, 4), "test"));

   eina_array_free(ea);

#ifdef _WIN32
   ea =
      eina_file_split(strdup(
                         "this\\\\is\\\\\\a \\more\\complex\\\\\\case\\\\\\"));
#else
   ea = eina_file_split(strdup("this//is///a /more/complex///case///"));
#endif

   fail_if(!ea);
   fail_if(eina_array_count(ea) != 6);
   fail_if(strcmp(eina_array_data_get(ea, 0), "this"));
   fail_if(strcmp(eina_array_data_get(ea, 1), "is"));
   fail_if(strcmp(eina_array_data_get(ea, 2), "a "));
   fail_if(strcmp(eina_array_data_get(ea, 3), "more"));
   fail_if(strcmp(eina_array_data_get(ea, 4), "complex"));
   fail_if(strcmp(eina_array_data_get(ea, 5), "case"));

   eina_array_free(ea);

   eina_shutdown();
}
END_TEST

Eina_Tmpstr*
get_full_path(const char* tmpdirname, const char* filename)
{
    char full_path[PATH_MAX] = "";
    eina_str_join(full_path, sizeof(full_path), '/', tmpdirname, filename);
    return eina_tmpstr_add(full_path);
}

Eina_Tmpstr*
get_eina_test_file_tmp_dir()
{
   Eina_Tmpstr *tmp_dir;

   Eina_Bool created = eina_file_mkdtemp("EinaFileTestXXXXXX", &tmp_dir);

   if (!created)
     {
        return NULL;
     }

   return tmp_dir;
}

START_TEST(eina_file_direct_ls_simple)
{
   eina_init();

   const char *good_dirs[] =
     {
        "eina_file_direct_ls_simple_dir",
        "a.",
        "$a$b",
        "~$a@:-*$b!{}"
     };
   const int good_dirs_count = sizeof(good_dirs) / sizeof(const char *);
   Eina_Tmpstr *test_dirname = get_eina_test_file_tmp_dir();
   fail_if(test_dirname == NULL);

   for (int i = 0; i != good_dirs_count; ++i)
     {
        Eina_Tmpstr *dirname = get_full_path(test_dirname, good_dirs[i]);
        // clean old test directories
        rmdir(dirname);
        fail_if(mkdir(dirname, default_dir_rights) != 0);

        Eina_File_Direct_Info *dir_info;
        Eina_Iterator *it = eina_file_direct_ls(test_dirname);
        Eina_Bool found_dir = EINA_FALSE;

        while (eina_iterator_next(it, (void **)&dir_info))
          {
             if (!strcmp(dir_info->path, dirname))
               {
                  found_dir = EINA_TRUE;
               }
          }

        eina_iterator_free(it);

        fail_if(!found_dir);
        fail_if(rmdir(dirname) != 0);

        eina_tmpstr_del(dirname);
     }
   fail_if(rmdir(test_dirname) != 0);
   eina_tmpstr_del(test_dirname);
   eina_shutdown();
}
END_TEST

START_TEST(eina_file_ls_simple)
{
   eina_init();

   const char *good_dirs[] =
     {
        "eina_file_ls_simple_dir",
        "b.",
        "$b$a",
        "~$b@:-*$a!{}"
     };
   const int good_dirs_count = sizeof(good_dirs) / sizeof(const char *);
   Eina_Tmpstr *test_dirname = get_eina_test_file_tmp_dir();
   fail_if(test_dirname == NULL);

   for (int i = 0; i != good_dirs_count; ++i)
     {
        Eina_Tmpstr *dirname = get_full_path(test_dirname, good_dirs[i]);
        // clean old test directories
        rmdir(dirname);
        fail_if(mkdir(dirname, default_dir_rights) != 0);

        char *filename;
        Eina_Iterator *it = eina_file_ls(test_dirname);
        Eina_Bool found_dir = EINA_FALSE;

        while (eina_iterator_next(it, (void **)&filename))
          {
             if (!strcmp(filename, dirname))
               {
                  found_dir = EINA_TRUE;
               }
          }

        eina_iterator_free(it);

        fail_if(!found_dir);
        fail_if(rmdir(dirname) != 0);

        eina_tmpstr_del(dirname);
     }
   fail_if(rmdir(test_dirname) != 0);
   eina_tmpstr_del(test_dirname);
   eina_shutdown();
}
END_TEST

void
eina_test_file(TCase *tc)
{
   tcase_add_test(tc, eina_file_split_simple);
   tcase_add_test(tc, eina_file_direct_ls_simple);
   tcase_add_test(tc, eina_file_ls_simple);
}

