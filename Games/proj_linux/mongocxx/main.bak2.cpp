//http://mongocxx.org/mongocxx-v3/tutorial/
//http://mongocxx.org/api/mongocxx-v3/classmongocxx_1_1collection.html
//http://mongocxx.org/api/mongocxx-v3/examples_2mongocxx_2client_session_8cpp-example.html#_a2
//https://blog.csdn.net/ppwwp/article/details/80051585

#include <vector>
#include <iostream>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/exception/exception.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using bsoncxx::to_json;

using namespace mongocxx;
using namespace bsoncxx::builder::basic;
using namespace bsoncxx::types;

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;


mongocxx::instance inst{};
//mongocxx::uri uri("mongodb://192.168.2.97:27017");
//mongocxx::client client(uri);
// mongocxx::client conn{ mongocxx::uri{"mongodb://192.168.2.97:27017"} };//conn
// mongocxx::database db = conn["gameconfig"];//db
// mongocxx::collection collection = db["android_user"];//tblname
auto builder = bsoncxx::builder::stream::document{};
bsoncxx::document::value doc = builder
<< "name" << "MongoDB"
<< "type" << "database"
<< "count" << 1
<< "versions" << bsoncxx::builder::stream::open_array
<< "v3.2" << "v3.0" << "v2.6"
<< close_array
<< "info" << bsoncxx::builder::stream::open_document
<< "x" << 203
<< "y" << 102
<< bsoncxx::builder::stream::close_document
<< bsoncxx::builder::stream::finalize;

using namespace std;

