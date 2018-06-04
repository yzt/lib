
#include "y_lockfree.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
using std::cout;

double Now () {
    using namespace std::chrono;
    return duration_cast<duration<double>>(high_resolution_clock::now().time_since_epoch()).count();
}

int main () {

	cout << "[Basic example]\n";
    {
        constexpr unsigned Count = 128;
        char mem [Count * sizeof(std::string)];
        y::Lockfree::Queue<std::string> queue {(std::string *)mem, Count};

        queue.put("Hello");
        queue.put(" cruel ");
        queue.put("world.");

        cout << queue.get().value() << queue.get().value() << queue.get().value() << '\n';
        cout << queue.get().has_value() << '\n';
    }

	cout << "\n[SPSC example 1]\n";
    {
        constexpr unsigned Count = 16;
        auto mem = new long long [Count];

        {
            y::Lockfree::Queue<long long> q {mem, Count};

            constexpr long long N = 10'000'000;
            double insert_time = 0;
            long long insert_fails = 0;
            double extract_time = 0;
            long long extract_duds = 0;

            std::thread inserter ([&]{
                long long cnt = 0, fails = 0;
                auto t0 = Now();
                while (cnt < N) {
                    if (q.put(cnt)) {
                        cnt += 1;
                    } else {
                        fails += 1;
                        std::this_thread::yield();
                    }
                }
                auto t1 = Now();
                insert_time = t1 - t0;
                insert_fails = fails;
            });

            std::thread extractor ([&]{
                long long duds = 0, cnt = 0;
                auto t0 = Now();
                while (cnt < N) {
                    auto ov = q.get();
                    if (ov.has_value()) {
                        assert(*ov == cnt);
                        cnt += 1;
                    } else {
                        duds += 1;
                        std::this_thread::yield();
                    }
                }
                auto t1 = Now();
                extract_time = t1 - t0;
                extract_duds = duds;
            });
            
            inserter.join();
            extractor.join();

            cout << q.empty() << '\n';
            cout << "N = " << N << " Queue Cap = " << Count << ", insert time = " << insert_time << ", insert failures = " << insert_fails << ", extract time = " << extract_time << ", duds = " << extract_duds << '\n';
            cout << "insert time = " << int(0.5 + (insert_time * 1'000'000'000.0 / N)) << " ns per item, extract time = " << int(0.5 + (extract_time * 1'000'000'000.0 / N)) << " ns per item\n";
        }

        delete[] mem;
    }

	cout << "\n[SPSC example 2]\n";
    {
        constexpr unsigned Count = 32'768;
        auto mem = new long long [Count];

        {
            y::Lockfree::Queue<long long> q {mem, Count};

            constexpr long long N = 10'000'000;
            double insert_time = 0;
            long long insert_fails = 0;
            double extract_time = 0;
            long long extract_duds = 0;

            std::thread inserter ([&]{
                long long cnt = 0, fails = 0;
                auto t0 = Now();
                while (cnt < N) {
                    if (q.put(cnt)) {
                        cnt += 1;
                    } else {
                        fails += 1;
                        std::this_thread::yield();
                    }
                }
                auto t1 = Now();
                insert_time = t1 - t0;
                insert_fails = fails;
            });

            std::thread extractor ([&]{
                long long duds = 0, cnt = 0;
                auto t0 = Now();
                while (cnt < N) {
                    auto ov = q.get();
                    if (ov.has_value()) {
                        assert(*ov == cnt);
                        cnt += 1;
                    } else {
                        duds += 1;
                        std::this_thread::yield();
                    }
                }
                auto t1 = Now();
                extract_time = t1 - t0;
                extract_duds = duds;
            });
            
            inserter.join();
            extractor.join();

            cout << q.empty() << '\n';
            cout << "N = " << N << " Queue Cap = " << Count << ", insert time = " << insert_time << ", insert failures = " << insert_fails << ", extract time = " << extract_time << ", duds = " << extract_duds << '\n';
            cout << "insert time = " << int(0.5 + (insert_time * 1'000'000'000.0 / N)) << " ns per item, extract time = " << int(0.5 + (extract_time * 1'000'000'000.0 / N)) << " ns per item\n";
        }

        delete[] mem;
    }

	cout << "\n[MPMC example]\n";
    {
        constexpr unsigned Count = 16'384;
        auto mem = new long long [Count];

        {
            constexpr long long N = 10'000'000;
            constexpr int I = 4, E = 4;

			std::atomic_int done_inserters = 0;
            y::Lockfree::Queue<long long> q {mem, Count};

            auto inserter_func = [&](int idx){
                long long cnt = 0, fails = 0;
                auto t0 = Now();
                while (cnt < N) {
                    if (q.put(idx)) {
                        cnt += 1;
                    } else {
                        fails += 1;
                        std::this_thread::yield();
                    }
                }
                done_inserters += 1;
                auto t1 = Now();
                
				fprintf(stdout, " [INSERTER] #%-2d:  inserted %llu in %.2fs with %llu failures (%d ns per item.)\n", idx, cnt, (t1 - t0), fails, int(0.5 + ((t1 - t0) * 1'000'000'000.0 / N)));
				fflush(stdout);
            };

            auto extractor_func = [&](int idx){
                long long fails = 0, cnt = 0;
                auto t0 = Now();
                while (done_inserters < I || !q.empty()) {
                    auto ov = q.get();
                    if (ov.has_value()) {
                        cnt += 1;
                    } else {
                        fails += 1;
                        std::this_thread::yield();
                    }
                }
                auto t1 = Now();
                
				fprintf(stdout, "[EXTRACTOR] #%-2d: extracted %llu in %.2fs with %llu failures (%d ns per item.)\n", idx, cnt, (t1 - t0), fails, int(0.5 + ((t1 - t0) * 1'000'000'000.0 / N)));
				fflush(stdout);
            };
            
			std::vector<std::thread> threads;
			threads.reserve(I + E);
			for (int i = 0; i < I; ++i)
				threads.emplace_back(inserter_func, i);
			for (int i = 0; i < E; ++i)
				threads.emplace_back(extractor_func, i);
		
			for (auto & t : threads)
				t.join();
				
            cout << "Done. " << q.empty() << '\n';
        }

        delete[] mem;
    }

    return 0;
}
