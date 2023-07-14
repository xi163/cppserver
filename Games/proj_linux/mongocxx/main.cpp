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

//mongodb://root:123456@192.168.2.212:27017
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

//随机种子
static struct srand_init_t {
	srand_init_t() {
		srand(time(NULL));
	}
}s_srand_init;

//RandomBetween 随机数[a,b]
extern int64_t RandomBetween(int64_t a, int64_t b) {
	return a + rand() % (b - a + 1);
}

//15631
int64_t usrid_start  = 25418270;
int64_t userid_max   = 25548270;

std::map<int64_t, bool> recordUserIDs;

void readFile(char const* filename, std::vector<std::string>& lines, char const* skip);

int main() {
	std::vector<std::string> lines;
	readFile("conf.ini", lines, ";;");
	usrid_start = (int64_t)atol(lines[0].c_str());
	userid_max = (int64_t)atol(lines[1].c_str());
	mongocxx::client conn{ mongocxx::uri{lines[2]} };//conn pwd???
	mongocxx::database db = conn["gameconfig"];//db
	mongocxx::collection collection = db["android_user"];//tblname

	try {
		int c = 0;
		mongocxx::cursor cursor = collection.find({});
		for (mongocxx::cursor::iterator it = cursor.begin();
			it != cursor.end(); ++it) {
			//view
			const bsoncxx::document::view& view = *it;
			//std::cout << bsoncxx::to_json(view) << std::endl;
			const bsoncxx::document::element& elem = view["userId"];
			if (elem.type() != bsoncxx::type::k_int64) {
				std::cout << "elem.type error" << std::endl;
				break;
			}
			//std::cout << "userId " << elem.get_int64() << std::endl;
			//elem.type() == bsoncxx::type::k_utf8;
			//elem.get_utf8().value.to_string();
			//用户ID
			int64_t userId;
			{
				//随机范围的userId [usrid_start+1,usrid_max-1]
				userId = RandomBetween(usrid_start + 1, userid_max - 1);
				std::map<int64_t, bool>::iterator it = recordUserIDs.find(userId);
				//存在了继续随机
				while (it != recordUserIDs.end()) {
					userId = RandomBetween(usrid_start + 1, userid_max - 1);
					it = recordUserIDs.find(userId);
				}
			}
// 			{
// 				userId = --userid_max;
// 			}
			//账号/昵称
			char account[15] = { 0 }, nickname[15] = { 0 };
			sprintf(account, "%d", userId);
			sprintf(nickname, "%d", userId);
			//头像
			int headID = RandomBetween(0, 11);
			bsoncxx::document::value cond = bsoncxx::builder::stream::document{}
				<< "userId"
				<< elem.get_int64()
				<< finalize;
			bsoncxx::document::value doc = bsoncxx::builder::stream::document{}
				<< "$set" << open_document
				<< "userId" << userId
				<< "account" << account
				<< "nickName" << nickname
				<< "headId" << headID
				<< close_document << finalize;
			//更新用户ID/账号/昵称/头像
			bsoncxx::stdx::optional<mongocxx::result::update> result = collection.update_one(cond.view(), doc.view());
			if (result) {
				++c;
				//更新成功，保存到记录
				recordUserIDs[userId] = true;
				//std::cout << "update succ" << std::endl;
			}
			else {
				std::cout << "update failed" << std::endl;
			}
// 			mongocxx::stdx::optional<bsoncxx::document::value> result = collection.find_one({});
// 			bsoncxx::document::view view = (*result).view();
// 			view["userId"].get_utf8().value.to_string();
// 			mongocxx::stdx::optional<bsoncxx::document::value> result =
// 				collection.find_one(bsoncxx::builder::stream::document{} << "userId" << elem.get_int64() << finalize);
// 			std::cout << bsoncxx::to_json((*result).view()) << std::endl;
		}
		std::cout << "total = [" << c << "] update done!" << std::endl;
	}
	catch (bsoncxx::exception &e) {
		std::cout << "bsoncxx::exception except!" << std::endl;
	}
	catch (...) {
		std::cout << "except!" << std::endl;
	}
	return 0;
}

//按行读取文件
void readFile(char const* filename, std::vector<std::string>& lines, char const* skip) {
#if 1
	{
		FILE* fp = fopen(filename, "r");
		if (fp == NULL) {
			return;
		}
		char line[512] = { 0 };
		while (!feof(fp)) {
			if (NULL != fgets(line, 512, fp)) {
				int len = strlen(line);
				if (line[len - 1] == '\r' ||
					(line[len - 2] != '\r' && line[len - 1] == '\n')) {
					//去掉\r或\n结尾
					line[len - 1] = '\0';
					--len;
				}
				else if (line[len - 2] == '\r' && line[len - 1] == '\n') {
					//0 == strcmp(&line[len-2], "\r\n")
					//去掉\r\n结尾
					line[len - 2] = '\0';
					line[len - 1] = '\0';
					len -= 2;
				}
				//printf(line);
				if (line[0] != '\0') {
					if (len >= strlen(skip) && 0 == strncmp(line, skip, strlen(skip))) {
					}
					else {
						lines.push_back(line);
					}
				}
			}
		}
		//printf("--- *** fgets:%d\n", lines.size());
		//for (std::vector<string>::iterator it = lines.begin();
		//	it != lines.end(); ++it) {
		//	printf("%s\n", it->c_str());
		//}
		fclose(fp);
	}
#else
	{
		lines.clear();
		std::fstream fs(filename);
		std::string line;
		while (getline(fs, line)) {
			int len = line.size();
			if (line[len - 1] == '\r' ||
				(line[len - 2] != '\r' && line[len - 1] == '\n')) {
				//去掉\r或\n结尾
				line[len - 1] = '\0';
				--len;
			}
			else if (line[len - 2] == '\r' && line[len - 1] == '\n') {
				//去掉\r\n结尾
				line[len - 2] = '\0';
				line[len - 1] = '\0';
				len -= 2;
			}
			//printf(line.c_str());
			if (line[0] != '\0') {
				if (len >= strlen(skip) &&
					0 == strncmp(&line.front(), skip, strlen(skip))) {
				}
				else {
					lines.push_back(line);
				}
			}
		}
		//printf("--- *** getline:%d\n", lines.size());
		//for (std::vector<string>::iterator it = lines.begin();
		//	it != lines.end(); ++it) {
		//	printf("%s\n", it->c_str());
		//}
	}
#endif
}