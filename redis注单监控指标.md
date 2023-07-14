{"latest":[{"op":"mongo.getCollection","key":"","dt":"0.000019"},{"op":"mongo.query","key":"game_user","dt":"0.001289"},{"op":"redis.query","key":"redlock(userid)","dt":"0.000549"},{"op":"redis.query","key":"add_score_order","dt":"0.000533"},{"op":"mongo.insert","key":"add_score_order","dt":"0.000670"},{"op":"mongo.update","key":"game_user","dt":"0.000757"},{"op":"mongo.insert","key":"user_score_record","dt":"0.000726"}],"stat_dt":20.527841,"req_timeout":8,"stat":{"stat_total":86,"stat_succ":71,"stat_fail":15,"stat_ratio":0.825581},"history":{"total":86,"succ":71,"fail":15,"ratio":0.825581}}

//redis 上下分监控指标
key = "s.monitor.order"

value =
{
最近一次请求(上分或下分操作的elapsed detail)
"latest":
	[
	{"op":"mongo.getCollection","key":"","dt":"0.000019"},
	{"op":"mongo.query","key":"game_user","dt":"0.001289"},
	{"op":"redis.query","key":"redlock(userid)","dt":"0.000549"},
	{"op":"redis.query","key":"add_score_order","dt":"0.000533"},
	{"op":"mongo.insert","key":"add_score_order","dt":"0.000670"},
	{"op":"mongo.update","key":"game_user","dt":"0.000757"},
	{"op":"mongo.insert","key":"user_score_record","dt":"0.000726"}
	],

//统计间隔时间	
"stat_dt":20.527841,

//估算每秒请求处理数
"test_TPS":95,

//请求超时时间
"req_timeout":8,

"stat":{
	//统计请求次数
	"stat_total":86,
	//统计成功次数
	"stat_succ":71,
	//统计失败次数
	"stat_fail":15,
	//统计命中率
	"stat_ratio":0.825581},

"history":{
	//历史请求次数
	"total":86,
	//历史成功次数
	"succ":71,
	//历史失败次数
	"fail":15,
	//历史命中率
	"ratio":0.825581}

}