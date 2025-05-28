#include "core/drivers/blockchain_ops.h"
#include "core/drivers/memory_ops.h"
#include "core/drivers/thread_ops.h"
#include "core/drivers/math_ops.h"

#include <openssl/sha.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/rand.h>

// Операции с хешированием
void core_blockchain_hash(const void* data, size_t size, uint8_t* hash) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, size);
    SHA256_Final(hash, &sha256);
}

void core_blockchain_hash_twice(const void* data, size_t size, uint8_t* hash) {
    uint8_t first_hash[CORE_BLOCKCHAIN_HASH_SIZE];
    core_blockchain_hash(data, size, first_hash);
    core_blockchain_hash(first_hash, CORE_BLOCKCHAIN_HASH_SIZE, hash);
}

int core_blockchain_verify_hash(const uint8_t* hash, uint32_t difficulty) {
    // Проверяем, что хеш удовлетворяет сложности
    // Сложность определяет количество ведущих нулей
    uint32_t zero_bits = 0;
    for (size_t i = 0; i < CORE_BLOCKCHAIN_HASH_SIZE; i++) {
        if (hash[i] == 0) {
            zero_bits += 8;
        } else {
            // Подсчитываем ведущие нули в последнем байте
            uint8_t byte = hash[i];
            while ((byte & 0x80) == 0) {
                zero_bits++;
                byte <<= 1;
            }
            break;
        }
    }
    return zero_bits >= difficulty;
}

// Операции с подписями
int core_blockchain_sign(const void* data, size_t size, const uint8_t* private_key, uint8_t* signature) {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key) {
        return -1;
    }

    BIGNUM* bn = BN_bin2bn(private_key, CORE_BLOCKCHAIN_PRIVATE_KEY_SIZE, NULL);
    if (!bn) {
        EC_KEY_free(key);
        return -1;
    }

    if (!EC_KEY_set_private_key(key, bn)) {
        BN_free(bn);
        EC_KEY_free(key);
        return -1;
    }

    // Создаем публичный ключ из приватного
    const EC_GROUP* group = EC_KEY_get0_group(key);
    EC_POINT* pub = EC_POINT_new(group);
    if (!pub) {
        BN_free(bn);
        EC_KEY_free(key);
        return -1;
    }

    if (!EC_POINT_mul(group, pub, bn, NULL, NULL, NULL)) {
        EC_POINT_free(pub);
        BN_free(bn);
        EC_KEY_free(key);
        return -1;
    }

    if (!EC_KEY_set_public_key(key, pub)) {
        EC_POINT_free(pub);
        BN_free(bn);
        EC_KEY_free(key);
        return -1;
    }

    // Подписываем данные
    uint8_t hash[CORE_BLOCKCHAIN_HASH_SIZE];
    core_blockchain_hash(data, size, hash);

    ECDSA_SIG* sig = ECDSA_do_sign(hash, CORE_BLOCKCHAIN_HASH_SIZE, key);
    if (!sig) {
        EC_POINT_free(pub);
        BN_free(bn);
        EC_KEY_free(key);
        return -1;
    }

    // Сохраняем подпись
    const BIGNUM* r = ECDSA_SIG_get0_r(sig);
    const BIGNUM* s = ECDSA_SIG_get0_s(sig);
    BN_bn2bin(r, signature);
    BN_bn2bin(s, signature + CORE_BLOCKCHAIN_HASH_SIZE);

    // Освобождаем ресурсы
    ECDSA_SIG_free(sig);
    EC_POINT_free(pub);
    BN_free(bn);
    EC_KEY_free(key);

    return 0;
}

