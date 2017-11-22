TARGET_EXEC=a.out
BUILD_DIR=./build
SRC_DIRS=./src

SRCS:=$(shell find $(SRC_DIRS) -name *.cpp -or -name *.cc -or -name *.c)
OBJS:=$(SRCS:%=$(BUILD_DIR)/%.o)
DEPS:=$(OBJS:.o=.d)
LIBS:=-L/opt/rocksdb -lstdc++ -lrocksdb

INC_DIRS:=$(shell find $(SRC_DIRS) -type d)
INC_FLAGS:=$(addprefix -I,$(INC_DIRS))

CPPFLAGS=$(INC_FLAGS) -std=c++11 -Wall -O2 -I /opt/rocksdb/include#-g -DDEBUG#-pedantic

$(BUILD_DIR)/$(TARGET_EXEC):$(OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OBJS) -o $@  $(LDFLAGS) $(LIBS)


$(BUILD_DIR)/%.cc.o:%.cc
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS)   -c $< -o $@

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P=mkdir -p
