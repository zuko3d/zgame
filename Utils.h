#pragma once

#include <algorithm>
#include <array>
#include <vector>
#include <numeric>
#include <random>
#include <assert.h>

inline std::string workerPic(int idx) {
    return "<img height=132px width=120px src =\"images/w" + std::to_string(idx) + ".png\">";
}

inline double urand(std::default_random_engine& generator, double min, double max) {
  return std::uniform_real_distribution<double>(min, max)(generator);
}

inline double nrand(std::default_random_engine& generator) {
  static std::normal_distribution<double> distribution(0.0, 1.0);
  return distribution(generator);
}

inline double lnrand(std::default_random_engine& generator) {
  static std::lognormal_distribution<double> distribution(0.0, 1.0);
  return distribution(generator);
}

template <class C>
inline bool is_in(const C& val, const std::vector<C>& vec) {
	return std::find(vec.begin(), vec.end(), val) != vec.end();
}

template <class C>
inline void rshuffle(C& vec, std::default_random_engine& g) {
	std::shuffle(vec.begin(), vec.end(), g);
}

template <class Container>
inline size_t countNonZero(const Container& vec) {
    return std::count_if(vec.begin(), vec.end(),
        [](const int amnt) { return amnt != 0; }
    );
}

template <class Container>
inline typename Container::value_type sum(const Container& vec) {
    typename Container::value_type ret = 0;
	for (const auto& c: vec) {
        ret += c;
    }

    return ret;
}

template <class Container>
inline typename Container::value_type abssum(const Container& vec) {
    typename Container::value_type ret = 0;
	for (const auto& c: vec) {
		if (c > 0) {
        	ret += c;
		} else {
			ret -= c;
		}
    }

    return ret;
}

template <class Container>
inline double mean(const Container& vec) {
    return ((double) sum(vec)) / vec.size();
}

template <class Container>
inline double variance(const Container& vec) {
	const double avg = mean(vec);
	double sumDiff = 0;
	for (const auto& c: vec) {
		sumDiff += (c - avg) * (c - avg);
	}
    return sqrt(sumDiff / vec.size());
}

template <class Container>
inline typename Container::valuy_type median(Container vec) {
    std::sort(vec.begin(), vec.end());
    return vec.at(vec.size() / 2);
}

template <class Container>
inline typename Container::value_type maximum(const Container& vec) {
    return *std::max_element(vec.begin(), vec.end());
}

template <class Container>
inline typename Container::value_type percentile(Container vec, double pct) {
    std::sort(vec.begin(), vec.end());
	assert((pct <= 1.0) && (pct >= 0.0));
    return vec.at((vec.size() - 1) * pct);
}

template<typename Container>
inline std::vector<size_t> argsort(const Container& array) {
	std::vector<size_t> indices(array.size());
	std::iota(indices.begin(), indices.end(), 0);
	std::sort(indices.begin(), indices.end(),
		[&array](int left, int right) -> bool {
		// sort indices according to corresponding array element
		return array.at(left) < array.at(right);
	});

	return indices;
}

template<typename Container>
inline std::vector<size_t> argsortDescending(const Container& array) {
	std::vector<size_t> indices(array.size());
	std::iota(indices.begin(), indices.end(), 0);
	std::sort(indices.begin(), indices.end(),
		[&array](int left, int right) -> bool {
		// sort indices according to corresponding array element
		return array.at(left) > array.at(right);
	});

	return indices;
}

template <class T>
class Enumerator {
public:
	struct Iterator {
	public:
		Iterator(const T* cont, size_t i)
			: cont_(cont)
			, idx(i)
		{ }

		std::pair<const T&, size_t> operator*() const {
			return { cont_[idx], idx };
		}

		void operator++() {
			idx++;
		}

		bool operator!=(const Iterator& rhs) const {
			assert(cont_ == rhs.cont_);

			return idx != rhs.idx;
		}

		bool operator==(const Iterator& rhs) const {
			assert(cont_ == rhs.cont_);

			return idx == rhs.idx;
		}

	private:
		const T* cont_;
		size_t idx;
	};

	Enumerator(const T* begin, size_t size)
		: begin_(begin, 0)
		, end_(begin, size)
	{ }

	const Iterator& begin() const {
		return begin_;
	}

	const Iterator& end() const {
		return end_;
	}

	Iterator begin() {
		return begin_;
	}

	Iterator end() {
		return end_;
	}

private:
	const Iterator begin_;
	const Iterator end_;
};

template <class T1, class T2>
class Zipper {
public:
	struct ZipIterator {
	public:
		ZipIterator(const T1* begin1, const T2* begin2, size_t i)
			: begin1_(begin1)
			, begin2_(begin2)
			, idx(i)
		{ }

		std::pair<const T1&, const T2&> operator*() const {
			return { begin1_[idx], begin2_[idx] };
		}

		void operator++() {
			idx++;
		}

		bool operator!=(const ZipIterator& rhs) const {
			assert(begin1_ == rhs.begin1_);
			assert(begin2_ == rhs.begin2_);

			return idx != rhs.idx;
		}

		bool operator==(const ZipIterator& rhs) const {
			assert(begin1_ == rhs.begin1_);
			assert(begin2_ == rhs.begin2_);

			return idx == rhs.idx;
		}

	private:
		const T1* begin1_;
		const T2* begin2_;
		size_t idx;
	};

	Zipper(const T1* begin1, const T2* begin2, size_t size)
		: begin_(begin1, begin2, 0)
		, end_(begin1, begin2, size)
	{ }

	const ZipIterator& begin() const {
		return begin_;
	}

	const ZipIterator& end() const {
		return end_;
	}

	ZipIterator begin() {
		return begin_;
	}

	ZipIterator end() {
		return end_;
	}

private:
	const ZipIterator begin_;
	const ZipIterator end_;
};



template <class T>
class Reverser {
public:
	struct Iterator {
	public:
		Iterator(const std::vector<T>& cont, size_t i)
			: cont_(cont)
			, idx(i)
		{ }

		const T& operator*() {
			return cont_.at(cont_.size() - idx - 1);
		}

		void operator++() {
			idx++;
		}

		bool operator!=(const Iterator& rhs) const {
			return idx != rhs.idx;
		}

		bool operator==(const Iterator& rhs) const {
			return idx == rhs.idx;
		}

	private:
		const std::vector<T>& cont_;
		size_t idx;
	};

	Reverser(const std::vector<T>& cont)
		: cont_(cont)
	{ }

	Iterator begin() const {
		return Iterator{ cont_, 0 };
	}

	Iterator end() const {
		return Iterator(cont_, (size_t)cont_.size());
	}

private:
	const std::vector<T>& cont_;
};

template <class Container>
inline Enumerator<std::remove_reference_t<typename Container::value_type>> enumerate(const Container& cont) {
	return Enumerator<std::remove_reference_t<typename Container::value_type>>(&cont.front(), cont.size());
}

template <class Container1, class Container2>
inline Zipper<std::remove_reference_t<typename Container1::value_type>, std::remove_reference_t<typename Container2::value_type>>
zip(const Container1& cont1, const Container2& cont2) {
	assert(cont1.size() == cont2.size());
	return {&cont1.front(), cont2.front(), cont1.size()};
}

template <class T>
inline Reverser<T> reversed(const std::vector<T>& cont) {
	return Reverser(cont);
}