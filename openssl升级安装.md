https://mirrors.cloud.tencent.com/openssl/source/

# 下载源码包
wget https://mirrors.cloud.tencent.com/openssl/source/openssl-1.1.1i.tar.gz -O /usr/local/src/openssl-1.1.1i.tar.gz
# 源码编译依赖gcc和perl（5.0版本以上）
yum -y install gcc perl


# 压缩包解压
cd /usr/local/src
tar xf openssl-1.1.1i.tar.gz
# 切换到源码目录
cd openssl-1.1.1i
# 执行编译
./config --prefix=/usr/local/openssl
./config -t
make && make install

# 查询当前版本
rpm -qf `which openssl`
# 查询旧版OpenSSL
rpm -qa |grep openssl
# 卸载旧版OpenSSL（--nodeps参数表示忽略依赖关系）
rpm -e openssl --nodeps
# 使用新版OpenSSL
ln -s /usr/local/openssl/bin/openssl /usr/bin/openssl
# 添加函数库
echo "/usr/local/openssl/lib" >> /etc/ld.so.conf
# 更新函数库
ldconfig -v
# 检查新版OpenSSL
openssl version -a