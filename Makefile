# ==============================================================
# Makefile for iw-gaussimp & iw-mailmon
# ==============================================================

CC          ?= gcc
CFLAGS      ?= -Wall -Wextra -O2 -g

# 两个目标可执行文件名
TARGET_GAUSS := iw-gaussimp
TARGET_MAIL  := iw-mailmon

# --------------------------------------------------------------
# 源码文件定义
# --------------------------------------------------------------
# 1. GaussDB 导入程序源码
SRCS_GAUSS   := iw-gaussimp.c
OBJS_GAUSS   := $(SRCS_GAUSS:.c=.o)

# 2. 邮件监控程序源码（自动扫描引入当前目录下匹配的 goolib 实现代码）
SRCS_MAIL    := iw-mailmon.c $(wildcard goolib-*.c)
OBJS_MAIL    := $(SRCS_MAIL:.c=.o)

# --------------------------------------------------------------
# 依赖环境配置: iw-gaussimp (GaussDB / libpq)
# --------------------------------------------------------------
GAUSS_HOME   ?= /opt/gaussdb
PG_CONFIG    := $(shell which pg_config 2>/dev/null)

GAUSS_INCDIR := /export/opt/iWork/pgsql/include
 GAUSS_LIBDIR := /export/opt/iWork/pgsql/lib

GAUSS_CFLAGS := -I$(GAUSS_INCDIR)
GAUSS_LDFLAGS:= -L$(GAUSS_LIBDIR) -Wl,-rpath,$(GAUSS_LIBDIR)
GAUSS_LDLIBS := -lpq

# --------------------------------------------------------------
# 依赖环境配置: iw-mailmon (Pthread & Goolib)
# --------------------------------------------------------------
MAIL_CFLAGS  := -pthread -I.
MAIL_LDFLAGS := -pthread
MAIL_LDLIBS  := -lpthread

# --------------------------------------------------------------
# 构建规则
# --------------------------------------------------------------
.PHONY: all clean help info $(TARGET_GAUSS) $(TARGET_MAIL)

# 默认同时编译两个程序
all: $(TARGET_GAUSS) $(TARGET_MAIL)

# 1. 编译 iw-gaussimp
$(TARGET_GAUSS): $(OBJS_GAUSS)
	$(CC) $(OBJS_GAUSS) $(GAUSS_LDFLAGS) $(GAUSS_LDLIBS) -o $@
	@echo ">>> Successfully built: $(TARGET_GAUSS)"

# 2. 编译 iw-mailmon
$(TARGET_MAIL): $(OBJS_MAIL)
	$(CC) $(OBJS_MAIL) $(MAIL_LDFLAGS) $(MAIL_LDLIBS) -o $@
	@echo ">>> Successfully built: $(TARGET_MAIL)"

# 单独的编译规则（指定对应依赖的 CFLAGS）
$(OBJS_GAUSS): %.o: %.c
	$(CC) $(CFLAGS) $(GAUSS_CFLAGS) -c $< -o $@

$(OBJS_MAIL): %.o: %.c
	$(CC) $(CFLAGS) $(MAIL_CFLAGS) -c $< -o $@

# --------------------------------------------------------------
# 辅助指令
# --------------------------------------------------------------
clean:
	rm -f $(OBJS_GAUSS) $(OBJS_MAIL) $(TARGET_GAUSS) $(TARGET_MAIL)
	@echo "Cleaned all object files and binaries."

info:
	@echo "--- Configuration Info ---"
	@echo "CC              : $(CC)"
	@echo "Gauss Inc Dir   : $(GAUSS_INCDIR)"
	@echo "Gauss Lib Dir   : $(GAUSS_LIBDIR)"
	@echo "Mailmon Sources : $(SRCS_MAIL)"

help:
	@echo "Usage:"
	@echo "  make              - Build both iw-gaussimp and iw-mailmon"
	@echo "  make iw-gaussimp  - Build only the GaussDB importer"
	@echo "  make iw-mailmon   - Build only the mail file watcher"
	@echo "  make clean        - Remove compiled files"
	@echo "  make info         - Show auto-detected include/lib paths"