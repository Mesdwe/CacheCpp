#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include <array>


#include "CachePolicy.h"
#include "LFU.h"
#include "LRU.h"
#include "ARC.h"

enum class AccessPattern {
	Hotspot,
	Random,
	// Future: Zipf, Scan
};

namespace Test {
	class Timer {
	public:
		Timer() : start(std::chrono::high_resolution_clock::now()) {}

		double elapsedMs() const {
			auto now = std::chrono::high_resolution_clock::now();
			return std::chrono::duration<double, std::milli>(now - start).count();
		}

	private:
		std::chrono::high_resolution_clock::time_point start;
	};

	class CacheTestRunner {
	public:
		static void Run(int capacity, int operations, AccessPattern pattern);

	private:
		static void RunSingleTest(const std::string& name,
			std::unique_ptr<CacheCpp::ICachePolicy<int, std::string>> cache,
			int capacity,
			int operations,
			AccessPattern pattern);
	};

	void CacheTestRunner::Run(int capacity, int operations, AccessPattern pattern) {
		std::cout << "=== Running Tests [capacity=" << capacity << ", ops=" << operations << "] ===\n";

		RunSingleTest("LRU",
			std::make_unique<CacheCpp::LRUCache<int, std::string>>(capacity),
			capacity, operations, pattern);

		RunSingleTest("LRU-K",
			std::make_unique<CacheCpp::LRUKCache<int, std::string>>(capacity, capacity * 4, 2),
			capacity, operations, pattern);

		RunSingleTest("LRU-Hash",
			std::make_unique<CacheCpp::LRUHashCache<int, std::string>>(capacity, 4),
			capacity, operations, pattern);


		RunSingleTest("LFU",
			std::make_unique<CacheCpp::LFUCache<int, std::string>>(capacity, 900000),
			capacity, operations, pattern);

		RunSingleTest("ARC",
			std::make_unique<CacheCpp::ARCCache<int, std::string>>(capacity, 50),
			capacity, operations, pattern);
	}

	void CacheTestRunner::RunSingleTest(const std::string& name,
		std::unique_ptr<CacheCpp::ICachePolicy<int, std::string>> cache,
		int capacity,
		int operations,
		AccessPattern pattern) {
		const int HOT_KEYS = 20;
		const int COLD_KEYS = 5000;

		std::random_device rd;
		std::mt19937 gen(rd());

		int hit = 0, get_ops = 0;
		Timer timer;

		// Insert phase
		for (int op = 0; op < operations; ++op) {
			int key;
			if (pattern == AccessPattern::Hotspot) {
				key = (op % 100 < 70) ? gen() % HOT_KEYS : HOT_KEYS + (gen() % COLD_KEYS);
			}
			else {
				key = gen() % (HOT_KEYS + COLD_KEYS);
			}

			cache->Put(key, "val" + std::to_string(key));
		}

		// Access phase
		for (int op = 0; op < operations; ++op) {
			int key;
			if (pattern == AccessPattern::Hotspot) {
				key = (op % 100 < 70) ? gen() % HOT_KEYS : HOT_KEYS + (gen() % COLD_KEYS);
			}
			else {
				key = gen() % (HOT_KEYS + COLD_KEYS);
			}

			std::string val;
			get_ops++;
			if (cache->Get(key, val)) hit++;
		}

		double elapsed = timer.elapsedMs();

		std::cout << std::setw(10) << name << " | "
			<< "Hit rate: " << std::setw(6) << std::fixed << std::setprecision(2)
			<< (100.0 * hit / get_ops) << "% | "
			<< "Time: " << std::setw(8) << std::fixed << std::setprecision(2)
			<< elapsed << "ms\n";
	}


}

int main() {
	const int capacity = 50;
	const int operations = 500000;

	Test::CacheTestRunner::Run(capacity, operations, AccessPattern::Hotspot);
	Test::CacheTestRunner::Run(capacity, operations, AccessPattern::Random);

	return 0;
}