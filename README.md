http_proxy_for_Android
======================

http proxy for Android

It's a part of 3proxy project that implements HTTP/HTTPS proxy with FTP over HTTP support.
3proxy project under BSD style licence, and this project under BSD licence too.
For documentation and more detailed information please refer to 3proxy project(see links)


To build project follow steps below:
0. install android-ndk
1. cd http_proxy_for_Android/jni
2. ANDROID_NDK_PATH/ndk-build
3. at http_proxy_for_Android/libs/armeabi/proxy you'll find output binary. 
Push it to your device and run it from adb shell or android terminal emulator on your device:
./proxy -help
./proxy -i127.0.0.1 -p8888


Links:
http://3proxy.ru/download/
