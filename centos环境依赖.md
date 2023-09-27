### rz/sz安装
    yum -y install lrzsz
    
### screen安装
    yum -y install screen

### zookeeper-3.4.14安装
    yum search java |grep jdk
    yum install java-1.8.0-openjdk
    java -version
    tar -zxvf zookeeper-3.4.14.tar.gz

### mongodb-server安装
    cd /etc/yum.repos.d/
    vim mongodb-org-4.4.repo
        [mongodb-org-4.4]
        name=MongoDB Repository
        baseurl=https://repo.mongodb.org/yum/redhat/$releasever/mongodb-org/4.4/x86_64/
        gpgcheck=1
        enabled=1
        gpgkey=https://www.mongodb.org/static/pgp/server-4.4.asc
    sudo yum install -y mongodb-org
    mongo
    use admin
    db.createUser({user:"root",pwd:"Lcw@12345678#!",roles:[{role:"root",db:"admin"}]})
    vim /etc/mongod.conf

### redis安装
    tar -zxvf redis-7.0.5.tar.gz
    cd redis-7.0.5
    make && make install

### bazel安装
    wget https://copr.fedorainfracloud.org/coprs/vbatts/bazel/repo/epel-7/vbatts-bazel-epel-7.repo
    chmod +x vbatts-bazel-epel-7.repo
    mv vbatts-bazel-epel-7.repo /etc/yum.repos.d/
    vim /etc/profile
    export BAZEL_HOME=/usr/bin
    export PATH =$PATH:$BAZEL_HOME
    source /etc/profile
    yum install bazel
    yum install bazel4

### pkg-config安装 
    https://www.freedesktop.org/wiki/Software/pkg-config/
    wget  https://pkg-config.freedesktop.org/releases/pkg-config-0.29.tar.gz
    tar -zxvf pkg-config-0.29.tar.gz
    ./configure --with-internal-glib
    ./configure --prefix=/usr/local --with-internal-glib
    make && make install

### apt-get安装

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
    debuginfo-install glibc-2.17-326.el7_9.x86_64 libgcc-4.8.5-44.el7.x86_64 libstdc++-4.8.5-44.el7.x86_64 zlib-1.2.7-20.el7_9.x86_64
    yum --enablerepo='*debug*' install /usr/lib/debug/.build-id/81/414f466ecfd2127b001e5aef506d56362b12e8
    yum install gcc
    yum install gcc-c++
    yum install gdb
    yum install curl-devel expat-devel gettext-devel openssl-devel zlib-devel asciidoc
    yum install make automake gcc gcc-c++ kernel-devel cmake
    yum groupinstall "Development Tools" "Development Libraries" --skip-broken

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

### protobuf-v2.5.0编译
    https://github.com/protocolbuffers/protobuf/releases?page=14
    tar zxvf protobuf-2.5.0.tar.gz
    cd protobuf-2.5.0/
	./configure --prefix=/usr/local --disable-shared
    ./configure --prefix=/usr/local --disable-shared CFLAGS='-fPIC' CXXFLAGS='-fPIC'
    make && make install
    
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

    yum -y install atomic
    ln -s /usr/lib/gcc/x86_64-redhat-linux/4.8.2/32/libatomic.so /usr/lib64/libatomic.so.1.0.0

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

### grpc安装
    git clone --recursive -b v1.56.2 https://github.com/grpc/grpc grpc-1.56.2
    https://github.com/grpc/grpc/blob/v1.56.2/BUILDING.md

    git clone --recursive -b v1.4.2 https://github.com/grpc/grpc.git grpc-v1.4.2
    https://github.com/grpc/grpc/blob/v1.4.2/INSTALL.md
    https://github.com/grpc/grpc/blob/v1.4.2/.gitmodules



### grpc与protobuf兼容
    grpc	protobuf	grpc	protobuf	grpc	protobuf
    v1.0.0	3.0.0(GA)	v1.12.0	3.5.2	v1.22.0	3.8.0
    v1.0.1	3.0.2	v1.13.1	3.5.2	v1.23.1	3.8.0
    v1.1.0	3.1.0	v1.14.2	3.5.2	v1.24.0	3.8.0
    v1.2.0	3.2.0	v1.15.1	3.6.1	v1.25.0	3.8.0
    v1.2.0	3.2.0	v1.16.1	3.6.1	v1.26.0	3.8.0
    v1.3.4	3.3.0	v1.17.2	3.6.1	v1.27.3	3.11.2
    v1.3.5	3.2.0	v1.18.0	3.6.1	v1.28.1	3.11.2
    v1.4.0	3.3.0	v1.19.1	3.6.1	v1.29.0	3.11.2
    v1.6.0	3.4.0	v1.20.1	3.7.0	v1.30.0	3.12.2
    v1.8.0	3.5.0	v1.21.3	3.7.0
#### v2.4.0之前
    option cc_generic_services = true;
    option java_generic_services = true;
    option py_generic_services = true;


### mongocxx安装
/usr/local/share/mongo-c-driver/uninstall.sh
/usr/local/share/mongo-cxx-driver/uninstall.sh
/usr/local/src/mongo-c-driver-1.17.3/cmake-build/generate_uninstall/uninstall.sh
/usr/local/src/mongo-cxx-driver-r3.6.2/build/generate_uninstall/uninstall.sh

wget https://github.com/mongodb/mongo-c-driver/releases/download/1.24.3/mongo-c-driver-1.24.3.tar.gz
tar xzf mongo-c-driver-1.24.3.tar.gz
cd mongo-c-driver-1.24.3
mkdir cmake-build
cd cmake-build
cmake .. -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF
cmake --build .
cmake --build . --target install
make && make install

wget https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.8.0/mongo-cxx-driver-r3.8.0.tar.gz
tar -xzf mongo-cxx-driver-r3.8.0.tar.gz
cd mongo-cxx-driver-r3.8.0/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBSONCXX_POLY_USE_BOOST=1
cmake --build .
make && make install