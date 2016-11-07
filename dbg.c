/*
 * Copyright (c) 2010-2011 Luca Abeni
 * Copyright (c) 2010-2011 Csaba Kiraly
 *
 * This file is part of PeerStreamer.
 *
 * PeerStreamer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PeerStreamer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PeerStreamer.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>

#include "dbg.h"

static struct timeval tnow;

int ftprintf(FILE *stream, const char *format, ...)
{
  va_list ap;
  int ret;
  
  gettimeofday(&tnow, NULL);
  fprintf(stream, "%ld.%03ld ", tnow.tv_sec, tnow.tv_usec/1000);

  va_start (ap, format);
  ret = vfprintf(stderr, format, ap);
  va_end (ap);
  
  return ret;
}
