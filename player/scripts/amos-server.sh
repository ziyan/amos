#!/bin/sh
SCRIPT_DIR=`dirname $0`
killall -9 festival 2>&1 > /dev/null || true
player -d6 -p7000 $SCRIPT_DIR/../configs/amos/amos.cfg
