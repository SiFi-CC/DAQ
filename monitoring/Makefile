INCLUDES=-I/usr/include -I`root-config --incdir` -I/usr/include/python3.10
CPPFLAGS=`root-config --cflags`
LDFLAGS=`root-config --glibs`
LDLIBS=`root-config --libs` -lRHTTP -L/usr/lib64 -lfreetype -L/usr/lib/x86_64-linux-gnu/ -lzmq -L/usr/lib/x86_64-linux-gnu/ -lpython3.10
online: CommandLine.cxx online.cxx
	g++ $(INCLUDES) $(CPPFLAGS) $(LDFLAGS) -o online CommandLine.cxx online.cxx $(LDLIBS)
.PHONY: clean
clean:
	rm -f online *.o *.pcm
