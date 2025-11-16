auh: src/main.cpp 
	g++ -O3 -Wall -std=c++11 -o auh src/main.cpp

install: auh 
	chmod +x auh 
	sudo mv auh /usr/bin/

# Install man page (requires man-db)
install-man: auh.1
	sudo mkdir -p /usr/share/man/man1
	sudo install -m 644 auh.1 /usr/share/man/man1/
	sudo mandb

# Install info documentation (requires texinfo)
install-info: auh.info
	sudo mkdir -p /usr/share/info
	sudo install -m 644 auh.info /usr/share/info/
	sudo install-info /usr/share/info/auh.info /usr/share/info/dir

# Build info file from texinfo source
auh.info: auh.texi
	makeinfo auh.texi

# Build HTML documentation from texinfo
auh.html: auh.texi
	makeinfo --html --no-split auh.texi -o auh.html

# Build all documentation
docs: auh.info auh.html

# Install everything (binary + documentation)
install-all: install install-man install-info

clean: 
	rm -f auh
	rm -f auh.info
	rm -f auh.html

format: src/main.cpp
	clang-format -i --style=gnu src/main.cpp

lint: src/main.cpp
	clang-tidy src/main.cpp -- -std=c++11 -Iinclude

.PHONY: clean install install-man install-info install-all docs
