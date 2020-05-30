.PHONY: elf clean

elf: hello.o
	readelf -a $<
	nm $<

hello.o: object_hello.hxp elf.hxp
	cpp $< | hexproc -b > $@

clean:
	rm -v *.o || true
