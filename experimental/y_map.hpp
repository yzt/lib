#pragma once 

#if defined(_MSC_VER)
    // You must use all three macros to be portable...
    #define Y_PACK_STRUCT_BEGIN     __pragma(pack (push, 1))
    #define Y_PACK_STRUCT_END       __pragma(pack (pop))
    #define Y_PACK_STRUCT_ATTRIB    /**/
#else
    // You must use all three macros to be portable...
    #define Y_PACK_STRUCT_BEGIN     /**/
    #define Y_PACK_STRUCT_END       /**/
    #define Y_PACK_STRUCT_ATTRIB    __attribute__((__packed__))
#endif

namespace y {

template <typename K, typename V, typename H>
class Map {
public:
    Y_PACK_STRUCT_BEGIN;
    struct Y_PACK_STRUCT_ATTRIB Metadata {
        unsigned char distance : 7;
        unsigned char in_use : 1;
        H hash;
    };
    Y_PACK_STRUCT_END;

public:
    static constexpr auto SizeOfKeyBuffer (int capacity) {return sizeof(K) * capacity;}
    static constexpr auto SizeOfValueBuffer (int capacity) {return sizeof(V) * capacity;}
    static constexpr auto SizeOfMetadataBuffer (int capacity) {return sizeof(Metadata<H>) * capacity;}

public:
    Map (int capacity, void * key_buffer, void * value_buffer, void * metadata_buffer)
        : Map (capacity, (K *)key_buffer, (V *)value_buffer, (Metadata *)metadata_buffer)
    {}

    Map (unsigned capacity, K * key_buffer, V * value_buffer, Metadata * metadata_buffer)
        //: Map (capacity, (void *)key_buffer, (void *)value_buffer, (void *)metadata_buffer) {}
        : m_capacity (capacity), m_count (0), m_meta (metadata_buffer), m_keys (key_buffer), m_values (value_buffer)
    {
        if (m_capacity > 0 && m_meta && m_keys && m_values) {
            Metadata const empty = {};
            Metadata * p = m_meta;
            for (int i = 0; i < m_capacity; ++i, ++p)
                *p = {};
        } else {
            throw 42;
        }
    }

    ~Map () {clear();}

    int capacity () const {return m_capacity;}
    int count () const {return m_count;}
    bool empty () const {return 0 == m_count;}
    bool full () const {return m_count >= m_capacity;}
    float load_factor () const {return float(m_count) / m_capacity;}

    bool index_in_use (int index) const {return index >= 0 && index < m_capacity && 0 != m_meta[index].in_use;}
    int index_of_hash (H hash) const {return ((unsigned long long)m_capacity * hash) >> 32;}

    K const & key_at (int index) const {return m_keys[index];}
    V const & value_at (int index) const {return m_values[index];}
    V & value_at (int index) {return m_values[index];}
    Metadata const & metadata_at (int index) const {return m_meta[index];}

    int find_hash (H hash) const {
        auto start_index = index_of_hash(hash);
        int dist = 0;
        ...
    }

    int find (K const & key) const {
    }

    bool has (H hash) const {
    }

    bool has (K const & key, H hash_value) const {
    }

    bool insert (K key, H hash, V value) {  // fails if key already exists
    }

    bool update (H hash, K const & key, V value) {  // fails if key doesn't exist
    }

    bool upsert (H hash, K key, V value) {  // inserts or updates; fails only if there is no more room
    }

    bool erase (H hash, K const & key) {
    }

