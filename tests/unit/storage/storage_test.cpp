#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <random>

class StorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Инициализация тестового окружения
    }

    void TearDown() override {
        // Очистка после тестов
    }
};

// Тест на запись данных
TEST_F(StorageTest, WritePerformance) {
    const int num_entries = 10000;
    std::map<std::string, std::vector<char>> storage;
    std::mutex storage_mutex;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_entries; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::vector<char> value(1024); // 1KB
        for (char& c : value) {
            c = static_cast<char>(dis(gen));
        }
        
        std::lock_guard<std::mutex> lock(storage_mutex);
        storage[key] = std::move(value);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Write performance test completed in " << duration.count() << "ms" << std::endl;
    EXPECT_EQ(storage.size(), num_entries);
}

// Тест на чтение данных
TEST_F(StorageTest, ReadPerformance) {
    const int num_entries = 10000;
    std::map<std::string, std::vector<char>> storage;
    std::mutex storage_mutex;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    // Подготовка данных
    for (int i = 0; i < num_entries; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::vector<char> value(1024); // 1KB
        for (char& c : value) {
            c = static_cast<char>(dis(gen));
        }
        storage[key] = std::move(value);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_entries; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::lock_guard<std::mutex> lock(storage_mutex);
        auto it = storage.find(key);
        EXPECT_NE(it, storage.end());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Read performance test completed in " << duration.count() << "ms" << std::endl;
}

// Тест на параллельный доступ
TEST_F(StorageTest, ConcurrentAccess) {
    const int num_threads = 4;
    const int operations_per_thread = 1000;
    std::map<std::string, std::vector<char>> storage;
    std::mutex storage_mutex;
    std::vector<std::thread> threads;
    std::atomic<int> successful_operations{0};

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&storage, &storage_mutex, &successful_operations, operations_per_thread, i]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 1);

            for (int j = 0; j < operations_per_thread; ++j) {
                std::string key = "key_" + std::to_string(i) + "_" + std::to_string(j);
                
                if (dis(gen) == 0) { // Write
                    std::vector<char> value(1024, static_cast<char>(i));
                    std::lock_guard<std::mutex> lock(storage_mutex);
                    storage[key] = std::move(value);
                    successful_operations++;
                } else { // Read
                    std::lock_guard<std::mutex> lock(storage_mutex);
                    auto it = storage.find(key);
                    if (it != storage.end()) {
                        successful_operations++;
                    }
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Concurrent access test completed in " << duration.count() << "ms" << std::endl;
    EXPECT_GT(successful_operations, 0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 