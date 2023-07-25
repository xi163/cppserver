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

	/*explicit*/ Random::Random() :impl_(New<RandomImpl>()) {
		//placement new
	}

	/*explicit*/ Random::Random(int a, int b) :impl_(New<RandomImpl, int>(a, b)) {
	}

	/*explicit*/ Random::Random(int64_t a, int64_t b) :impl_(New<RandomImpl, int64_t>(a, b)) {
	}

	/*explicit*/ Random::Random(float a, float b) :impl_(New<RandomImpl, float>(a, b)) {
	}

	Random::~Random() {
		Delete<RandomImpl>(impl_);
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

	Weight::Weight() :impl_(New<WeightImpl>()) {
		//placement new
	}

	Weight::~Weight() {
		Delete<WeightImpl>(impl_);
	}

	void Weight::init(int weight[], int len) {
	}

	void Weight::shuffle() {

	}

	int Weight::getResult(bool bv) {
	}

	void Weight::recover(int weight[], int len) {

	}
}