int core_blockchain_verify(const void* data, size_t size, const uint8_t* signature, const uint8_t* public_key) {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key) {
        return -1;
    }

    const EC_GROUP* group = EC_KEY_get0_group(key);
    EC_POINT* pub = EC_POINT_new(group);
    if (!pub) {
        EC_KEY_free(key);
        return -1;
    }

    // Преобразуем публичный ключ
    if (!EC_POINT_oct2point(group, pub, public_key, CORE_BLOCKCHAIN_PUBLIC_KEY_SIZE, NULL)) {
        EC_POINT_free(pub);
        EC_KEY_free(key);
        return -1;
    }

    if (!EC_KEY_set_public_key(key, pub)) {
        EC_POINT_free(pub);
        EC_KEY_free(key);
        return -1;
    }

    // Создаем подпись
    ECDSA_SIG* sig = ECDSA_SIG_new();
    if (!sig) {
        EC_POINT_free(pub);
        EC_KEY_free(key);
        return -1;
    }

    BIGNUM* r = BN_bin2bn(signature, CORE_BLOCKCHAIN_HASH_SIZE, NULL);
    BIGNUM* s = BN_bin2bn(signature + CORE_BLOCKCHAIN_HASH_SIZE, CORE_BLOCKCHAIN_HASH_SIZE, NULL);
    if (!r || !s) {
        if (r) BN_free(r);
        if (s) BN_free(s);
        ECDSA_SIG_free(sig);
        EC_POINT_free(pub);
        EC_KEY_free(key);
        return -1;
    }

    ECDSA_SIG_set0(sig, r, s);

    // Проверяем подпись
    uint8_t hash[CORE_BLOCKCHAIN_HASH_SIZE];
    core_blockchain_hash(data, size, hash);

    int result = ECDSA_do_verify(hash, CORE_BLOCKCHAIN_HASH_SIZE, sig, key);

    // Освобождаем ресурсы
    ECDSA_SIG_free(sig);
    EC_POINT_free(pub);
    EC_KEY_free(key);

    return result;
}

// Операции с ключами
int core_blockchain_generate_keypair(uint8_t* public_key, uint8_t* private_key) {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key) {
        return -1;
    }

    if (!EC_KEY_generate_key(key)) {
        EC_KEY_free(key);
        return -1;
    }

    // Получаем приватный ключ
    const BIGNUM* priv = EC_KEY_get0_private_key(key);
    if (!priv) {
        EC_KEY_free(key);
        return -1;
    }

    BN_bn2bin(priv, private_key);

    // Получаем публичный ключ
    const EC_POINT* pub = EC_KEY_get0_public_key(key);
    if (!pub) {
        EC_KEY_free(key);
        return -1;
    }

    size_t size = EC_POINT_point2oct(EC_KEY_get0_group(key), pub,
                                   POINT_CONVERSION_COMPRESSED,
                                   public_key, CORE_BLOCKCHAIN_PUBLIC_KEY_SIZE,
                                   NULL);
    if (size != CORE_BLOCKCHAIN_PUBLIC_KEY_SIZE) {
        EC_KEY_free(key);
        return -1;
    }

    EC_KEY_free(key);
    return 0;
}

int core_blockchain_public_key_from_private(const uint8_t* private_key, uint8_t* public_key) {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key) {
        return -1;
    }

    BIGNUM* bn = BN_bin2bn(private_key, CORE_BLOCKCHAIN_PRIVATE_KEY_SIZE, NULL);
    if (!bn) {
        EC_KEY_free(key);
        return -1;
    }

    if (!EC_KEY_set_private_key(key, bn)) {
        BN_free(bn);
        EC_KEY_free(key);
        return -1;
    }

    // Создаем публичный ключ из приватного
    const EC_GROUP* group = EC_KEY_get0_group(key);
    EC_POINT* pub = EC_POINT_new(group);
    if (!pub) {
        BN_free(bn);
        EC_KEY_free(key);
        return -1;
    }

    if (!EC_POINT_mul(group, pub, bn, NULL, NULL, NULL)) {
        EC_POINT_free(pub);
        BN_free(bn);
        EC_KEY_free(key);
        return -1;
    }

    size_t size = EC_POINT_point2oct(group, pub,
                                   POINT_CONVERSION_COMPRESSED,
                                   public_key, CORE_BLOCKCHAIN_PUBLIC_KEY_SIZE,
                                   NULL);
    if (size != CORE_BLOCKCHAIN_PUBLIC_KEY_SIZE) {
        EC_POINT_free(pub);
        BN_free(bn);
        EC_KEY_free(key);
        return -1;
    }

    EC_POINT_free(pub);
    BN_free(bn);
    EC_KEY_free(key);
    return 0;
}

// Операции с блоками
core_block_t* core_block_create() {
    core_block_t* block = (core_block_t*)core_malloc(sizeof(core_block_t));
    if (!block) {
        return NULL;
    }

    block->transactions = NULL;
    block->transaction_count = 0;
    return block;
}

void core_block_destroy(core_block_t* block) {
    if (!block) {
        return;
    }

    if (block->transactions) {
        for (size_t i = 0; i < block->transaction_count; i++) {
            core_transaction_destroy(&block->transactions[i]);
        }
        core_free(block->transactions);
    }

    core_free(block);
}

