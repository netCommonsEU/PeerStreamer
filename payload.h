/*
 * Copyright (c) 2010-2011 Luca Abeni
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
#define VIDEO_PAYLOAD_HEADER_SIZE 1 + 2 + 2 + 2 + 2 + 1 // 1 Frame type + 2 width + 2 height + 2 frame rate num + 2 frame rate den + 1 number of frames
#define FRAME_HEADER_SIZE (3 + 4 + 1)	// 3 Frame size + 4 PTS + 1 DeltaTS

static inline void frame_header_parse(const uint8_t *data, int *size, int64_t *pts, int64_t *dts)
{
  int i;

  *size = 0;
  for (i = 0; i < 3; i++) {
    *size = *size << 8;
    *size |= data[i];
  }
  *dts = 0;
  for (i = 0; i < 4; i++) {
    *dts = *dts << 8;
    *dts |= data[3 + i];
  }
  if (data[7] != 255) {
    *pts = *dts + data[7];
  } else {
    *pts = -1;
  }
}

static inline void payload_header_parse(const uint8_t *data, uint8_t *codec, int *width, int *height, int *frame_rate_n, int *frame_rate_d)
{
  *codec = data[0];
  *width = data[1] << 8 | data[2];
  *height = data[3] << 8 | data[4];
  *frame_rate_n = data[5] << 8 | data[6];
  *frame_rate_d = data[7] << 8 | data[8];
}

static inline void payload_header_write(uint8_t *data, uint8_t codec, int width, int height, int num, int den)
{
  data[0] = codec;
  data[1] = width >> 8;
  data[2] = width & 0xFF;
  data[3] = height >> 8;
  data[4] = height & 0xFF;
  data[5] = num >> 8;
  data[6] = num & 0xFF;
  data[7] = den >> 8;
  data[8] = den & 0xFF;
}

static inline void frame_header_write(uint8_t *data, int size, int32_t pts, int32_t dts)
{
  data[0] = size >> 16;
  data[1] = size >> 8;
  data[2] = size & 0xFF;
  data[3] = dts >> 24;
  data[4] = dts >> 16;
  data[5] = dts >> 8;
  data[6] = dts & 0xFF;
  if (pts != -1) {
    data[7] = (pts - dts) & 0xFF;
  } else {
    data[7] = 255;
  }
}
