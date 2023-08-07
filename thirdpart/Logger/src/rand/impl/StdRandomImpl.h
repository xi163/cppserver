#ifndef INCLUDE_STD_RANDOMIMPL_H
#define INCLUDE_STD_RANDOMIMPL_H

#include "../../Macro.h"
#include "../../log/impl/LoggerImpl.h"

#define _RANDOM STD::RandomImpl::instance
#define _WEIGHT STD::WeightImpl::instance

//#define DEBUG_PRINT

namespace STD {
	static void test001();
	static void test002(char const* filename);

	//////////////////////////////////////
	//GeneratorImpl
	class GeneratorImpl {
	public:
		GeneratorImpl()
			: mt_({ std::random_device{}() })
			, re_({ std::random_device{}() }) {
		}
		static GeneratorImpl& instance() {
			static GeneratorImpl gen_;
			return gen_;
		}
		std::mt19937& get_mt() {
			return mt_;
		}
		std::default_random_engine& get_re() {
			return re_;
		}
	private:
		std::mt19937 mt_;
		std::default_random_engine re_;
	};

	//////////////////////////////////////
	//RandomImpl
	using RangeInt = std::uniform_int_distribution<>::param_type;
	using RangeInt64 = std::uniform_int_distribution<int64_t>::param_type;
	using RangeFloat = std::uniform_real_distribution<float>::param_type;
	class RandomImpl {
	public:
		explicit RandomImpl() {}
		explicit RandomImpl(int a, int b)
			: iValue_(a, b) {}
		explicit RandomImpl(int64_t a, int64_t b)
			: iValue64_(a, b) {}
		explicit RandomImpl(float a, float b)
			: fValue_(a, b) {}
		static RandomImpl& instance() {
			static RandomImpl r_;
			return r_;
		}
		//整数范围a+rand()%(b-a+1)
		RandomImpl& betweenInt(int a, int b) {
			iValue_.param(RangeInt{ a, b });
			return *this;
		}
		RandomImpl& betweenInt64(int64_t a, int64_t b) {
			iValue64_.param(RangeInt64{ a, b });
			return *this;
		}
		//浮点范围
		RandomImpl& betweenFloat(float a, float b) {
			fValue_.param(RangeFloat{ a, b });
			return *this;
		}
	public:
		int randInt_mt(bool bv = false) {
			return iValue_(bv ? STD::GeneratorImpl::instance().get_mt() : inst_.get_mt());
		}
		int64_t randInt64_mt(bool bv = false) {
			return iValue64_(bv ? STD::GeneratorImpl::instance().get_mt() : inst_.get_mt());
		}
		int randInt_re(bool bv = false) {
			return iValue_(bv ? STD::GeneratorImpl::instance().get_re() : inst_.get_re());
		}
		int64_t randInt64_re(bool bv = false) {
			return iValue64_(bv ? STD::GeneratorImpl::instance().get_re() : inst_.get_re());
		}
		float randFloat_mt(bool bv = false) {
			return fValue_(bv ? STD::GeneratorImpl::instance().get_mt() : inst_.get_mt());
		}
		float randFloat_re(bool bv = false) {
			return fValue_(bv ? STD::GeneratorImpl::instance().get_re() : inst_.get_re());
		}
	private:
		STD::GeneratorImpl inst_;
		std::uniform_int_distribution<> iValue_;
		std::uniform_int_distribution<int64_t> iValue64_;
		std::uniform_real_distribution<float> fValue_;
	};