int main()
{
    //    mongocxx::instance inst{};
    ////    mongocxx::client conn{mongocxx::uri{"mongodb://192.168.2.97:27017/"}};
    //    mongocxx::client conn{mongocxx::uri{"mongodb://192.168.2.97:27017"}};

    //    bsoncxx::builder::stream::document document{};

    //    auto collection = conn["testdb"]["testcollection"];
    //    document << "hello" << "worldxxx";

    //    collection.insert_one(document.view());
    //    auto cursor = collection.find({});

    //    for (auto&& doc : cursor) {
    //        std::cout << bsoncxx::to_json(doc) << std::endl;
    //    }

    //mongocxx::instance inst{};
    mongocxx::client conn{mongocxx::uri{"mongodb://192.168.2.97:27017"}};

//    bsoncxx::builder::stream::document doc{};
    auto collection = conn["gameconfig"]["game_kind"];
    bsoncxx::document::value doc = make_document(kvp("gameid",b_int32{730}),
                                                 kvp("gamename","抢庄牌九"),
                                                 kvp("sort",b_int32{1}),
                                                 kvp("servicename","Game_Qzpj"),
												 kvp("revenueRatio", b_int32{5}),
                                                 kvp("type",b_int32{0}),
                                                 kvp("ishot",b_int32{0}),
                                                 kvp("status",b_int32{1}),
                                                 kvp("rooms",make_array(
                                                         make_document(kvp("roomid",b_int32{7301}),
                                                                       kvp("roomname","体验房"),
                                                                       kvp("tablecount",b_int32{100}),
                                                                       kvp("floorscore",b_int64{100}),
                                                                       kvp("ceilscore",b_int64{100}),
                                                                       kvp("minplayernum",b_int32{2}),
                                                                       kvp("maxplayernum",b_int32{5}),
                                                                       kvp("enterminscore",b_int64{2000}),
                                                                       kvp("entermaxscore",b_int64{-1}),
                                                                       kvp("broadcastscore",b_int64{20000}),
                                                                       kvp("enableandroid",b_int32{1}),
                                                                       kvp("androidcount",b_int32{30}),
                                                                       kvp("androidmaxusercount",b_int32{80}),
                                                                       kvp("totalstock",b_int64{-1}),
                                                                       kvp("totalstocklowerlimit",b_int64{-1}),
                                                                       kvp("totalstockhighlimit",b_int64{-1}),
                                                                       kvp("systemkillallratio",b_int32{0}),
                                                                       kvp("systemreduceratio",b_int32{0}),
                                                                       kvp("changecardratio",b_int32{0}),
                                                                       kvp("maxjettonscore",b_int64{2000}),
                                                                       kvp("jettons",make_array(b_int64{100},
                                                                                                b_int64{200},
                                                                                                b_int64{300},
                                                                                                b_int64{400},
                                                                                                b_int64{500},
                                                                                                b_int64{600},
                                                                                                b_int64{700})),
                                                                       kvp("status",b_int32{1})),
                                                         make_document(kvp("roomid",b_int32{7302}),
                                                                       kvp("roomname","初级房"),
                                                                       kvp("tablecount",b_int32{100}),
                                                                       kvp("floorscore",b_int64{100}),
                                                                       kvp("ceilscore",b_int64{100}),
                                                                       kvp("minplayernum",b_int32{2}),
                                                                       kvp("maxplayernum",b_int32{5}),
                                                                       kvp("enterminscore",b_int64{2000}),
                                                                       kvp("entermaxscore",b_int64{-1}),
                                                                       kvp("broadcastscore",b_int64{20000}),
                                                                       kvp("enableandroid",b_int32{0}),
                                                                       kvp("androidcount",b_int32{30}),
                                                                       kvp("androidmaxusercount",b_int32{80}),
                                                                       kvp("totalstock",b_int64{-1}),
                                                                       kvp("totalstocklowerlimit",b_int64{-1}),
                                                                       kvp("totalstockhighlimit",b_int64{-1}),
                                                                       kvp("systemkillallratio",b_int32{0}),
                                                                       kvp("systemreduceratio",b_int32{0}),
                                                                       kvp("changecardratio",b_int32{0}),
                                                                       kvp("maxjettonscore",b_int64{2000}),
                                                                       kvp("jettons",make_array(b_int64{500},
                                                                                                b_int64{1000},
                                                                                                b_int64{1500},
                                                                                                b_int64{2000},
                                                                                                b_int64{2500},
                                                                                                b_int64{3000},
                                                                                                b_int64{3500})),
                                                                       kvp("status",b_int32{1})),
                                                         make_document(kvp("roomid",b_int32{7303}),
                                                                       kvp("roomname","中级房"),
                                                                       kvp("tablecount",b_int32{100}),
                                                                       kvp("floorscore",b_int64{100}),
                                                                       kvp("ceilscore",b_int64{100}),
                                                                       kvp("minplayernum",b_int32{2}),
                                                                       kvp("maxplayernum",b_int32{5}),
                                                                       kvp("enterminscore",b_int64{2000}),
                                                                       kvp("entermaxscore",b_int64{-1}),
                                                                       kvp("broadcastscore",b_int64{20000}),
                                                                       kvp("enableandroid",b_int32{0}),
                                                                       kvp("androidcount",b_int32{30}),
                                                                       kvp("androidmaxusercount",b_int32{80}),
                                                                       kvp("totalstock",b_int64{-1}),
                                                                       kvp("totalstocklowerlimit",b_int64{-1}),
                                                                       kvp("totalstockhighlimit",b_int64{-1}),
                                                                       kvp("systemkillallratio",b_int32{0}),
                                                                       kvp("systemreduceratio",b_int32{0}),
                                                                       kvp("changecardratio",b_int32{0}),
                                                                       kvp("maxjettonscore",b_int64{2000}),
                                                                       kvp("jettons",make_array(b_int64{2000},
                                                                                                b_int64{4000},
                                                                                                b_int64{6000},
                                                                                                b_int64{8000},
                                                                                                b_int64{10000},
                                                                                                b_int64{12000},
                                                                                                b_int64{14000})),
                                                                       kvp("status",b_int32{1})),
                                                         make_document(kvp("roomid",b_int32{7304}),
                                                                       kvp("roomname","高级房"),
                                                                       kvp("tablecount",b_int32{100}),
                                                                       kvp("floorscore",b_int64{100}),
                                                                       kvp("ceilscore",b_int64{100}),
                                                                       kvp("minplayernum",b_int32{2}),
                                                                       kvp("maxplayernum",b_int32{5}),
                                                                       kvp("enterminscore",b_int64{2000}),
                                                                       kvp("entermaxscore",b_int64{-1}),
                                                                       kvp("broadcastscore",b_int64{20000}),
                                                                       kvp("enableandroid",b_int32{0}),
                                                                       kvp("androidcount",b_int32{30}),
                                                                       kvp("androidmaxusercount",b_int32{80}),
                                                                       kvp("totalstock",b_int64{-1}),
                                                                       kvp("totalstocklowerlimit",b_int64{-1}),
                                                                       kvp("totalstockhighlimit",b_int64{-1}),
                                                                       kvp("systemkillallratio",b_int32{0}),
                                                                       kvp("systemreduceratio",b_int32{0}),
                                                                       kvp("changecardratio",b_int32{0}),
                                                                       kvp("maxjettonscore",b_int64{2000}),
                                                                       kvp("jettons",make_array(b_int64{5000},
                                                                                                b_int64{10000},
                                                                                                b_int64{15000},
                                                                                                b_int64{20000},
                                                                                                b_int64{25000},
                                                                                                b_int64{30000},
                                                                                                b_int64{35000})),
                                                                       kvp("status",b_int32{1}))

                                                         )));

	bsoncxx::document::value doc1 = make_document(kvp("gameid", b_int32{ 630 }),
												kvp("gamename", "十三水"),
												kvp("sort", b_int32{ 1 }),
												kvp("servicename", "Game_s13s"),
												kvp("revenueRatio", b_int32{ 5 }),
												kvp("type",b_int32{0}),
                                                kvp("ishot",b_int32{0}),
                                                kvp("status",b_int32{1}),
												kvp("updatePlayerNum", b_int32{30}),
												kvp("rooms", make_array(
													make_document(kvp("roomid", b_int32{ 6301 }),
														kvp("roomname", "体验房(常规场)"),
														kvp("tablecount", b_int32{ 100 }),
														kvp("floorscore", b_int64{ 0 }),
														kvp("ceilscore", b_int64{ 0 }),
														kvp("minplayernum", b_int32{ 2 }),
														kvp("maxplayernum", b_int32{ 5 }),
														kvp("enterminscore", b_int64{ 0 }),
														kvp("entermaxscore", b_int64{ -1 }),
														kvp("broadcastscore", b_int64{ 20000 }),
														kvp("enableandroid", b_int32{ 0 }),
														kvp("androidcount", b_int32{ 30 }),
														kvp("androidmaxusercount", b_int32{ 80 }),
														kvp("totalstock", b_int64{ -1 }),
														kvp("totalstocklowerlimit", b_int64{ -1 }),
														kvp("totalstockhighlimit", b_int64{ -1 }),
														kvp("systemkillallratio", b_int32{ 0 }),
														kvp("systemreduceratio", b_int32{ 0 }),
														kvp("changecardratio", b_int32{ 0 }),
														kvp("maxjettonscore", b_int64{ 2000 }),
														kvp("jettons", make_array(b_int64{ 100 },
															b_int64{ 200 },
															b_int64{ 300 },
															b_int64{ 400 },
															b_int64{ 500 },
															b_int64{ 600 },
															b_int64{ 700 })),
														kvp("status", b_int32{ 1 })),
													make_document(kvp("roomid", b_int32{ 6302 }),
														kvp("roomname", "初级房(常规场)"),
														kvp("tablecount", b_int32{ 100 }),
														kvp("floorscore", b_int64{ 0 }),
														kvp("ceilscore", b_int64{ 0 }),
														kvp("minplayernum", b_int32{ 2 }),
														kvp("maxplayernum", b_int32{ 5 }),
														kvp("enterminscore", b_int64{ 0 }),
														kvp("entermaxscore", b_int64{ -1 }),
														kvp("broadcastscore", b_int64{ 20000 }),
														kvp("enableandroid", b_int32{ 0 }),
														kvp("androidcount", b_int32{ 30 }),
														kvp("androidmaxusercount", b_int32{ 80 }),
														kvp("totalstock", b_int64{ -1 }),
														kvp("totalstocklowerlimit", b_int64{ -1 }),
														kvp("totalstockhighlimit", b_int64{ -1 }),
														kvp("systemkillallratio", b_int32{ 0 }),
														kvp("systemreduceratio", b_int32{ 0 }),
														kvp("changecardratio", b_int32{ 0 }),
														kvp("maxjettonscore", b_int64{ 2000 }),
														kvp("jettons", make_array(b_int64{ 500 },
															b_int64{ 1000 },
															b_int64{ 1500 },
															b_int64{ 2000 },
															b_int64{ 2500 },
															b_int64{ 3000 },
															b_int64{ 3500 })),
														kvp("status", b_int32{ 1 })),
													make_document(kvp("roomid", b_int32{ 6303 }),
														kvp("roomname", "中级房(常规场)"),
														kvp("tablecount", b_int32{ 100 }),
														kvp("floorscore", b_int64{ 0 }),
														kvp("ceilscore", b_int64{ 0 }),
														kvp("minplayernum", b_int32{ 2 }),
														kvp("maxplayernum", b_int32{ 5 }),
														kvp("enterminscore", b_int64{ 0 }),
														kvp("entermaxscore", b_int64{ -1 }),
														kvp("broadcastscore", b_int64{ 20000 }),
														kvp("enableandroid", b_int32{ 0 }),
														kvp("androidcount", b_int32{ 30 }),
														kvp("androidmaxusercount", b_int32{ 80 }),
														kvp("totalstock", b_int64{ -1 }),
														kvp("totalstocklowerlimit", b_int64{ -1 }),
														kvp("totalstockhighlimit", b_int64{ -1 }),
														kvp("systemkillallratio", b_int32{ 0 }),
														kvp("systemreduceratio", b_int32{ 0 }),
														kvp("changecardratio", b_int32{ 0 }),
														kvp("maxjettonscore", b_int64{ 2000 }),
														kvp("jettons", make_array(b_int64{ 2000 },
															b_int64{ 4000 },
															b_int64{ 6000 },
															b_int64{ 8000 },
															b_int64{ 10000 },
															b_int64{ 12000 },
															b_int64{ 14000 })),
														kvp("status", b_int32{ 1 })),
													make_document(kvp("roomid", b_int32{ 6304 }),
														kvp("roomname", "高级房(常规场)"),
														kvp("tablecount", b_int32{ 100 }),
														kvp("floorscore", b_int64{ 0 }),
														kvp("ceilscore", b_int64{ 0 }),
														kvp("minplayernum", b_int32{ 2 }),
														kvp("maxplayernum", b_int32{ 5 }),
														kvp("enterminscore", b_int64{ 0 }),
														kvp("entermaxscore", b_int64{ -1 }),
														kvp("broadcastscore", b_int64{ 20000 }),
														kvp("enableandroid", b_int32{ 0 }),
														kvp("androidcount", b_int32{ 30 }),
														kvp("androidmaxusercount", b_int32{ 80 }),
														kvp("totalstock", b_int64{ -1 }),
														kvp("totalstocklowerlimit", b_int64{ -1 }),
														kvp("totalstockhighlimit", b_int64{ -1 }),
														kvp("systemkillallratio", b_int32{ 0 }),
														kvp("systemreduceratio", b_int32{ 0 }),
														kvp("changecardratio", b_int32{ 0 }),
														kvp("maxjettonscore", b_int64{ 2000 }),
														kvp("jettons", make_array(b_int64{ 5000 },
															b_int64{ 10000 },
															b_int64{ 15000 },
															b_int64{ 20000 },
															b_int64{ 25000 },
															b_int64{ 30000 },
															b_int64{ 35000 })),
														kvp("status", b_int32{ 1 })),
															make_document(kvp("roomid", b_int32{ 6305 }),
																kvp("roomname", "体验房(极速场)"),
																kvp("tablecount", b_int32{ 100 }),
																kvp("floorscore", b_int64{ 0 }),
																kvp("ceilscore", b_int64{ 0 }),
																kvp("minplayernum", b_int32{ 2 }),
																kvp("maxplayernum", b_int32{ 5 }),
																kvp("enterminscore", b_int64{ 0 }),
																kvp("entermaxscore", b_int64{ -1 }),
																kvp("broadcastscore", b_int64{ 20000 }),
																kvp("enableandroid", b_int32{ 0 }),
																kvp("androidcount", b_int32{ 30 }),
																kvp("androidmaxusercount", b_int32{ 80 }),
																kvp("totalstock", b_int64{ -1 }),
																kvp("totalstocklowerlimit", b_int64{ -1 }),
																kvp("totalstockhighlimit", b_int64{ -1 }),
																kvp("systemkillallratio", b_int32{ 0 }),
																kvp("systemreduceratio", b_int32{ 0 }),
																kvp("changecardratio", b_int32{ 0 }),
																kvp("maxjettonscore", b_int64{ 2000 }),
																kvp("jettons", make_array(b_int64{ 100 },
																	b_int64{ 200 },
																	b_int64{ 300 },
																	b_int64{ 400 },
																	b_int64{ 500 },
																	b_int64{ 600 },
																	b_int64{ 700 })),
																kvp("status", b_int32{ 1 })),
															make_document(kvp("roomid", b_int32{ 6306 }),
																kvp("roomname", "初级房(极速场)"),
																kvp("tablecount", b_int32{ 100 }),
																kvp("floorscore", b_int64{ 0 }),
																kvp("ceilscore", b_int64{ 0 }),
																kvp("minplayernum", b_int32{ 2 }),
																kvp("maxplayernum", b_int32{ 5 }),
																kvp("enterminscore", b_int64{ 0 }),
																kvp("entermaxscore", b_int64{ -1 }),
																kvp("broadcastscore", b_int64{ 20000 }),
																kvp("enableandroid", b_int32{ 0 }),
																kvp("androidcount", b_int32{ 30 }),
																kvp("androidmaxusercount", b_int32{ 80 }),
																kvp("totalstock", b_int64{ -1 }),
																kvp("totalstocklowerlimit", b_int64{ -1 }),
																kvp("totalstockhighlimit", b_int64{ -1 }),
																kvp("systemkillallratio", b_int32{ 0 }),
																kvp("systemreduceratio", b_int32{ 0 }),
																kvp("changecardratio", b_int32{ 0 }),
																kvp("maxjettonscore", b_int64{ 2000 }),
																kvp("jettons", make_array(b_int64{ 500 },
																	b_int64{ 1000 },
																	b_int64{ 1500 },
																	b_int64{ 2000 },
																	b_int64{ 2500 },
																	b_int64{ 3000 },
																	b_int64{ 3500 })),
																kvp("status", b_int32{ 1 })),
															make_document(kvp("roomid", b_int32{ 6307 }),
																kvp("roomname", "中级房(极速场)"),
																kvp("tablecount", b_int32{ 100 }),
																kvp("floorscore", b_int64{ 0 }),
																kvp("ceilscore", b_int64{ 0 }),
																kvp("minplayernum", b_int32{ 2 }),
																kvp("maxplayernum", b_int32{ 5 }),
																kvp("enterminscore", b_int64{ 0 }),
																kvp("entermaxscore", b_int64{ -1 }),
																kvp("broadcastscore", b_int64{ 20000 }),
																kvp("enableandroid", b_int32{ 0 }),
																kvp("androidcount", b_int32{ 30 }),
																kvp("androidmaxusercount", b_int32{ 80 }),
																kvp("totalstock", b_int64{ -1 }),
																kvp("totalstocklowerlimit", b_int64{ -1 }),
																kvp("totalstockhighlimit", b_int64{ -1 }),
																kvp("systemkillallratio", b_int32{ 0 }),
																kvp("systemreduceratio", b_int32{ 0 }),
																kvp("changecardratio", b_int32{ 0 }),
																kvp("maxjettonscore", b_int64{ 2000 }),
																kvp("jettons", make_array(b_int64{ 2000 },
																	b_int64{ 4000 },
																	b_int64{ 6000 },
																	b_int64{ 8000 },
																	b_int64{ 10000 },
																	b_int64{ 12000 },
																	b_int64{ 14000 })),
																kvp("status", b_int32{ 1 })),
															make_document(kvp("roomid", b_int32{ 6308 }),
																kvp("roomname", "高级房(极速场)"),
																kvp("tablecount", b_int32{ 100 }),
																kvp("floorscore", b_int64{ 0 }),
																kvp("ceilscore", b_int64{ 0 }),
																kvp("minplayernum", b_int32{ 2 }),
																kvp("maxplayernum", b_int32{ 5 }),
																kvp("enterminscore", b_int64{ 0 }),
																kvp("entermaxscore", b_int64{ -1 }),
																kvp("broadcastscore", b_int64{ 20000 }),
																kvp("enableandroid", b_int32{ 0 }),
																kvp("androidcount", b_int32{ 30 }),
																kvp("androidmaxusercount", b_int32{ 80 }),
																kvp("totalstock", b_int64{ -1 }),
																kvp("totalstocklowerlimit", b_int64{ -1 }),
																kvp("totalstockhighlimit", b_int64{ -1 }),
																kvp("systemkillallratio", b_int32{ 0 }),
																kvp("systemreduceratio", b_int32{ 0 }),
																kvp("changecardratio", b_int32{ 0 }),
																kvp("maxjettonscore", b_int64{ 2000 }),
																kvp("jettons", make_array(b_int64{ 5000 },
																	b_int64{ 10000 },
																	b_int64{ 15000 },
																	b_int64{ 20000 },
																	b_int64{ 25000 },
																	b_int64{ 30000 },
																	b_int64{ 35000 })),
																kvp("jettons", make_array(b_int64{ 5000 },
																	b_int64{ 10000 },
																	b_int64{ 15000 },
																	b_int64{ 20000 },
																	b_int64{ 25000 },
																	b_int64{ 30000 },
																	b_int64{ 35000 })),
																kvp("androidPercentage", make_array()),
																kvp("status", b_int32{ 1 }))

												)));

												bsoncxx::document::value doc2 = make_document(kvp("gameid", b_int32{ 630 }),
													kvp("gamename", "十三水"),
													kvp("sort", b_int32{ 1 }),
													kvp("servicename", "Game_s13s"),
													kvp("revenueRatio", b_int32{ 5 }),
													kvp("type", b_int32{ 0 }),
													kvp("ishot", b_int32{ 0 }),
													kvp("status", b_int32{ 1 }),
													kvp("updatePlayerNum", b_int32{ 30 }),
													kvp("rooms", make_array(
														make_document(kvp("roomid", b_int32{ 6301 }),
															kvp("roomname", "体验房(常规场)"),
															kvp("tablecount", b_int32{ 100 }),
															kvp("floorscore", b_int64{ 0 }),
															kvp("ceilscore", b_int64{ 0 }),
															kvp("minplayernum", b_int32{ 2 }),
															kvp("maxplayernum", b_int32{ 5 }),
															kvp("enterminscore", b_int64{ 0 }),
															kvp("entermaxscore", b_int64{ -1 }),
															kvp("broadcastscore", b_int64{ 20000 }),
															kvp("enableandroid", b_int32{ 0 }),
															kvp("androidcount", b_int32{ 30 }),
															kvp("androidmaxusercount", b_int32{ 80 }),
															kvp("totalstock", b_int64{ -1 }),
															kvp("totalstocklowerlimit", b_int64{ -1 }),
															kvp("totalstockhighlimit", b_int64{ -1 }),
															kvp("systemkillallratio", b_int32{ 0 }),
															kvp("systemreduceratio", b_int32{ 0 }),
															kvp("changecardratio", b_int32{ 0 }),
															kvp("maxjettonscore", b_int64{ 2000 }),
															kvp("jettons", make_array(b_int64{ 100 },
																b_int64{ 200 },
																b_int64{ 300 },
																b_int64{ 400 },
																b_int64{ 500 },
																b_int64{ 600 },
																b_int64{ 700 })),
															kvp("status", b_int32{ 1 }))
													)));
    //         auto collection = conn["gamemain"]["game_log"];

    //        int32_t index = 0;
    //        for(;index<100;index++)
    //        {
    //            bsoncxx::document::value doc = make_document(kvp("gamelogid","kjasdjkasdjkasdjklasdjkl"),
    //                                                         kvp("logdate",b_date{std::chrono::seconds{(uint32_t)time(NULL)}}),
    //                                                         kvp("userid",b_int64{859103068}),
    //                                                         kvp("account","test859103068"),
    //                                                         kvp("agentid",b_int32{1001}),
    //                                                         kvp("gameid",b_int32{930}),
    //                                                         kvp("roomid",b_int32{9301+index%4}),
    //                                                         kvp("winscore",b_int64{8965}),
    //                                                         kvp("revenue",b_int64{155}));
    //             collection.insert_one(doc.view());
    //        }

    //    auto collection = conn["gamemain"]["add_score_order"];


    //    bsoncxx::document::value doc = make_document(
    //                                                 kvp("createtime",b_date{std::chrono::seconds{(uint32_t)time(NULL)}}),
    //                                                 kvp("userid",b_int64{859103068}),
    //                                                 kvp("account","test859103068"),
    //                                                 kvp("agentid",b_int32{1001}),
    //                                                 kvp("score",b_int64{98989}),
    //                                                 kvp("orderid","asjk12iuiu31ff23jasj12"),
    //                                                 kvp("status",b_int32{0}),
    //                                                 kvp("finishtime",b_date{std::chrono::seconds{(uint32_t)time(NULL)}}));
	collection.insert_one(doc2.view());
	//collection.insert_one(doc.view());
	

    printf("done \n");
}
