all:
	@rm -rf build
	mkdir build && cd build && cmake .. && make && make install && cd ..

.PHONY:clean
clean:
	@rm -rf bin build