int core_block_add_transaction(core_block_t* block, const core_transaction_t* transaction) {
    if (!block || !transaction) {
        return -1;
    }

    core_transaction_t* new_transactions = (core_transaction_t*)core_malloc(
        sizeof(core_transaction_t) * (block->transaction_count + 1));
    if (!new_transactions) {
        return -1;
    }

    // Копируем существующие транзакции
    if (block->transactions) {
        core_memcpy_aligned(new_transactions, block->transactions,
                          sizeof(core_transaction_t) * block->transaction_count);
        core_free(block->transactions);
    }

    // Копируем новую транзакцию
    core_memcpy_aligned(&new_transactions[block->transaction_count], transaction,
                      sizeof(core_transaction_t));

    block->transactions = new_transactions;
    block->transaction_count++;

    return 0;
}

int core_block_verify(const core_block_t* block) {
    if (!block) {
        return -1;
    }

    // Проверяем хеш блока
    if (!core_blockchain_verify_hash(block->header.hash, block->header.difficulty)) {
        return -1;
    }

    // Проверяем Merkle дерево
    uint8_t calculated_root[CORE_BLOCKCHAIN_HASH_SIZE];
    core_blockchain_build_merkle_tree((core_block_t*)block);
    if (core_memcmp(calculated_root, block->header.merkle_root,
                   CORE_BLOCKCHAIN_HASH_SIZE) != 0) {
        return -1;
    }

    // Проверяем все транзакции
    for (size_t i = 0; i < block->transaction_count; i++) {
        if (!core_transaction_verify(&block->transactions[i])) {
            return -1;
        }
    }

    return 0;
}

int core_block_mine(core_block_t* block, uint32_t difficulty) {
    if (!block) {
        return -1;
    }

    block->header.difficulty = difficulty;
    block->header.timestamp = time(NULL);

    // Пробуем разные nonce значения
    for (block->header.nonce = 0; block->header.nonce < UINT64_MAX; block->header.nonce++) {
        // Обновляем хеш блока
        core_blockchain_hash_twice(&block->header, sizeof(core_block_header_t),
                                 block->header.hash);

        // Проверяем, удовлетворяет ли хеш сложности
        if (core_blockchain_verify_hash(block->header.hash, difficulty)) {
            return 0;
        }
    }

    return -1;
}

// Операции с Merkle деревом
void core_blockchain_build_merkle_tree(core_block_t* block) {
    if (!block || !block->transactions || block->transaction_count == 0) {
        return;
    }

    // Вычисляем хеши листьев
    for (size_t i = 0; i < block->transaction_count; i++) {
        core_blockchain_hash_twice(&block->transactions[i],
                                 sizeof(core_transaction_t),
                                 block->merkle_tree[0][i]);
    }

    // Вычисляем хеши промежуточных узлов
    size_t level_size = block->transaction_count;
    size_t level = 0;

    while (level_size > 1) {
        size_t next_level_size = (level_size + 1) / 2;
        for (size_t i = 0; i < next_level_size; i++) {
            size_t left = i * 2;
            size_t right = left + 1;

            if (right < level_size) {
                // Объединяем два хеша
                uint8_t combined[2 * CORE_BLOCKCHAIN_HASH_SIZE];
                core_memcpy_aligned(combined,
                                  block->merkle_tree[level][left],
                                  CORE_BLOCKCHAIN_HASH_SIZE);
                core_memcpy_aligned(combined + CORE_BLOCKCHAIN_HASH_SIZE,
                                  block->merkle_tree[level][right],
                                  CORE_BLOCKCHAIN_HASH_SIZE);
                core_blockchain_hash_twice(combined,
                                        2 * CORE_BLOCKCHAIN_HASH_SIZE,
                                        block->merkle_tree[level + 1][i]);
            } else {
                // Копируем одиночный хеш
                core_memcpy_aligned(block->merkle_tree[level + 1][i],
                                  block->merkle_tree[level][left],
                                  CORE_BLOCKCHAIN_HASH_SIZE);
            }
        }

        level++;
        level_size = next_level_size;
    }

    // Сохраняем корневой хеш
    core_memcpy_aligned(block->header.merkle_root,
                      block->merkle_tree[level][0],
                      CORE_BLOCKCHAIN_HASH_SIZE);
}

