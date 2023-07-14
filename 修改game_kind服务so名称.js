// usecase:
//mongo -u txkj007 -p aa123465 --authenticationDatabase admin  192.168.2.226:37017/gamemain e:/mongoscript/agentid_fix.js > e:/mongoscript/update.log
//mongo 192.168.2.97:27017/gameconfig e:/mongoscript/add_game_890.js
var s = db.getMongo().startSession()
var android_strategy = "android_strategy";
var game_kind = "game_kind";

s.startTransaction()
print("==========================================> "+Date("<YYYY-mm-ddTHH:MM:ssZ>"))
try {
	//update game_kind 百人龙虎 900
	db[game_kind].updateOne({ 'gameid':900 },{$set:{'servicename':'Game_longhu'}} )
	//update game_kind 百人牛牛 930
	db[game_kind].updateOne({ 'gameid':930 },{$set:{'servicename':'Game_brnn'}} )
	//update game_kind 扎金花 220
	db[game_kind].updateOne({ 'gameid':220 },{$set:{'servicename':'Game_zjh'}} )
	//update game_kind 抢庄牛牛 830
	db[game_kind].updateOne({ 'gameid':830 },{$set:{'servicename':'Game_qznn'}} )
	//update game_kind 百人二八杠 720
	db[game_kind].updateOne({ 'gameid':720 },{$set:{'servicename':'Game_ebg'}} )
	//update game_kind 三公 860
	db[game_kind].updateOne({ 'gameid':860 },{$set:{'servicename':'Game_sg'}} )
	//update game_kind 红黑大战 210
	db[game_kind].updateOne({ 'gameid':210 },{$set:{'servicename':'Game_honghei'}} )
	//update game_kind 21点 600
	db[game_kind].updateOne({ 'gameid':600 },{$set:{'servicename':'Game_hjk'}} )
	//update game_kind 港式五张 400
	db[game_kind].updateOne({ 'gameid':400 },{$set:{'servicename':'Game_gswz'}} )
	//update game_kind 通比牛牛 870
	db[game_kind].updateOne({ 'gameid':870 },{$set:{'servicename':'Game_tbnn'}} )
	//update game_kind 水果机 1810
	db[game_kind].updateOne({ 'gameid':1810 },{$set:{'servicename':'Game_sgj'}} )
	//update game_kind 十三水 630
	db[game_kind].updateOne({ 'gameid':630 },{$set:{'servicename':'Game_s13s'}} )
	//update game_kind 金蟾捕鱼 500
	db[game_kind].updateOne({ 'gameid':500 },{$set:{'servicename':'Game_jcfish'}} )
	//update game_kind 看牌抢庄牛牛 890
	db[game_kind].updateOne({ 'gameid':890 },{$set:{'servicename':'Game_kpqznn'}} )
	//update game_kind 奔驰宝马 1960
	db[game_kind].updateOne({ 'gameid':1960 },{$set:{'servicename':'Game_bcbm'}} )
	//update game_kind 斗地主 610
	db[game_kind].updateOne({ 'gameid':610 },{$set:{'servicename':'Game_ddz'}} )
	//update game_kind 金鲨银鲨 1940
	db[game_kind].updateOne({ 'gameid':1940 },{$set:{'servicename':'Game_jsys'}} )
	//update game_kind 抢庄选三张三公场 820
	db[game_kind].updateOne({ 'gameid':820 },{$set:{'servicename':'Game_qzxszsg'}} )
	//update game_kind 抢庄选三张金花场 300
	db[game_kind].updateOne({ 'gameid':300 },{$set:{'servicename':'Game_qzxszzjh'}} )
	//update game_kind 二人雀神 740
	db[game_kind].updateOne({ 'gameid':740 },{$set:{'servicename':'Game_erqs'}} )
	//update game_kind 抢庄扎金花 240
	db[game_kind].updateOne({ 'gameid':240 },{$set:{'servicename':'Game_qzzjh'}} )
	s.commitTransaction();
}catch(e){
	print(e);
	s.abortTransaction();
}
print("==========================================> "+Date("<YYYY-mm-ddTHH:MM:ssZ>"))
print("Successed...");