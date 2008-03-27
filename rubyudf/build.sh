#!/bin/sh
sudo rm /opt/local/lib/mysql5/mysql/librubyudf.so
rm librubyudf.*
export MACOSX_DEPLOYMENT_TARGET="10.4"
g++ -I/opt/local/lib/ruby/1.8/i686-darwin8.11.1/ -Wall -fPIC -L/opt/local/lib/ -c -O2 `mysql_config5 --cflags` -o librubyudf.o rubyudf.cpp
#libtool -dynamic librubyudf.o -L/opt/local/lib/ -lruby -lc -o librubyudf.dylib
#cc -c -o librubyudf.o `mysql_config5 --cflags` -I/opt/local/lib/ruby/1.8/i686-darwin8.11.1/ -DMYSQL_DYNAMIC_PLUGIN rubyudf.c
g++ -bundle -std=gnu99 librubyudf.o -lruby -lc -o librubyudf.so 
sudo cp librubyudf.so /opt/local/lib/mysql5/mysql