int core_blockchain_verify_merkle_proof(const uint8_t* leaf_hash,
                                      const uint8_t* root_hash,
                                      const uint8_t* proof,
                                      size_t proof_size) {
    if (!leaf_hash || !root_hash || !proof || proof_size == 0) {
        return -1;
    }

    uint8_t current_hash[CORE_BLOCKCHAIN_HASH_SIZE];
    core_memcpy_aligned(current_hash, leaf_hash, CORE_BLOCKCHAIN_HASH_SIZE);

    // Проверяем каждый шаг доказательства
    for (size_t i = 0; i < proof_size; i++) {
        uint8_t combined[2 * CORE_BLOCKCHAIN_HASH_SIZE];
        if (i % 2 == 0) {
            // Текущий хеш слева
            core_memcpy_aligned(combined, current_hash, CORE_BLOCKCHAIN_HASH_SIZE);
            core_memcpy_aligned(combined + CORE_BLOCKCHAIN_HASH_SIZE,
                              proof + i * CORE_BLOCKCHAIN_HASH_SIZE,
                              CORE_BLOCKCHAIN_HASH_SIZE);
        } else {
            // Текущий хеш справа
            core_memcpy_aligned(combined,
                              proof + i * CORE_BLOCKCHAIN_HASH_SIZE,
                              CORE_BLOCKCHAIN_HASH_SIZE);
            core_memcpy_aligned(combined + CORE_BLOCKCHAIN_HASH_SIZE,
                              current_hash,
                              CORE_BLOCKCHAIN_HASH_SIZE);
        }

        core_blockchain_hash_twice(combined,
                                 2 * CORE_BLOCKCHAIN_HASH_SIZE,
                                 current_hash);
    }

    // Сравниваем с корневым хешем
    return core_memcmp(current_hash, root_hash, CORE_BLOCKCHAIN_HASH_SIZE) == 0;
}

// Операции с транзакциями
core_transaction_t* core_transaction_create() {
    core_transaction_t* transaction = (core_transaction_t*)core_malloc(sizeof(core_transaction_t));
    if (!transaction) {
        return NULL;
    }

    transaction->data = NULL;
    transaction->data_size = 0;
    return transaction;
}

void core_transaction_destroy(core_transaction_t* transaction) {
    if (!transaction) {
        return;
    }

    if (transaction->data) {
        core_free(transaction->data);
    }
}

int core_transaction_sign(core_transaction_t* transaction, const uint8_t* private_key) {
    if (!transaction || !private_key) {
        return -1;
    }

    // Вычисляем хеш транзакции
    core_blockchain_hash_twice(transaction, sizeof(core_transaction_t) - CORE_BLOCKCHAIN_SIGNATURE_SIZE,
                             transaction->hash);

    // Подписываем хеш
    return core_blockchain_sign(transaction->hash, CORE_BLOCKCHAIN_HASH_SIZE,
                              private_key, transaction->signature);
}

int core_transaction_verify(const core_transaction_t* transaction) {
    if (!transaction) {
        return -1;
    }

    // Вычисляем хеш транзакции
    uint8_t calculated_hash[CORE_BLOCKCHAIN_HASH_SIZE];
    core_blockchain_hash_twice(transaction, sizeof(core_transaction_t) - CORE_BLOCKCHAIN_SIGNATURE_SIZE,
                             calculated_hash);

    // Проверяем хеш
    if (core_memcmp(calculated_hash, transaction->hash, CORE_BLOCKCHAIN_HASH_SIZE) != 0) {
        return -1;
    }

    // Проверяем подпись
    return core_blockchain_verify(transaction->hash, CORE_BLOCKCHAIN_HASH_SIZE,
                                transaction->signature, transaction->public_key);
}

// Операции с цепочкой блоков
int core_blockchain_verify_chain(const core_block_t* blocks, size_t count) {
    if (!blocks || count == 0) {
        return -1;
    }

    // Проверяем первый блок
    if (!core_block_verify(&blocks[0])) {
        return -1;
    }

    // Проверяем остальные блоки
    for (size_t i = 1; i < count; i++) {
        // Проверяем связь с предыдущим блоком
        if (core_memcmp(blocks[i].header.previous_hash,
                       blocks[i-1].header.hash,
                       CORE_BLOCKCHAIN_HASH_SIZE) != 0) {
            return -1;
        }

        // Проверяем блок
        if (!core_block_verify(&blocks[i])) {
            return -1;
        }
    }

    return 0;
}

int core_blockchain_find_fork(const core_block_t* blocks1, size_t count1,
                             const core_block_t* blocks2, size_t count2,
                             size_t* fork_height) {
    if (!blocks1 || !blocks2 || !fork_height) {
        return -1;
    }

    // Ищем общий предок
    size_t min_count = count1 < count2 ? count1 : count2;
    for (size_t i = 0; i < min_count; i++) {
        if (core_memcmp(blocks1[i].header.hash,
                       blocks2[i].header.hash,
                       CORE_BLOCKCHAIN_HASH_SIZE) == 0) {
            *fork_height = i;
            return 0;
        }
    }

    return -1;
} 