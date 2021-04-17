#!/bin/bash
if [[ $# -lt 2 ]]; then
	echo "Usage: ./add_nanomites.sh input_elf output_elf"
	exit 2
fi

input_elf=$1
output_elf=$2

python3 src/prepare_packer.py $input_elf	#add encrypted nanomites to ELF
xxd -i resc/nanomites_dump src/nanomites_dump.h	#transform nanomites_dump into C header file
xxd -i resc/nanomites_encrypted src/nanomites_encrypted.h	#transform nanomites_encrypted into C header file
gcc src/packer.c -lssl -lcrypto -o $output_elf		#compile all resources into single executable so it can run everywhere