    void clear () {
        auto * p = m_meta;
        auto cnt = m_count;
        Metadata const empty = {};
        for (int i = 0, n = m_capacity; cnt > 0 && i < n; ++i, ++p)
            if (p->in_use) {
                cnt -= 1;
                *p = empty;
                (m_keys + i)->K::~K();
                (m_values + i)->V::~V();
            }
        m_count = cnt;
    }

private:
    int m_capacity = 0;
    int m_count = 0;
    Metadata * m_meta = nullptr;
    K * m_keys = nullptr;
    V * m_values = nullptr;

private:
    static_assert(sizeof(H) == 1 || sizeof(H) == 2 || sizeof(H) == 4 || sizeof(H) == 8, "Type of hash value must have a size of 1, 2, 4, or 8.");
    static_assert(H(0) < H(-1), "Type of hash value must be unsigned.");
    static_assert(unsigned(0) < unsigned(-1), "Type of hash value must be unsigned.");
    static_assert(sizeof(Metadata) == sizeof(H) + 1, "Structure packing didn't take effect!");
    // TODO: also check for H being integral, without std type traits or too much scaffolding.
};

//template <typename K, typename V, typename H> map (unsigned, void *, void *, void *) -> Map<K, V, H>;

#if  0
Y_PACK_STRUCT_BEGIN;
template <typename H>
struct Y_PACK_STRUCT_ATTRIB MapMetadata {
    unsigned char distance : 7;
    unsigned char in_use : 1;
    H hash;

    static_assert(sizeof(H) == 1 || sizeof(H) == 2 || sizeof(H) == 4 || sizeof(H) == 8, "Type of hash value must have a size of 1, 2, 4, or 8.");
    static_assert(H(0) < H(-1), "Type of hash value must be unsigned.");
    static_assert(unsigned(0) < unsigned(-1), "Type of hash value must be unsigned.");
};
Y_PACK_STRUCT_END;
static_assert(sizeof(MapMetadata<unsigned>) == sizeof(unsigned) + 1, "Structure packing didn't take effect!");

template <typename K, typename V, typename H>
struct Map {
    unsigned capacity;
    unsigned count;
    MapMetadata<H> * meta;
    K * keys;
    V * values;
};

template <typename K, typename V, typename H>
Map<K, V, H> Map_Init (unsigned capacity, K * key_buffer, V * value_buffer, MapMetadata<H> * metadata_buffer) {
    Map<K, V, H> ret = {};
    if (capacity > 0 && key_buffer && value_buffer && metadata_buffer) {
        ret.capacity = capacity;
        ret.count = 0;
        ret.meta = metadata_buffer;
        ret.keys = key_buffer;
        ret.values = value_buffer;

        ::memset(ret.meta, 0, sizeof(MapMetadata<H>) * ret.capacity);
    }
    return ret;
}

template <typename K, typename V, typename H>
void Map_Clear (Map<K, V, H> & map) {
    auto * p = map.meta;
    auto cnt = map.count;
    for (unsigned i = 0, n = map.capacity; cnt > 0 && i < n; ++i, ++p) {
        if (p->in_use) {
            cnt -= 1;
            p->distance = 0;
            p->in_use = 0;
            p->hash = 0;
            (map.keys + i)->K::~K();
            (map.values + i)->V::~V();
        }
    }
    map.count = cnt;
}

//template <typename K, typename V, typename H>
//bool Map_Valid (Map<K, V, H> const & map) {return map.capacity > 0;}
//
//template <typename K, typename V, typename H>
//bool Map_Empty (Map<K, V, H> const & map) {return map.count == 0;}
//
//template <typename K, typename V, typename H>
//bool Map_Full (Map<K, V, H> const & map) {return map.count >= map.capacity;}
//
//template <typename K, typename V, typename H>
//unsigned Map_Capacity (Map<K, V, H> const & map) {return map.capacity;}
//
//template <typename K, typename V, typename H>
//unsigned Map_Count (Map<K, V, H> const & map) {return map.count;}
//
//template <typename K, typename V, typename H>
//float Map_LoadFactor (Map<K, V, H> const & map) {return float(map.count) / map.capacity;}

template <typename K, typename V, typename H>
bool Map_IndexInUse (Map<K, V, H> const & map, unsigned index) {
    return (index < map.capacity) && (0 != (map.hashes[index] & 1));
}

template <typename K, typename V, typename H>
unsigned Map_IndexOfHash (Map<K, V, H> const & map, H const & hash_value) {
    return unsigned(((unsigned long long)map.capacity * hash_value) >> 32);
}

//template <typename K, typename V, typename H>
//template <typename K, typename V, typename H>
//template <typename K, typename V, typename H>
#endif

}   // namespace y