	//////////////////////////////////////
	//WeightImpl
	class WeightImpl {
	public:
		WeightImpl() :rand_() {
			sum_ = 0;
		}
		static WeightImpl& instance() {
			static WeightImpl w_;
			return w_;
		}
		void init(int weight[], int len) {
			indxId_.resize(len);
			weight_.resize(len);
			sum_ = 0;
			for (int i = 0; i < weight_.size(); ++i) {
				weight_[i] = weight[i];
				indxId_[i] = i;
				sum_ += weight[i];
			}
			assert(sum_ >= 1);
			rand_.betweenInt(1, sum_);
		}
		void shuffle() {
			for (int i = weight_.size() - 1; i > 0; --i) {
				std::uniform_int_distribution<decltype(i)> d(0, i);
				int j = d(STD::GeneratorImpl::instance().get_mt());
				std::swap(weight_[i], weight_[j]);
				std::swap(indxId_[i], indxId_[j]);
			}
		}
		//按权值来随机，返回索引
		int getResult(bool bv = false) {
			assert(weight_.size() > 0);
#ifdef DEBUG_PRINT
			for (int i = 0; i < weight_.size(); ++i) {
				__LOG_DEBUG("w[%d]=%d\n", indxId_[i], weight_[i]);
			}
#endif
			int r = rand_.randInt_mt(bv), c = r;
			for (int i = 0; i < weight_.size(); ++i) {
				c -= weight_[i];
				if (c <= 0) {
#ifdef DEBUG_PRINT
					__LOG_DEBUG("sum=%d r=%d i=%d\n", sum_, r, indxId_[i]);
					__LOG_DEBUG("-------------------------\n\n\n");
#endif
					return indxId_[i];
				}
			}
		}
		//权重输出
		void recover(int weight[], int len) {
			assert(len >= weight_.size());
			for (int i = 0; i < weight_.size(); ++i) {
				weight[indxId_[i]] = weight_[i];
			}
		}
	public:
		STD::RandomImpl rand_;   //随机数值
		int sum_;				 //权值之和
		std::vector<int> weight_;//权重集合
		std::vector<int> indxId_;//对应索引
	};

	static void test001() {

		//随机数[5,10]
		STD::RandomImpl r1(5, 10);
		r1.randInt_mt();

		//随机数[5,10]
		STD::RandomImpl r2;
		r2.betweenInt(5, 10).randInt_mt();

		//随机浮点数[0.1,0.9]
		STD::RandomImpl r3(0.1f, 0.9f);
		r3.randFloat_mt();

		//随机浮点数[0.1,0.9]
		STD::RandomImpl r4;
		r4.betweenFloat(0.1f, 0.9f).randFloat_mt();

		//概率分别为30, 20, 50
		int weight[3] = { 30,20,50 };
		STD::WeightImpl w;
		//初始化
		w.init(weight, 3);
		//随机10次
		for (int i = 0; i < 10; ++i) {
			//权值随机重排，可以不调用
			w.shuffle();
			//返回随机索引
			w.getResult();
		}
	}

	//测试按权重随机概率结果
	//写入文件再导入Excel并插入图表查看概率正态分布情况
	//filename char const* 要写入的文件 如/home/testweight.txt
	static void test002(char const* filename) {
		while (1) {
			if ('q' == getchar()) {
				break;
			}
			remove(filename);
			FILE* fp = fopen(filename, "a");
			if (fp == NULL) {
				return;
			}
			int c = 1000;			//循环总次数
			int scale = 10;			//放大倍数
			int ratioExC = 25;		//换牌概率
			int exC = 0, noExC = 0; //换牌/不换牌分别统计次数
			//概率分别为ratioExC, 100 - ratioExC
			int weight[2] = { ratioExC*scale,(100 - ratioExC)*scale };
			STD::WeightImpl w;
			//初始化
			w.init(weight, 2);
			//随机c次
			for (int i = 0; i < c; ++i) {
				//权值随机重排，可以不调用
				w.shuffle();
				//返回随机索引
				int index = w.getResult();
				if (index == 0) {
					++exC;
				}
				else if (index == 1) {
					++noExC;
				}
				//写入文件再导入Excel并插入图表查看概率正态分布情况
				char ch[10] = { 0 };
				sprintf(ch, "%d\t", index == 0 ? 1 : -1);
				fwrite(ch, strlen(ch), 1, fp);
			}
			fflush(fp);
			fclose(fp);
			__LOG_DEBUG("c:%d:%d scale:%d ratioExC:%d exC:%d:ratio:%.02f",
				c, exC + noExC, scale, ratioExC,
				exC, ((float)exC) / (float)(exC + noExC));
		}
	}
}

#endif