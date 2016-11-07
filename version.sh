#!/bin/bash
#________________________________________
# Copyright (c) 2011 Gianluca Ciccarelli.
#
# Usage: version.sh
#_________________________________________

set -e

prefix="../THIRDPARTY-LIBS"
GRAPES_VERSION=$(
  cd $prefix/GRAPES; git rev-parse HEAD
)

FFMPEG_VERSION=$(cat $prefix/ffmpeg/version.h |\
  grep '^#define FFMPEG_VERSION' |\
  sed 's/.*"\(.*\)"$/\1/'
)

LIBOGG_VERSION=$(cat $prefix/libogg/config.h |\
  grep '^#define VERSION' |\
  sed 's/.*"\(.*\)"/\1/'
)

LIBVORBIS_VERSION=$(cat $prefix/libvorbis/config.h |\
  grep '^#define VERSION' |\
  sed 's/.*"\(.*\)"/\1/'
)

MP3LAME_VERSION=$(cat $prefix/mp3lame/config.h |\
  grep '^#define VERSION' |\
  sed 's/.*"\(.*\)"/\1/'
)

X264_VERSION=$(cat $prefix/x264/config.h |\
  grep '^#define X264_VERSION' |\
  sed 's/.*"\(.*\)"/\1/'
)

prefix="../THIRDPARTY-LIBS/NAPA-BASELIBS/3RDPARTY-LIBS"
LIBCONFUSE_VERSION=$(
  find $prefix/libconfuse/_src/ -maxdepth 1 -name 'confuse*' |\
    xargs basename |\
    sed 's/confuse-\(.*\)/\1/'
)

LIBEVENT_VERSION=$(
  find $prefix/libevent/_src/ -maxdepth 1 -name 'libevent*' |\
    xargs basename |\
    sed 's/libevent-\(.*\)/\1/'
)

LIBXML2_VERSION=$(
  find $prefix/libxml2/_src/ -maxdepth 1 -name 'libxml*' |\
    xargs basename |\
    sed 's/libxml2-\(.*\)/\1/'
)

prefix="../THIRDPARTY-LIBS/NAPA-BASELIBS/"
NAPA_BASELIBS_VERSION=$(
  cd $prefix; git rev-parse HEAD
)


echo FFMPEG_VERSION          $FFMPEG_VERSION
echo LIBOGG_VERSION          $LIBOGG_VERSION
echo LIBVORBIS_VERSION       $LIBVORBIS_VERSION
echo MP3LAME_VERSION         $MP3LAME_VERSION
echo X264_VERSION            $X264_VERSION
echo LIBCONFUSE_VERSION      $LIBCONFUSE_VERSION
echo LIBEVENT_VERSION        $LIBEVENT_VERSION
echo LIBXML2_VERSION         $LIBXML2_VERSION
echo NAPA_BASELIBS_VERSION   $NAPA_BASELIBS_VERSION
echo GRAPES_VERSION          $GRAPES_VERSION 

cat > version.h <<ENDOFFILE
#ifndef VERSION_H
#define VERSION_H

#define FFMPEG_VERSION         "$FFMPEG_VERSION"
#define LIBOGG_VERSION         "$LIBOGG_VERSION"
#define LIBVORBIS_VERSION      "$LIBVORBIS_VERSION"
#define MP3LAME_VERSION        "$MP3LAME_VERSION"
#define X264_VERSION           "$X264_VERSION"
#define LIBCONFUSE_VERSION     "$LIBCONFUSE_VERSION"
#define LIBEVENT_VERSION       "$LIBEVENT_VERSION"
#define LIBXML2_VERSION        "$LIBXML2_VERSION"
#define NAPA_BASELIBS_VERSION  "$NAPA_BASELIBS_VERSION"
#define GRAPES_VERSION         "$GRAPES_VERSION"

#endif /* VERSION_H */
ENDOFFILE
