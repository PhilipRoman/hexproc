# Hexproc

Hexproc is a hexadecimal preprocessor for building hex and binary files.
See the [manual](man/hexproc.adoc) for more information.

Hexproc is written in C and has no other required runtime dependencies.

You can either build it yourself or grab the pre-built version from
[GitHub releases](https://github.com/PhilipRoman/hexproc/releases)

## Build

Runing `make` will produce a standalone executable `hexproc`. Copy it wherever
you want.

Requirements:

	* `make` (tested on GNU make)
	* Posix and C99 compatible compiler

## Documentation

Pre-built documention is provided in the `man/` directory in various formats.

Running `make doc` will generate documentation. `asciidoctor` and `wkhtmltopdf`
is required to rebuild the documentation.

## License

[MIT](LICENSE)
