iWork
=====

```
   ▄▄▄▄  ▄▄▄  ▄▄▄▄                   
▀▀ ▀███  ███  ███▀            ▄▄     
██  ███  ███  ███ ▄███▄ ████▄ ██ ▄█▀ 
██  ███▄▄███▄▄███ ██ ██ ██ ▀▀ ████   
██▄  ▀████▀████▀  ▀███▀ ██    ██ ▀█▄ 
```

## libwebsockets

### 编译

```

cmake ../.. -DLWS_WITH_LIBUV=ON -DLWS_WITH_TLS=OFF -DLWS_WITH_SSL=OFF
```

### 启动

```

export LD_LIBRARY_PATH=/Users/christian/export/local/works/doublegsoft.ai/iWork/03.Development/iWork-collab/3rd/libuv-1.52.1/build/darwin:/Users/christian/export/local/works/doublegsoft.ai/iWork/03.Development/iWork-collab/3rd/libwebsockets-4.5.4/build/darwin/lib
```