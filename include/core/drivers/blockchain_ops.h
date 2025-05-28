#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// Константы для блокчейна
#define CORE_BLOCKCHAIN_HASH_SIZE 32
#define CORE_BLOCKCHAIN_SIGNATURE_SIZE 64
#define CORE_BLOCKCHAIN_PUBLIC_KEY_SIZE 32
#define CORE_BLOCKCHAIN_PRIVATE_KEY_SIZE 32
#define CORE_BLOCKCHAIN_MERKLE_TREE_DEPTH 32

// Структуры для блокчейна
typedef struct {
    uint8_t hash[CORE_BLOCKCHAIN_HASH_SIZE];
    uint64_t timestamp;
    uint64_t nonce;
    uint8_t previous_hash[CORE_BLOCKCHAIN_HASH_SIZE];
    uint8_t merkle_root[CORE_BLOCKCHAIN_HASH_SIZE];
    uint32_t difficulty;
    uint32_t version;
} core_block_header_t;

typedef struct {
    uint8_t hash[CORE_BLOCKCHAIN_HASH_SIZE];
    uint8_t signature[CORE_BLOCKCHAIN_SIGNATURE_SIZE];
    uint8_t public_key[CORE_BLOCKCHAIN_PUBLIC_KEY_SIZE];
    uint64_t timestamp;
    uint32_t version;
    uint32_t type;
    void* data;
    size_t data_size;
} core_transaction_t;

typedef struct {
    core_block_header_t header;
    core_transaction_t* transactions;
    size_t transaction_count;
    uint8_t merkle_tree[CORE_BLOCKCHAIN_MERKLE_TREE_DEPTH][CORE_BLOCKCHAIN_HASH_SIZE];
} core_block_t;

// Операции с хешированием
void core_blockchain_hash(const void* data, size_t size, uint8_t* hash);
void core_blockchain_hash_twice(const void* data, size_t size, uint8_t* hash);
int core_blockchain_verify_hash(const uint8_t* hash, uint32_t difficulty);

// Операции с подписями
int core_blockchain_sign(const void* data, size_t size, const uint8_t* private_key, uint8_t* signature);
int core_blockchain_verify(const void* data, size_t size, const uint8_t* signature, const uint8_t* public_key);

// Операции с ключами
int core_blockchain_generate_keypair(uint8_t* public_key, uint8_t* private_key);
int core_blockchain_public_key_from_private(const uint8_t* private_key, uint8_t* public_key);

// Операции с блоками
core_block_t* core_block_create();
void core_block_destroy(core_block_t* block);
int core_block_add_transaction(core_block_t* block, const core_transaction_t* transaction);
int core_block_verify(const core_block_t* block);
int core_block_mine(core_block_t* block, uint32_t difficulty);

// Операции с Merkle деревом
void core_blockchain_build_merkle_tree(core_block_t* block);
int core_blockchain_verify_merkle_proof(const uint8_t* leaf_hash, const uint8_t* root_hash,
                                      const uint8_t* proof, size_t proof_size);

// Операции с транзакциями
core_transaction_t* core_transaction_create();
void core_transaction_destroy(core_transaction_t* transaction);
int core_transaction_sign(core_transaction_t* transaction, const uint8_t* private_key);
int core_transaction_verify(const core_transaction_t* transaction);

// Операции с цепочкой блоков
int core_blockchain_verify_chain(const core_block_t* blocks, size_t count);
int core_blockchain_find_fork(const core_block_t* blocks1, size_t count1,
                             const core_block_t* blocks2, size_t count2,
                             size_t* fork_height);

#ifdef __cplusplus
}
#endif 