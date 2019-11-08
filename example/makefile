.PHONY: java clean

JvmHello.class: jvm_hello.hxp classfile.hxp
	cpp $< | hexproc | xxd -r -p > $@

java: JvmHello.class
	java -version
	javap -verbose $<
	java JvmHello

clean:
	rm -v *.class || true
