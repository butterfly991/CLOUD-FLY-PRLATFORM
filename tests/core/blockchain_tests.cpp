#include <gtest/gtest.h>
#include "core/blockchain/MultiCoreBlockchain.h"
#include "core/drivers/blockchain_ops.h"

using namespace core;

class BlockchainTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        blockchain = std::make_unique<MultiCoreBlockchain>();
        EXPECT_TRUE(blockchain->initialize());
    }

    void TearDown() override
    {
        blockchain.reset();
    }

    std::unique_ptr<MultiCoreBlockchain> blockchain;
};

// Test blockchain initialization
TEST_F(BlockchainTest, Initialization)
{
    EXPECT_TRUE(blockchain->is_initialized());
    EXPECT_EQ(blockchain->get_core_count(), 4); // Default core count
}

// Test transaction processing
TEST_F(BlockchainTest, TransactionProcessing)
{
    Transaction tx;
    tx.type = TransactionType::TRANSFER;
    tx.data = "Test transaction";
    tx.amount = 100;
    tx.sender = "Alice";
    tx.receiver = "Bob";

    size_t tx_id = blockchain->submit_transaction(tx);
    EXPECT_GT(tx_id, 0);

    // Wait for transaction to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TransactionStatus status = blockchain->get_transaction_status(tx_id);
    EXPECT_EQ(status, TransactionStatus::CONFIRMED);
}

// Test block creation and validation
TEST_F(BlockchainTest, BlockOperations)
{
    // Create a new block
    Block block;
    block.timestamp = std::chrono::system_clock::now();
    block.previous_hash = "0000000000000000000000000000000000000000000000000000000000000000";
    block.transactions.push_back(Transaction{
        TransactionType::TRANSFER,
        "Test transaction",
        100,
        "Alice",
        "Bob"});

    size_t block_id = blockchain->create_block(block);
    EXPECT_GT(block_id, 0);

    // Validate block
    EXPECT_TRUE(blockchain->validate_block(block_id));

    // Commit block
    EXPECT_TRUE(blockchain->commit_block(block_id));
}

// Test consensus mechanism
TEST_F(BlockchainTest, Consensus)
{
    // Submit multiple transactions
    for (int i = 0; i < 10; i++)
    {
        Transaction tx;
        tx.type = TransactionType::TRANSFER;
        tx.data = "Transaction " + std::to_string(i);
        tx.amount = 100 + i;
        tx.sender = "Alice";
        tx.receiver = "Bob";
        blockchain->submit_transaction(tx);
    }

    // Wait for consensus
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Check if consensus was reached
    EXPECT_TRUE(blockchain->is_consensus_reached());
}

// Test state management
TEST_F(BlockchainTest, StateManagement)
{
    // Create initial state
    std::string state = "Initial state";
    EXPECT_TRUE(blockchain->verify_state(state));

    // Backup state
    EXPECT_TRUE(blockchain->backup_state());

    // Modify state
    state = "Modified state";
    EXPECT_TRUE(blockchain->verify_state(state));

    // Restore state
    EXPECT_TRUE(blockchain->restore_state());
    EXPECT_TRUE(blockchain->verify_state("Initial state"));
}

// Test performance metrics
TEST_F(BlockchainTest, PerformanceMetrics)
{
    // Get initial metrics
    auto initial_metrics = blockchain->get_metrics();

    // Submit some transactions
    for (int i = 0; i < 100; i++)
    {
        Transaction tx;
        tx.type = TransactionType::TRANSFER;
        tx.data = "Performance test transaction " + std::to_string(i);
        tx.amount = 100 + i;
        tx.sender = "Alice";
        tx.receiver = "Bob";
        blockchain->submit_transaction(tx);
    }

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final metrics
    auto final_metrics = blockchain->get_metrics();

    // Verify metrics have changed
    EXPECT_GT(final_metrics.transactions_processed, initial_metrics.transactions_processed);
    EXPECT_GT(final_metrics.blocks_created, initial_metrics.blocks_created);
}

// Test fault tolerance
TEST_F(BlockchainTest, FaultTolerance)
{
    // Simulate core failure
    blockchain->simulate_core_failure(0);

    // Verify system continues to operate
    Transaction tx;
    tx.type = TransactionType::TRANSFER;
    tx.data = "Fault tolerance test";
    tx.amount = 100;
    tx.sender = "Alice";
    tx.receiver = "Bob";

    size_t tx_id = blockchain->submit_transaction(tx);
    EXPECT_GT(tx_id, 0);

    // Wait for transaction to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TransactionStatus status = blockchain->get_transaction_status(tx_id);
    EXPECT_EQ(status, TransactionStatus::CONFIRMED);

    // Verify core was recovered
    EXPECT_TRUE(blockchain->is_core_healthy(0));
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}