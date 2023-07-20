### linux环境变量
    vim /etc/bashrc
    source /etc/bashrc
    
    vim ~/.bashrc
    source ...
    source ~/.bashrc

    vim /etc/profile
    export LD_LIBRARY_PATH=
    export PATH=
    source /etc/profile

    vim /etc/ld.so.conf
    ldconfig

### 查看so依赖
    ldd -r ./*.so

### gcc g++升级
    yum install curl-devel expat-devel gettext-devel openssl-devel zlib-devel asciidoc
    yum install make automake gcc gcc-c++ kernel-devel cmake
    yum groupinstall "Development Tools" "Development Libraries"  --skip-broken

    yum install centos-release-scl
    yum list dev\*gcc*

    yum install devtoolset-11-gcc devtoolset-11-gcc-c++

    source /opt/rh/devtoolset-11/enable

    echo "source /opt/rh/devtoolset-11/enable" >> ~/.bashrc
    source ~/.bashrc

    scl enable devtoolset-11 bash

    mv /usr/bin/gcc /usr/bin/gcc-4.8.5
    ln -s /opt/rh/devtoolset-11/root/bin/gcc /usr/bin/gcc

    mv /usr/bin/g++ /usr/bin/g++-4.8.5
    ln -s /opt/rh/devtoolset-11/root/bin/g++ /usr/bin/g++

    gcc --version
    g++ --version

### cmake安装
    yum remove cmake -y
    tar zxvf cmake-3.19.3.tar.gz
    cd cmake-3.19.3.tar.gz
    ./bootstrap
    gmake && gmake install
    /usr/local/bin/cmake --version
    ln -s /usr/local/bin/cmake /usr/bin/
    cmake --version

    chmod -R 777 .
    mkdir -p build
    cd build
    cmake ..
    make

# libuuid安装
    tar zxvf libuuid-1.0.3.tar.gz 
    cd libuuid-1.0.3/
    ./configure --prefix=/usr/local
    make && sudo make install

# jsoncpp安装
    yum install zip unzip
    unzip jsoncpp-0.y.z.zip
    cd jsoncpp-0.y.z
    mkdir -p build/debug
    cd build/debug
    cmake -DCMAKE_BUILD_TYPE=debug -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON -DARCHIVE_INSTALL_DIR=. -G "Unix Makefiles" ../..
    make
    cd jsoncpp-0.y.z/build/debug
    cp ./src/lib_json/libjsoncpp.a /usr/local/lib
    cd ../../
    rm -rf /usr/include/json
    cp -rf include/json /usr/include/

### protobuf静态编译
    rm -f /usr/local/lib/libprotobuf-lite.so.18.0.0
    rm -f /usr/local/lib/libprotobuf-lite.so.18
    rm -f /usr/local/lib/libprotobuf-lite.so
    rm -f /usr/local/lib/libprotobuf-lite.la
    rm -f /usr/local/lib/libprotobuf.so.18
    rm -f /usr/local/lib/libprotobuf.la
    rm -f /usr/local/lib/libprotobuf-lite.a
    rm -f /usr/local/lib/libprotobuf.a
    rm -f /usr/local/lib/libprotobuf.so
    rm -f /usr/local/lib/libprotobuf.so.18.0.0

    cp /usr/lib/gcc/x86_64-redhat-linux/4.8.2/libatomic.so /usr/lib64/libatomic.so
    ln -s /usr/lib64/libatomic.so /usr/lib64/libatomic.so.1.0.0 

    修改 configure
    if test "x${ac_cv_env_CFLAGS_set}" = "x"; then :
      CFLAGS="-fPIC"
    fi
    if test "x${ac_cv_env_CXXFLAGS_set}" = "x"; then :
      CXXFLAGS="-fPIC"
    fi

    ./configure --prefix=/usr/local --disable-shared
    ./configure --prefix=/usr/local --disable-shared CFLAGS='-fPIC' CXXFLAGS='-fPIC' LDFLAGS=''

    make && make install

### linux静态链接(lib*.a)：
    gcc -c mylib.c
    ar -rc libmylib.a mylib.o
    gcc main.c -o main -I./ -L./ -ldl -lmylib
### linux动态链接(lib*.so)：
    gcc -fpic -shared -o libmydll.so mylib.c
    gcc main.c -o main -I./ -L./ -ldl -lmydll
    ldconfig './'
