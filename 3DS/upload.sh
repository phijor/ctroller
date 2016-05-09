#/usr/bin/env bash

if [ $# -ne 2 ]; then
    echo "Usage:"
    echo "  $(basename $0) host application"
    exit 1
fi

HOST=$1
TARGET=$2
DSPORT=5000
HOMEBREWDIR="/3ds"

ftp -i $HOST $DSPORT > /dev/null <<EOF
anonymous

mdelete ${HOMEBREWDIR}/${TARGET}
rmdir  ${HOMEBREWDIR}/${TARGET}

mkdir ${HOMEBREWDIR}/${TARGET}

put ${TARGET}.3dsx ${HOMEBREWDIR}/${TARGET}/${TARGET}.3dsx
put ${TARGET}.smdh ${HOMEBREWDIR}/${TARGET}/${TARGET}.smdh
EOF
