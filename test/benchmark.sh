#!/usr/bin/env bash

# cd to repository root if necessary
if [ -f 'test.sh' ]; then
  cd ..
fi

if [ ! -f 'build/linux/hexproc' ]; then
	echo "Error: build/linux/hexproc executable not found"
	exit 2
fi

ntimes=5
nbytes=200000

echo "Running Hexproc with $nbytes octets of input $ntimes times"

for i in `seq $ntimes`; do
	time -p dd bs=1 count=$nbytes if=/dev/zero | xxd -p | build/linux/hexproc > /dev/null
done
