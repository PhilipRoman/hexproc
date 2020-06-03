# cd to repository root if necessary
if [ -f 'test.sh' ]; then
	cd ..
fi

if [ ! -f 'build/linux/hexproc' ]; then
	echo "Error: build/linux/hexproc executable not found"
	exit 2
fi

expect() {
	#  $1 is input
	#  $2 is expected output
	if [ "$2" != "$(echo "$1" | build/linux/hexproc)" ]; then
		echo '============================'
		echo "Assertion failed: $1 != $2"
		exit 1
	fi
}

echo 'Testing simple octets'
expect '11 22 33' '11 22 33'
expect 'aa bb   cc' 'aa bb cc'

echo 'Testing string literals'
expect '"Hello"' '48 65 6c 6c 6f'
expect '"Hel"   "lo"' '48 65 6c 6c 6f'

echo 'Testing formatters'
expect '[int](1 + 2)' '00 00 00 03'
expect '[int,LE](3 * 4 - 1)' '0b 00 00 00'
expect '[3,  int]3' '00 00 03'

echo 'Testing variables'
expect 'cc cc cc a: [byte]a' 'cc cc cc 03'
expect 'cc cc a: cc a: [byte]a' 'cc cc cc 03'
expect 'a := 1; b = a; a := 2; [byte]b' '02'
expect 'a := 1; b := a; a := 2; [byte]b' '01'
expect 'a = 1; a := a + 1; [byte]a' '02'

echo 'Testing endian configuration'
expect '[short]1 hexproc.endian := LE; [short]1' '00 01 01 00'
expect '[short]1 hexproc.endian := LE; [short]1  hexproc.endian := BE; [short]1' '00 01 01 00 00 01'

echo '===================='
echo 'All tests succeeded!'
