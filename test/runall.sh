#!/bin/bash

WDIR="$(dirname $0)"

for testfile in $(ls $WDIR/*.test); do
	$testfile
done
