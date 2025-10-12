auh: src/main.cpp 
	clang++ -Wall -Werror -std=c++11 -o auh src/main.cpp

install: auh 
	chmod +x auh 
	sudo mv auh /usr/share/bin/

clean: 
	rm -f auh

format: src/main.cpp
	clang-format -i --style=llvm src/main.cpp

lint: src/main.cpp
	clang-tidy src/main.cpp -- -std=c++11 -Iinclude

.PHONY: clean
