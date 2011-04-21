#!/bin/sh
SCRIPT_DIR=`dirname $0`

# flush map from server
amosmaptool flush

# remove old map db
rm -f $SCRIPT_DIR/../configs/igvcgps/2010/map.db

# create a map
amosmaptool create $SCRIPT_DIR/../configs/igvcgps/2010/map.db 0.2

# load map onto server
amosmaptool load $SCRIPT_DIR/../configs/igvcgps/2010/map.db

# put bound down
sh $SCRIPT_DIR/../configs/igvcgps/2010/$1_box.sh

# player server
player -d9 -p8000 $SCRIPT_DIR/../configs/igvcgps/igvcgps.cfg 2>&1 | grep apf
