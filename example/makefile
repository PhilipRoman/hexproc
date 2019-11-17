.PHONY: java clean

JvmHello.class: jvm_hello.hxp classfile.hxp
	cpp $< | hexproc -b > $@

java: JvmHello.class
	java -version
	javap -verbose $<
	java JvmHello

clean:
	rm -v *.class || true
