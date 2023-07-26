#include "../StdRandom.h"
#include "StdRandomImpl.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../auth/auth.h"

#include "../Macro.h"

namespace STD {

	template <class T> static inline T* New() {
		void* ptr = (void*)malloc(sizeof(T));
		return new(ptr) T();
	}

	template <class T, typename Ty> static inline T* New(Ty a, Ty b) {
		void* ptr = (void*)malloc(sizeof(T));
		return new(ptr) T(a, b);
	}

	template <class T> static inline void Delete(T* ptr) {
		ptr->~T();
		free(ptr);
	}

	Generator::Generator() :impl_(New<GeneratorImpl>()) {
		//placement new
	}

	Generator::~Generator() {
		Delete<GeneratorImpl>(impl_);
	}

	std::mt19937& Generator::get_mt() {
		return impl_->get_mt();
	}

	std::default_random_engine& Generator::get_re() {
		return impl_->get_re();
	}

	/*explicit*/ Random::Random() :impl_(New<RandomImpl>()), heap_(true) {
		//placement new
	}

	/*explicit*/ Random::Random(int a, int b) :impl_(New<RandomImpl, int>(a, b)), heap_(true) {
	}

	/*explicit*/ Random::Random(int64_t a, int64_t b) :impl_(New<RandomImpl, int64_t>(a, b)), heap_(true) {
	}

	/*explicit*/ Random::Random(float a, float b) :impl_(New<RandomImpl, float>(a, b)), heap_(true) {
	}

	/*explicit*/ Random::Random(RandomImpl* impl) :impl_(impl), heap_(false) {

	}

	Random::~Random() {
		switch (heap_) {
		case true:
			Delete<RandomImpl>(impl_);
		default:
			break;
		}
	}

	void Random::attatch(RandomImpl* impl) {
		impl_ = impl;
		heap_ = false;
	}

	Random& Random::betweenInt(int a, int b) {
		impl_->betweenInt(a, b);
		return *this;
	}

	Random& Random::betweenInt64(int64_t a, int64_t b) {
		impl_->betweenInt64(a, b);
		return *this;
	}

	Random& Random::betweenFloat(float a, float b) {
		impl_->betweenFloat(a, b);
		return *this;
	}

	int Random::randInt_mt(bool bv) {
		return impl_->randInt_mt(bv);
	}

	int64_t Random::randInt64_mt(bool bv) {
		return impl_->randInt64_mt(bv);
	}

	int Random::randInt_re(bool bv) {
		return impl_->randInt_re(bv);
	}

	int64_t Random::randInt64_re(bool bv) {
		return impl_->randInt64_re(bv);
	}

	float Random::randFloat_mt(bool bv) {
		return impl_->randFloat_mt(bv);
	}

	float Random::randFloat_re(bool bv) {
		return impl_->randFloat_re(bv);
	}

	Weight::Weight() :impl_(New<WeightImpl>()), rand_(&impl_->rand_) {
		//placement new
	}

	Weight::~Weight() {
		Delete<WeightImpl>(impl_);
	}

	Random& Weight::rand() {
		rand_.attatch(&impl_->rand_);
		return rand_;
	}

	void Weight::init(int weight[], int len) {
		impl_->init(weight, len);
	}

	void Weight::shuffle() {
		impl_->shuffle();
	}

	int Weight::getResult(bool bv) {
		return impl_->getResult(bv);
	}

	void Weight::recover(int weight[], int len) {
		impl_->recover(weight, len);
	}
}