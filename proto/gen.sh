#!/bin/bash
## ./protoc --descriptor_set_out=./test.pb proto/MsgProtocol.proto

protoc --proto_path=src --cpp_out=. src/Common.Message.proto

protoc --proto_path=src --cpp_out=. src/Game.Common.proto
protoc --proto_path=src --cpp_out=. src/ProxyServer.Message.proto
protoc --proto_path=src --cpp_out=. src/HallServer.Message.proto
protoc --proto_path=src --cpp_out=. src/GameServer.Message.proto
protoc --proto_path=src --cpp_out=. src/MatchServer.Message.proto
protoc --proto_path=src --cpp_out=. src/Longhu.Message.proto
protoc --proto_path=src --cpp_out=. src/HongHei.Message.proto
protoc --proto_path=src --cpp_out=. src/Brnn.Message.proto
protoc --proto_path=src --cpp_out=. src/BenCiBaoMa.Message.proto
protoc --proto_path=src --cpp_out=. src/sg.Message.proto
protoc --proto_path=src --cpp_out=. src/Qzpj.Message.proto
protoc --proto_path=src --cpp_out=. src/Brnn.Message.proto
protoc --proto_path=src --cpp_out=. src/sg.Message.proto
protoc --proto_path=src --cpp_out=. src/HongHei.Message.proto
protoc --proto_path=src --cpp_out=. src/Qznn.Message.proto
protoc --proto_path=src --cpp_out=. src/Tbnn.Message.proto
protoc --proto_path=src --cpp_out=. src/ErBaGang.Message.proto
protoc --proto_path=src --cpp_out=. src/Blackjack.Message.proto
protoc --proto_path=src --cpp_out=. src/Gswz.Message.proto
protoc --proto_path=src --cpp_out=. src/zjh.Message.proto
protoc --proto_path=src --cpp_out=. src/Sgj.Message.proto
protoc --proto_path=src --cpp_out=. src/s13s.Message.proto
protoc --proto_path=src --cpp_out=. src/suoha.Message.proto
protoc --proto_path=src --cpp_out=. src/texas.Message.proto
protoc --proto_path=src --cpp_out=. src/Fish.Message.proto
#protoc --proto_path=src --cpp_out=. src/Laba.Message.proto
#protoc --proto_path=src --cpp_out=. src/ddz.Message.proto
protoc --proto_path=src --cpp_out=. src/Gswz.Message.proto
protoc --proto_path=src --cpp_out=. src/KPQznn.Message.proto
protoc --proto_path=src --cpp_out=. src/Qzzjh.Message.proto

protoc --proto_path=src --cpp_out=. src/QZXszSg.Message.proto
protoc --proto_path=src --cpp_out=. src/QZXszZjh.Message.proto
protoc --proto_path=src --cpp_out=. src/Erqs.Message.proto
protoc --proto_path=src --cpp_out=. src/Jsys.Message.proto