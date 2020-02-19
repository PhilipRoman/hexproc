.PHONY: java elf clean

JvmHello.class: jvm_hello.hxp classfile.hxp
	cpp $< | hexproc -b > $@

java: JvmHello.class
	java -version
	javap -verbose $<
	java JvmHello

elf: hello.o
	readelf -a $<

hello.o: object_hello.hxp elf.hxp
	cpp $< | hexproc -b > $@

clean:
	rm -v *.class || true
	rm -v *.o || true
