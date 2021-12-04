TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
QT -= gui
CONFIG -= app_bundle
QT += multimedia
QT += core
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += main.cpp \
    ThreadProvider.cpp \
    FramePtrWrapper.cpp \
    Utils.cpp \
    AudioProvider.cpp \
    RecordAudioProvider.cpp \
    XMediaEncode.cpp \
    XRtmp.cpp \
    VideoProvider.cpp \
    CameraVideoProvider.cpp \
    FileVideoProvider.cpp \
    FileAudioProvider.cpp \
    NanoDetOpenVINOFilter.cpp
win32{
    options = $$find(QMAKESPEC, "msvc2015_64")
    MYLIB=-L$$PWD/../../lib/win64
    DESTDIR = "../../bin/win64"
    lessThan(options, 1) {
        message("win32_mingw Lib")
        MYLIB=-L$$PWD/../../lib/win32_mingw
        DESTDIR = $$PWD/../../bin/win32_mingw
    } else {
        message("win64 Lib")
    }
}
linux{
    message("linux Lib")
    MYLIB=-L/usr/local/ffmpeg/ -lavcodec
}

LIBS += $$MYLIB
LIBS += -lavformat
LIBS += -lavutil
LIBS += -lavcodec
LIBS += -lswscale
LIBS += -lswresample
INCLUDEPATH += $$PWD/../../include

OPENVINO_ABS_PATH=D:/IntelSWTools/openvino_2021
OPENVINOPATH=$$OPENVINO_ABS_PATH/inference_engine
OPENVINO_LIB=$$OPENVINOPATH/lib/intel64
OPENVINO_INC=$$OPENVINOPATH/include

INCLUDEPATH += $$OPENVINO_INC

CONFIG(debug, debug|release) {
LIBS += -L$$OPENVINO_LIB\Debug
LIBS += -linference_engined \
        -linference_engine_c_apid \
        -linference_engine_transformationsd
} else{
LIBS += -L$$OPENVINO_LIB\Release
LIBS += -linference_engine \
        -linference_engine_c_api \
        -linference_engine_transformations
}


OPENCVPATH=$$OPENVINO_ABS_PATH/opencv
OPENCV_LIB=$$OPENCVPATH/lib
OPENCV_INC=$$OPENCVPATH/include
INCLUDEPATH += $$OPENCV_INC

CONFIG(debug, debug|release) {
LIBS += -L$$OPENCV_LIB
LIBS += -lopencv_core453d \
        -lopencv_dnn453d \
        -lopencv_ml453d \
        -lopencv_highgui453d \
        -lopencv_imgcodecs453d \
        -lopencv_imgproc453d \
        -lopencv_video453d
}else{
LIBS += -L$$OPENCV_LIB
LIBS += -lopencv_core453 \
        -lopencv_dnn453 \
        -lopencv_ml453 \
        -lopencv_highgui453 \
        -lopencv_imgcodecs453 \
        -lopencv_imgproc453 \
        -lopencv_video453 \
        -lopencv_videoio453
}



HEADERS += \
    ThreadProvider.h \
    FramePtrWrapper.h \
    CameraVideoProvider.h \
    Utils.h \
    AudioProvider.h \
    RecordAudioProvider.h \
    XMediaEncode.h \
    XRtmp.h \
    VideoProvider.h \
    FileVideoProvider.h \
    FileAudioProvider.h \
    ImageFilter.h \
    NanoDetOpenVINOFilter.h

msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}else{
    QMAKE_CFLAGS += -fexec-charset=GBK \
    -finput-charset=UTF-8
    QMAKE_CXXFLAGS += -fexec-charset=GBK \
    -finput-charset=UTF-8
}
