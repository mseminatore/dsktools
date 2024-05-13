# /usr/bin/bash

DSKPATH=$(dirname $0)
cd "$DSKPATH"
DSKPATH=$(pwd)

cd /usr/local/bin

ln -sf "$DSKPATH/dsktools" dsktools
ln -sf "$DSKPATH/dsk_new" dsk_new
ln -sf "$DSKPATH/dsk_add" dsk_add
ln -sf "$DSKPATH/dsk_del" dsk_del
