#!/bin/bash

./clean.sh

## ./protoc --descriptor_set_out=./test.pb proto/MsgProtocol.proto

#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=./../thirdpart/muduo-vip/muduo/net/protorpc --cpp_out=./../thirdpart/muduo-vip/muduo/net/protorpc --rpc_out=./../thirdpart/muduo-vip/muduo/net/protorpc -I/usr/local/include ./../thirdpart/muduo-vip/muduo/net/protorpc/rpc.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=./../thirdpart/muduo-vip/muduo/protorpc2 --cpp_out=./../thirdpart/muduo-vip/muduo/protorpc2 --rpc_out=./../thirdpart/muduo-vip/muduo/protorpc2 -I/usr/local/include ./../thirdpart/muduo-vip/muduo/protorpc2/rpc2.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=./../thirdpart/muduo-vip/muduo/net/protorpc --cpp_out=./../thirdpart/muduo-vip/muduo/protorpc2 --rpc_out=./../thirdpart/muduo-vip/muduo/protorpc2 -I/usr/local/include -I./../thirdpart/muduo-vip/muduo/net/protorpc ./../thirdpart/muduo-vip/muduo/net/protorpc/rpcservice.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/protorpc.sudoku.proto

#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Common.Message.proto

protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Game.Rpc.proto
protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Game.Common.proto
protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/ProxyServer.Message.proto
protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/HallServer.Message.proto
protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/HallClubServer.Message.proto
protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/GameServer.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/MatchServer.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Longhu.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/HongHei.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Brnn.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/BenCiBaoMa.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/sg.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Qzpj.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Brnn.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/sg.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/HongHei.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Qznn.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Tbnn.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/ErBaGang.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Blackjack.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Gswz.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/zjh.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Sgj.Message.proto
protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/s13s.Message.proto
protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/suoha.Message.proto
protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/texas.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Fish.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Laba.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/ddz.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Gswz.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/KPQznn.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Qzzjh.Message.proto

#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/QZXszSg.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/QZXszZjh.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Erqs.Message.proto
#protoc --plugin=/home/build/protorpc-release/bin/protoc-gen-rpc --proto_path=/usr/local/include --proto_path=src --cpp_out=. --rpc_out=. -I/usr/local/include src/Jsys.Message.proto
