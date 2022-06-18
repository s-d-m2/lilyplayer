#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>

#include "filename_to_catch.h"
#include <errno.h>

// fluidsynth loads a soundfile from the filesystem. However, here we embed the soundfile
// directly in the app. Therefore, let's wrap fopen such that when fluidsynth tries to
// open the soundfile, we return a FILE* on the the buffer instead.
// fluidsynth also checks if the file exists and is a regular file using g_file_test.
// We also need to wrap that.

#if defined(__clang__)
// clang complains that names starting by __ like __real_fopen or starting by _ at global scope
// like _binary_Yamaha_Grand_Lite_v2_0_sf2_start are reserved. We don't have the choice though,
// these are the symbols we are looking for.
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif

extern FILE* __real_fopen(const char *restrict pathname, const char *restrict mode);
extern char _binary_Yamaha_Grand_Lite_v2_0_sf2_start;
extern char _binary_Yamaha_Grand_Lite_v2_0_sf2_end;

// following code is copy/pasted from `man fopencookie`
struct memfile_cookie_t
{
    char* buf;
    size_t buf_size;
    off_t offset;
};

static ssize_t memfile_write(void * c __attribute__((unused)),
			     const char * buf  __attribute__((unused)),
			     size_t  size  __attribute__((unused)))
{
  errno = EPERM;
  return -1;
}

static ssize_t memfile_read(void *c, char *buf, size_t size) {
  ssize_t xbytes;
  struct memfile_cookie_t *cookie = c;

  /*   Fetch  minimum   of  bytes   requested  and   bytes
       available. */

  xbytes = (ssize_t)size;
  if (cookie->offset + (ssize_t)size > (ssize_t)cookie->buf_size)
    xbytes = (ssize_t)(cookie->buf_size - (size_t)cookie->offset);
  if (xbytes < 0)     /* offset may be past endpos */
    xbytes = 0;

  memcpy(buf, cookie->buf + cookie->offset, (size_t)xbytes);

  cookie->offset += xbytes;
  return xbytes;
}

static int memfile_seek(void *c, off64_t *offset, int whence) {
  off64_t new_offset;
  struct memfile_cookie_t *cookie = c;

  if (whence == SEEK_SET)
    new_offset = *offset;
  else if (whence == SEEK_END)
    new_offset = (off64_t)(cookie->buf_size + (size_t)*offset);
  else if (whence == SEEK_CUR)
    new_offset = cookie->offset + *offset;
  else
    return -1;

  if (new_offset < 0)
    return -1;

  cookie->offset = new_offset;
  *offset = new_offset;
  return 0;
}

static int memfile_close(void *c) {
  struct memfile_cookie_t *cookie = c;

  cookie->buf = NULL;
  cookie->buf_size = 0;
  cookie->offset = -1;

  return 0;
}


static cookie_io_functions_t memfile_func = {
  .read  = memfile_read,
  .write = memfile_write,
  .seek  = memfile_seek,
  .close = memfile_close
};

static struct memfile_cookie_t memfile_cookie;


FILE * __wrap_fopen(const char *restrict pathname, const char *restrict mode);

FILE * __wrap_fopen(const char *restrict pathname, const char *restrict mode)
{
  if ((pathname == soundfile_filename) || (strcmp(pathname, soundfile_filename) == 0)) {
    size_t size = (size_t)(&_binary_Yamaha_Grand_Lite_v2_0_sf2_end - &_binary_Yamaha_Grand_Lite_v2_0_sf2_start);
    memfile_cookie.buf = &_binary_Yamaha_Grand_Lite_v2_0_sf2_start;
    memfile_cookie.buf_size = size;
    memfile_cookie.offset = 0;

    fprintf(stderr, "wrapping file [%s] -> returning soundfile from memory with mode %s\n", pathname, mode);
    return fopencookie(&memfile_cookie, "r", memfile_func);

  } else {
    fprintf(stderr, "wrapping file [%s] -> returning the real thing\n", pathname);
    return __real_fopen(pathname, mode);
  }
}


gboolean __wrap_g_file_test(const gchar* filename, GFileTest cond_to_check);
extern gboolean __real_g_file_test(const gchar* filename, GFileTest cond_to_check);

gboolean __wrap_g_file_test(const gchar* filename, GFileTest cond_to_check) {
  fprintf(stderr, "wrapping gfiletest [%s], cond to check is %u\n", filename, cond_to_check);
  if ((filename == soundfile_filename) || (strcmp(filename, soundfile_filename) == 0)) {
    return 1; // true
  } else {
    return __real_g_file_test(filename, cond_to_check);
  }
}
