.PHONY: java clean

java: JvmHello.class
	java -version
	javap -verbose $<
	java JvmHello

JvmHello.class: jvm_hello.hxp classfile.hxp
	cpp $< | hexproc -b > $@

clean:
	rm -v *.class || true
