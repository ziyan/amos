all: build

build:
	@mkdir -p .build
	@cd .build && cmake .. && make --no-print-directory

install:
	@cd .build && make install --no-print-directory

uninstall:
	@rm -f `cat .build/install_manifest.txt`

clear:
	@find . -iname \*~ -exec rm -f {} \;

clean:
	@rm -rf .build

