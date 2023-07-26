#ifndef INCLUDE_STD_RANDOM_H
#define INCLUDE_STD_RANDOM_H

#include "Logger/src/Macro.h"

namespace STD {

	class GeneratorImpl;
	class Generator {
	public:
		Generator();
		~Generator();
		static Generator& instance() {
			static Generator gen_;
			return gen_;
		}
		std::mt19937& get_mt();
		std::default_random_engine& get_re();
	private:
		GeneratorImpl* impl_;
	};

	class RandomImpl;
	class Random {
		friend class Weight;
	public:
		explicit Random();
		explicit Random(int a, int b);
		explicit Random(int64_t a, int64_t b);
		explicit Random(float a, float b);
		~Random();
		Random& betweenInt(int a, int b);
		Random& betweenInt64(int64_t a, int64_t b);
		Random& betweenFloat(float a, float b);
	public:
		int randInt_mt(bool bv = false);
		int64_t randInt64_mt(bool bv = false);
		int randInt_re(bool bv = false);
		int64_t randInt64_re(bool bv = false);
		float randFloat_mt(bool bv = false);
		float randFloat_re(bool bv = false);
	private:
		explicit Random(RandomImpl* impl);
		void attatch(RandomImpl* impl);
		bool heap_;
		RandomImpl* impl_;
	};

	class WeightImpl;
	class Weight {
	public:
		Weight();
		~Weight();
		Random& rand();
		void init(int weight[], int len);
		void shuffle();
		int getResult(bool bv = false);
		void recover(int weight[], int len);
	private:
		Random rand_;
		WeightImpl* impl_;
	};
}

#endif