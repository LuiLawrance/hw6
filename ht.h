#ifndef HT_H
#define HT_H

#include <vector>
#include <iostream>
#include <cmath>
#include <stdexcept>

typedef size_t HASH_INDEX_T;

template <typename KeyType>
struct Prober {
    HASH_INDEX_T start_;
    HASH_INDEX_T m_;
    size_t numProbes_;
    static const HASH_INDEX_T npos = (HASH_INDEX_T)-1;
    void init(HASH_INDEX_T start, HASH_INDEX_T m, const KeyType& key) {
        (void)key;
        start_ = start;
        m_ = m;
        numProbes_ = 0;
    }
    virtual HASH_INDEX_T next() = 0;
};

template <typename KeyType>
struct LinearProber : public Prober<KeyType> {
    HASH_INDEX_T next() {
        if (this->numProbes_ >= this->m_) {
            return Prober<KeyType>::npos;
        }
        HASH_INDEX_T loc = (this->start_ + this->numProbes_) % this->m_;
        this->numProbes_++;
        return loc;
    }
};

template <typename KeyType, typename Hash2>
struct DoubleHashProber : public Prober<KeyType> {
    Hash2 h2_;
    HASH_INDEX_T dhstep_;
    static const HASH_INDEX_T DOUBLE_HASH_MOD_VALUES[];
    static const int DOUBLE_HASH_MOD_SIZE;

private:
    HASH_INDEX_T findModulusToUseFromTableSize(HASH_INDEX_T currTableSize) {
        HASH_INDEX_T modulus = DOUBLE_HASH_MOD_VALUES[0];
        for (int i = 0; i < DOUBLE_HASH_MOD_SIZE && DOUBLE_HASH_MOD_VALUES[i] < currTableSize; i++) {
            modulus = DOUBLE_HASH_MOD_VALUES[i];
        }
        return modulus;
    }

public:
    DoubleHashProber(const Hash2& h2 = Hash2()) : h2_(h2) {}

    void init(HASH_INDEX_T start, HASH_INDEX_T m, const KeyType& key) {
        Prober<KeyType>::init(start, m, key);
        HASH_INDEX_T modulus = findModulusToUseFromTableSize(m);
        dhstep_ = modulus - h2_(key) % modulus;
    }

    HASH_INDEX_T next() {
        if (this->numProbes_ >= this->m_) {
            return Prober<KeyType>::npos;
        }
        HASH_INDEX_T loc = (this->start_ + this->numProbes_ * this->dhstep_) % this->m_;
        this->numProbes_++;
        return loc;
    }
};

template <typename KeyType, typename Hash2>
const HASH_INDEX_T DoubleHashProber<KeyType, Hash2>::DOUBLE_HASH_MOD_VALUES[] =
{
    7, 19, 43, 89, 193, 389, 787, 1583, 3191, 6397, 12841, 25703, 51431, 102871,
    205721, 411503, 823051, 1646221, 3292463, 6584957, 13169963, 26339921, 52679927,
    105359939, 210719881, 421439749, 842879563, 1685759113
};

template <typename KeyType, typename Hash2>
const int DoubleHashProber<KeyType, Hash2>::DOUBLE_HASH_MOD_SIZE =
    sizeof(DoubleHashProber<KeyType, Hash2>::DOUBLE_HASH_MOD_VALUES) / sizeof(HASH_INDEX_T);

template<
    typename K,
    typename V,
    typename Prober = LinearProber<K>,
    typename Hash = std::hash<K>,
    typename KEqual = std::equal_to<K>
>
class HashTable {
public:
    typedef K KeyType;
    typedef V ValueType;
    typedef std::pair<KeyType, ValueType> ItemType;
    typedef Hash Hasher;
    struct HashItem {
        ItemType item;
        bool deleted;
        HashItem(const ItemType& newItem) {
            item = newItem;
            deleted = false;
        }
    };

    HashTable(double resizeAlpha = 0.4, const Prober& prober = Prober(), const Hasher& hash = Hasher(), const KEqual& kequal = KEqual())
        : hash_(hash), kequal_(kequal), prober_(prober), totalProbes_(0), mIndex_(0) {}
    ~HashTable() {}
    bool empty() const {
        return size() == 0;
    }
    size_t size() const {
        size_t count = 0;
        for (auto item : table_) {
            if (item != nullptr && !item->deleted) {
                count++;
            }
        }
        return count;
    }
    void insert(const ItemType& p) {}
    void remove(const KeyType& key) {}
    ItemType const* find(const KeyType& key) const {
        HASH_INDEX_T h = this->probe(key);
        if ((prober_.npos == h) || nullptr == table_[h]) {
            return nullptr;
        }
        return &table_[h]->item;
    }
    ItemType* find(const KeyType& key) {
        HASH_INDEX_T h = this->probe(key);
        if ((prober_.npos == h) || nullptr == table_[h]) {
            return nullptr;
        }
        return &table_[h]->item;
    }
    const ValueType& at(const KeyType& key) const {
        HashItem const* item = this->internalFind(key);
        if (item == nullptr) {
            throw std::out_of_range("Bad key");
        }
        return item->item.second;
    }
    ValueType& at(const KeyType& key) {
        HashItem* item = this->internalFind(key);
        if (item == nullptr) {
            throw std::out_of_range("Bad key");
        }
        return item->item.second;
    }
    const ValueType& operator[](const KeyType& key) const {
        return this->at(key);
    }
    ValueType& operator[](const KeyType& key) {
        return this->at(key);
    }
    HashItem* internalFind(const KeyType& key) const {
        HASH_INDEX_T h = this->probe(key);
        if ((prober_.npos == h) || nullptr == table_[h]) {
            return nullptr;
        }
        return table_[h];
    }
    void resize() {}
    void reportAll(std::ostream& out) const {
        for (HASH_INDEX_T i = 0; i < CAPACITIES[mIndex_]; ++i) {
            if (table_[i] != nullptr) {
                out << "Bucket " << i << ": " << table_[i]->item.first << " " << table_[i]->item.second << std::endl;
            }
        }
    }
    void clearTotalProbes() { totalProbes_ = 0; }
    size_t totalProbes() const { return totalProbes_; }

private:
    std::vector<HashItem*> table_;
    Hasher hash_;
    KEqual kequal_;
    mutable Prober prober_;
    mutable size_t totalProbes_;
    static const HASH_INDEX_T CAPACITIES[];
    HASH_INDEX_T mIndex_;

    HASH_INDEX_T probe(const KeyType& key) const {
        HASH_INDEX_T h = hash_(key) % CAPACITIES[mIndex_];
        prober_.init(h, CAPACITIES[mIndex_], key);
        HASH_INDEX_T loc = prober_.next();
        totalProbes_++;
        while (prober_.npos != loc) {
            if (nullptr == table_[loc]) {
                return loc;
            } else if (kequal_(table_[loc]->item.first, key)) {
                return loc;
            }
            loc = prober_.next();
            totalProbes_++;
        }
        return prober_.npos;
    }
};

template<typename K, typename V, typename Prober, typename Hash, typename KEqual>
const HASH_INDEX_T HashTable<K, V, Prober, Hash, KEqual>::CAPACITIES[] = {
    11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853, 25717, 51437, 102877,
    205759, 411527, 823117, 1646237, 3292489, 6584983, 13169977, 26339969, 52679969,
    105359969, 210719881, 421439783, 842879579, 1685759167
};

#endif
