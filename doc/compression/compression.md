# ZSTD
选版本tag记得选v开头的，不要选zstd开头，本次使用v1.5.6

安装zstd的过程如下：
```sh
git clone git@github.com:facebook/zstd.git
cd zstd
git pull
git checkout v1.5.6
git pull origin v1.5.6
cd build/cmake
mkdir build
cd build
cmake .. 
make -j20
sudo make install
```
然后就可以愉快的使用zstd了