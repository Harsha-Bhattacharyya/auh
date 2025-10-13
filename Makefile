auh: src/main.cpp 
	g++ -O3 -Wall -Werror -std=c++11 -o auh src/main.cpp

install: auh 
	chmod +x auh 
	sudo mv auh /usr/bin/

clean: 
	rm -f auh

format: src/main.cpp
	clang-format -i --style=gnu src/main.cpp

lint: src/main.cpp
	clang-tidy src/main.cpp -- -std=c++11 -Iinclude

.PHONY: clean
