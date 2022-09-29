CPPFLAGS=-I/usr/include `root-config --cflags`
LDFLAGS=`root-config --glibs`
LDLIBS=`root-config --libs` -lRHTTP -L/usr/lib64 -lfreetype -L/usr/lib/x86_64-linux-gnu/ -lzmq
online: online.o
	g++ $(LDFLAGS) -o online online.o $(LDLIBS)
online.o: online.cxx
	g++ $(CPPFLAGS) -c online.cxx
