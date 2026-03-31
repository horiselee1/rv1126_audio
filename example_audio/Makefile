# 交叉编译工具链
CROSS_COMPILE = /opt/gcc-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar


# 目标文件
TARGET_EXE = rkmedia_ai_test
TARGET_AUDIO = rkmedia_audio_test
BUILD_DIR = build

# 头文件路径
INC_DIR = -I../include_and_lib/BSP/include/rkmedia

# 库路径和库
LIB_DIR = -L../include_and_lib/BSP/lib -L./lib
LIBS = -leasymedia


# 编译参数
CFLAGS = -Wall -O2 -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
#LDFLAGS = $(LIB_DIR) $(LIBS)  -Wl,-rpath=./opencv5/lib:./librknn_api/lib:./lib

LDFLAGS = $(LIB_DIR) $(LIBS)



# 源文件和目标文件
SRCS_EXE = rkmedia_ai_test.c
SRCS_AUDIO = rkmedia_audio_test.c
OBJS_EXE = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS_EXE))
OBJS_AUDIO = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS_AUDIO))


# 默认目标
all: $(BUILD_DIR)/$(TARGET_EXE) $(BUILD_DIR)/$(TARGET_AUDIO)


# 生成可执行文件
$(BUILD_DIR)/$(TARGET_EXE): $(OBJS_EXE)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lm

$(BUILD_DIR)/$(TARGET_AUDIO): $(OBJS_AUDIO)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lm



# 编译规则
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INC_DIR) -c -o $@ $<


# 清理
clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/$(TARGET_EXE) $(BUILD_DIR)/$(TARGET_AUDIO)

.PHONY: all clean
