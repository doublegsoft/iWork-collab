iWork
=====

```
   ▄▄▄▄  ▄▄▄  ▄▄▄▄                   
▀▀ ▀███  ███  ███▀            ▄▄     
██  ███  ███  ███ ▄███▄ ████▄ ██ ▄█▀ 
██  ███▄▄███▄▄███ ██ ██ ██ ▀▀ ████   
██▄  ▀████▀████▀  ▀███▀ ██    ██ ▀█▄ 
```

## 提示词协议

**代码规范**

请求格式：

```
我要求的代码格式：

1. 缩进为2
2. 如果是C语言和C++语言，pointer, Left-Aligned (Type-Oriented)
3. 返回的代码，在代码块之前描述清楚文件路径，格式为filepath: <文件路径>

你记住了吗？那我们就正式开始。
```

**LLM必须告诉我完整的文件名和文件内容**

应答格式：

```
filepath: <文件路径>

<完整文件代码>
```


## libwebsockets

### 编译

```

cmake ../.. -DLWS_WITH_LIBUV=ON -DLWS_WITH_TLS=OFF -DLWS_WITH_SSL=OFF   \
-DLIBUV_INCLUDE_DIRS=/export/local/works/doublegsoft.ai/iWork/03.Development/iWork-collab/3rd/libuv-1.52.1/include   \
-DLIBUV_LIBRARIES=/export/local/works/doublegsoft.ai/iWork/03.Development/iWork-collab/3rd/libuv-1.52.1/build/linux/libuv.so
```

## 启动应用

**环境变量**

```

export LD_LIBRARY_PATH=/Users/christian/export/local/works/doublegsoft.ai/iWork/03.Development/iWork-collab/3rd/libuv-1.52.1/build/darwin:/Users/christian/export/local/works/doublegsoft.ai/iWork/03.Development/iWork-3rd/libwebsockets-4.5.4/build/darwin/lib
```