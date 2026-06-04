CXX      := g++
CXXFLAGS := -O2 -s -static -Wall -Wextra -std=c++11
TARGET   := nls2txt
SRC      := nls2txt.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: clean
