CXXFLAGS=`pkg-config --cflags opencv`
LDFLAGS=`pkg-config --libs opencv`

%: %.cpp
	g++ $< -o $@ $(CXXFLAGS) $(LDFLAGS)
