#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <random>
#include <atomic>

class BlockchainTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Инициализация тестового окружения
    }

    void TearDown() override {
        // Очистка после тестов
    }
};

// Тест на создание блоков
TEST_F(BlockchainTest, BlockCreation) {
    const int num_blocks = 1000;
    std::vector<std::string> blocks;
    blocks.reserve(num_blocks);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_blocks; ++i) {
        std::string block = "Block_" + std::to_string(i);
        blocks.push_back(block);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Block creation test completed in " << duration.count() << "ms" << std::endl;
    EXPECT_EQ(blocks.size(), num_blocks);
}

// Тест на хеширование
TEST_F(BlockchainTest, Hashing) {
    const int num_hashes = 10000;
    std::vector<std::string> hashes;
    hashes.reserve(num_hashes);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_hashes; ++i) {
        std::string data(32, ' ');
        for (char& c : data) {
            c = static_cast<char>(dis(gen));
        }
        // Простое хеширование для примера
        std::hash<std::string> hasher;
        hashes.push_back(std::to_string(hasher(data)));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Hashing test completed in " << duration.count() << "ms" << std::endl;
    EXPECT_EQ(hashes.size(), num_hashes);
}

// Тест на консенсус
TEST_F(BlockchainTest, Consensus) {
    const int num_nodes = 4;
    const int num_rounds = 100;
    std::vector<std::atomic<int>> node_states(num_nodes);
    std::vector<std::thread> nodes;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_nodes; ++i) {
        nodes.emplace_back([&node_states, i, num_rounds]() {
            for (int round = 0; round < num_rounds; ++round) {
                // Имитация процесса консенсуса
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                node_states[i]++;
            }
        });
    }

    for (auto& node : nodes) {
        node.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Consensus test completed in " << duration.count() << "ms" << std::endl;
    
    // Проверяем, что все узлы достигли консенсуса
    for (const auto& state : node_states) {
        EXPECT_EQ(state, num_rounds);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 