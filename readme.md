# 说明

1. 使用的是openvino 2021版本 ffmpeg3.x版本 opencv是openvino中带的4.5版本
2. QT用的是msvc64 5.9.9版本
3. 本地测试openvino的debug模式有问题，release模式没问题
4. openvino携带的opencv不可以解码mp4文件，如果需要请自行编译
5. 配置好pro文件中`ffmpeg`、`opencv`、`openvino`的路径(ffmpeg的包在Release里可以下载)
6. 将openvino的相关路径加入环境变量，或者参考我视频中的做法
7. nanodet转openvino下的模型参考 https://github.com/RangiLyu/nanodet/tree/main/demo_openvino
8. 直播rtmp服务搭建使用的是`nginx-rtmp`，搭建在远程linux中，源码里写localhost是因为用了ssh端口映射映射到了本地
9. 软件还有诸多问题，仅仅是一个demo，发现问题还请在issue区提问。

