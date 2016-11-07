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
#ifndef CHUNKBUFFER_HELPERS_H
#define CHUNKBUFFER_HELPERS_H

#include <chunkbuffer.h>

inline struct chunk_buffer *cb_init(const char *config) {
  return chbInit(config);
}

inline int cb_add_chunk(struct chunk_buffer *cb, const struct chunk *c) {
  return chbAddChunk(cb, c);
}

inline struct chunk *cb_get_chunks(const struct chunk_buffer *cb, int *n) {
 return chbGetChunks(cb, n);
}

inline int cb_clear(struct chunk_buffer *cb) {
 return chbClear(cb);
}

inline const struct chunk *cb_get_chunk(const struct chunk_buffer *cb, int id) {
  return chbGetChunk(cb, id);
}

#endif	/* CHUNKBUFFER_HELPERS_H */
