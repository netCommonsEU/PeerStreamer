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
#ifndef INPUT_STREAM_H
#define INPUT_STREAM_H

struct input_stream;

struct input_stream *input_stream_open(const char *fname, int *period, uint16_t flags);
void input_stream_close(struct input_stream *dummy);
uint8_t *chunkise(struct input_stream *dummy, int id, int *size, uint64_t *ts);

#endif	/* INPUT_STREAM_H */
