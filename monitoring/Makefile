CPPFLAGS=-I/usr/include `root-config --cflags` -I`root-config --incdir` -I/usr/include/python3.8
LDFLAGS=`root-config --glibs`
LDLIBS=`root-config --libs` -lRHTTP -L/usr/lib64 -lfreetype -L/usr/lib/x86_64-linux-gnu/ -lzmq -lpython3.8
online: dict.cxx online.cxx
	g++ $(CPPFLAGS) $(LDFLAGS) -o online dict.cxx online.cxx $(LDLIBS)
dict.cxx: SControl.h Linkdef.h
	rootcling -f $@ -c $(CPPFLAGS) -p $^
.PHONY: clean
clean:
	rm -f online dict.cxx *.o *.pcm
