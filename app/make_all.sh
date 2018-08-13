#!/bin/bash

echo "You must ensure no variables are set in the makefile"
set -ex

VERSION=v3.5.3

make -j12 ANEM_TYPE=ROOM_TYPE READ_OFFSET=0
cp bin/hamilton/hamilton_3c_qfw.bin binaries/room_4_offset_0_${VERSION}.bin

make -j12 ANEM_TYPE=DUCT_TYPE READ_OFFSET=12
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_4_offset_12_${VERSION}.bin
make -j12 ANEM_TYPE=DUCT_TYPE READ_OFFSET=16
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_4_offset_16_${VERSION}.bin
make -j12 ANEM_TYPE=DUCT_TYPE READ_OFFSET=20
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_4_offset_20_${VERSION}.bin
make -j12 ANEM_TYPE=DUCT_TYPE READ_OFFSET=24
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_4_offset_24_${VERSION}.bin
make -j12 ANEM_TYPE=DUCT_TYPE READ_OFFSET=28
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_4_offset_28_${VERSION}.bin
make -j12 ANEM_TYPE=DUCT_TYPE READ_OFFSET=32
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_4_offset_32_${VERSION}.bin

make -j12 ANEM_TYPE=DUCT6_TYPE READ_OFFSET=12
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_6_offset_12_${VERSION}.bin
make -j12 ANEM_TYPE=DUCT6_TYPE READ_OFFSET=16
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_6_offset_16_${VERSION}.bin
make -j12 ANEM_TYPE=DUCT6_TYPE READ_OFFSET=20
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_6_offset_20_${VERSION}.bin
make -j12 ANEM_TYPE=DUCT6_TYPE READ_OFFSET=24
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_6_offset_24_${VERSION}.bin
make -j12 ANEM_TYPE=DUCT6_TYPE READ_OFFSET=28
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_6_offset_28_${VERSION}.bin
make -j12 ANEM_TYPE=DUCT6_TYPE READ_OFFSET=32
cp bin/hamilton/hamilton_3c_qfw.bin binaries/duct_6_offset_32_${VERSION}.bin
