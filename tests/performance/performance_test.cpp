#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <random>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <cmath>
#include <complex>
#include <algorithm>
#include <numeric>
#include <queue>

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Инициализация тестового окружения
    }

    void TearDown() override {
        // Очистка после тестов
    }
};

// Тест на вычислительную мощность
TEST_F(PerformanceTest, ComputationalPower) {
    const int iterations = 5000000; // Увеличено в 5 раз
    std::vector<std::complex<double>> results(iterations);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-10.0, 10.0);

    auto start = std::chrono::high_resolution_clock::now();

    // Выполняем сложные вычисления
    for (int i = 0; i < iterations; ++i) {
        double x = dis(gen);
        double y = dis(gen);
        std::complex<double> z(x, y);
        // Сложные математические операции
        results[i] = std::pow(z, 3) + std::sin(z) * std::cos(z) + std::tan(z) * std::exp(z);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Computational test completed in " << duration.count() << "ms" << std::endl;
    EXPECT_GT(duration.count(), 0);
}

// Тест на многопоточность
TEST_F(PerformanceTest, MultiThreading) {
    const int num_threads = 16; // Увеличено в 4 раза
    const int iterations = 5000000; // Увеличено в 5 раз
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    std::vector<std::atomic<int>> thread_counters(num_threads);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter, &thread_counters, i, iterations]() {
            for (int j = 0; j < iterations; ++j) {
                counter++;
                thread_counters[i]++;
                // Добавляем небольшую нагрузку
                std::this_thread::yield();
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Multi-threading test completed in " << duration.count() << "ms" << std::endl;
    EXPECT_EQ(counter, num_threads * iterations);
    for (const auto& tc : thread_counters) {
        EXPECT_EQ(tc, iterations);
    }
}

// Тест на синхронизацию
TEST_F(PerformanceTest, Synchronization) {
    const int num_threads = 16; // Увеличено в 4 раза
    const int iterations = 1000000; // Увеличено в 10 раз
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> ready{false};
    std::vector<std::thread> threads;
    std::atomic<int> counter{0};
    std::atomic<int> threads_ready{0};
    std::vector<std::atomic<int>> thread_counters(num_threads);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&mtx, &cv, &ready, &counter, &threads_ready, &thread_counters, iterations, i]() {
            threads_ready++;
            
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&ready]() { return ready.load(); });
            }
            
            for (int j = 0; j < iterations; ++j) {
                std::lock_guard<std::mutex> guard(mtx);
                counter++;
                thread_counters[i]++;
            }
        });
    }

    while (threads_ready < num_threads) {
        std::this_thread::yield();
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;
    }
    cv.notify_all();

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Synchronization test completed in " << duration.count() << "ms" << std::endl;
    EXPECT_EQ(counter, num_threads * iterations);
    for (const auto& tc : thread_counters) {
        EXPECT_EQ(tc, iterations);
    }
}

// Тест на операции с памятью
TEST_F(PerformanceTest, MemoryOperations) {
    const int num_operations = 100000;
    const int buffer_size = 1024 * 64; // 64KB
    const int num_threads = 16;
    const int batch_size = 100;
    std::queue<std::vector<char>> buffer_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::atomic<int> processed_buffers{0};
    std::atomic<int> created_buffers{0};
    std::atomic<bool> all_buffers_created{false};
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    auto start = std::chrono::high_resolution_clock::now();

    // Функция для обработки буфера
    auto process_buffer = [&](std::vector<char>& buffer) {
        std::sort(buffer.begin(), buffer.end());
        std::reverse(buffer.begin(), buffer.end());
        std::rotate(buffer.begin(), buffer.begin() + buffer.size()/2, buffer.end());
    };

    // Создаем и запускаем потоки обработки
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            std::vector<std::vector<char>> batch;
            batch.reserve(batch_size);
            while (true) {
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    queue_cv.wait(lock, [&]() {
                        return !buffer_queue.empty() || (all_buffers_created && buffer_queue.empty());
                    });
                    if (all_buffers_created && buffer_queue.empty()) {
                        break;
                    }
                    while (!buffer_queue.empty() && batch.size() < batch_size) {
                        batch.push_back(std::move(buffer_queue.front()));
                        buffer_queue.pop();
                    }
                }
                for (auto& buffer : batch) {
                    process_buffer(buffer);
                    processed_buffers++;
                }
                batch.clear();
            }
        });
    }

    // Создаем и добавляем буферы в очередь
    for (int i = 0; i < num_operations; ++i) {
        std::vector<char> buffer(buffer_size);
        std::generate(buffer.begin(), buffer.end(), [&]() { return dis(gen); });
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            buffer_queue.push(std::move(buffer));
            created_buffers++;
        }
        queue_cv.notify_one();
        if (i % 1000 == 0) {
            std::cout << "Created " << created_buffers << " buffers, processed " << processed_buffers << " buffers" << std::endl;
        }
    }
    all_buffers_created = true;
    queue_cv.notify_all();
    for (auto& thread : threads) {
        thread.join();
    }
    EXPECT_EQ(processed_buffers, num_operations);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Memory operations test completed in " << duration.count() << "ms" << std::endl;
    std::cout << "Created " << created_buffers << " buffers, processed " << processed_buffers << " buffers" << std::endl;
    EXPECT_GT(duration.count(), 0);
}

// Тест на сетевые операции
TEST_F(PerformanceTest, NetworkOperations) {
    const int num_packets = 1000000;
    const int packet_size = 1024;
    std::vector<std::vector<char>> packets(num_packets);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::atomic<int> processed_packets{0};

    auto start = std::chrono::high_resolution_clock::now();

    // Создаем пакеты
    for (int i = 0; i < num_packets; ++i) {
        packets[i].resize(packet_size);
        std::generate(packets[i].begin(), packets[i].end(), [&]() { return dis(gen); });
    }

    // Имитация обработки пакетов
    const int num_threads = 8;
    std::vector<std::thread> threads;
    std::mutex mtx;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&packets, &processed_packets, &mtx, num_packets, i, num_threads]() {
            for (int j = i; j < num_packets; j += num_threads) {
                // Имитация обработки пакета
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                processed_packets++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Network operations test completed in " << duration.count() << "ms" << std::endl;
    EXPECT_EQ(processed_packets, num_packets);
}

// Тест на операции с блокчейном
TEST_F(PerformanceTest, BlockchainOperations) {
    const int num_blocks = 100000;
    const int transactions_per_block = 100;
    std::vector<std::vector<std::string>> blocks(num_blocks);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::atomic<int> processed_blocks{0};

    auto start = std::chrono::high_resolution_clock::now();

    // Создаем блоки с транзакциями
    for (int i = 0; i < num_blocks; ++i) {
        blocks[i].resize(transactions_per_block);
        for (int j = 0; j < transactions_per_block; ++j) {
            std::string transaction(32, ' ');
            std::generate(transaction.begin(), transaction.end(), [&]() { return dis(gen); });
            blocks[i][j] = transaction;
        }
    }

    // Имитация обработки блоков
    const int num_threads = 8;
    std::vector<std::thread> threads;
    std::mutex mtx;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&blocks, &processed_blocks, &mtx, num_blocks, i, num_threads]() {
            for (int j = i; j < num_blocks; j += num_threads) {
                // Имитация обработки блока
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                processed_blocks++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Blockchain operations test completed in " << duration.count() << "ms" << std::endl;
    EXPECT_EQ(processed_blocks, num_blocks);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 