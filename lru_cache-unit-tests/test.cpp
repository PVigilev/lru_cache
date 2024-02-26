#include "lru_cache.hpp"
#include <gtest/gtest.h>

namespace gen
{
    enum class CacheOperationControl
    {
        GET_EXIST=0x0,
        GET_NOT_EXIST,
        PUT_NEW,
        PUT_EXIST,
        RESERVE
    };
    using CommandType = std::tuple<CacheOperationControl, int, int >;
    using LRUcache = lru_cache<int, int>;

    class ParamTestFixture : public testing::Test,
        public testing::WithParamInterface<std::vector<CommandType>>
    {
    public:
        // small helper methid to validate cache's LRU element
        static inline void validateLRU(size_t index, std::tuple< CacheOperationControl, int, int> operation, const LRUcache& cache)
        {
            EXPECT_TRUE(cache.front().has_value()) << index;
            if (cache.front().has_value())
            {
                EXPECT_EQ(*cache.front(), std::make_pair(std::get<1>(operation), std::get<2>(operation)))
                    << index;
            }
        }
        static inline void validateLRU(size_t index, std::pair<int, int> keyValue, const LRUcache& cache)
        {
            EXPECT_TRUE(cache.front().has_value()) 
                << index;
            if (cache.front().has_value())
            {
                EXPECT_EQ(*cache.front(), keyValue)
                    << index;
            }
        }
    };


    // Parametrized test suite for happy-path put/get sequences
    TEST_P(ParamTestFixture, PutGetTestSuite)
    {
        std::vector <std::tuple<CacheOperationControl, int, int>> data = GetParam();
        LRUcache cache(0);

        size_t expected_size = 0;
        EXPECT_EQ(cache.size(), expected_size);
        EXPECT_FALSE(cache.front().has_value());
        for (size_t i = 0; i < data.size(); i++)
        {
            std::tuple<CacheOperationControl, int, int> tuple = data[i];
            if (std::get<0>(tuple) == CacheOperationControl::PUT_NEW)
            {
                auto top_old = cache.front();
                EXPECT_FALSE(cache.cached(std::get<1>(tuple))) << i << " " << "PUT_NEW";
                if (expected_size != cache.capacity())
                {
                    expected_size++;
                }
                cache.put(std::get<1>(tuple), std::get<2>(tuple));
                auto top_new = cache.front();
                if (expected_size > 0) 
                {
                    EXPECT_NE(top_old, top_new) << i << " " << "PUT_NEW";
                    validateLRU(i, tuple, cache);
                }
                else
                {
                    EXPECT_FALSE(top_new.has_value());
                }
            }
            else if (std::get<0>(tuple) == CacheOperationControl::PUT_EXIST)
            {
                EXPECT_TRUE(cache.cached(std::get<1>(tuple))) << i << " " << "PUT_EXIST";
                cache.put(std::get<1>(tuple), std::get<2>(tuple));
                validateLRU(i, tuple, cache);
            }
            else if (std::get<0>(tuple) == CacheOperationControl::GET_EXIST)
            {
                auto value = cache.get(std::get<1>(tuple));
                EXPECT_TRUE(cache.cached(std::get<1>(tuple))) << i << " " << "GET_EXIST";
                EXPECT_TRUE(value.has_value()) << i << " " << "GET_EXIST";
                if (value.has_value())
                {
                    EXPECT_EQ(*value, std::get<2>(tuple)) << i << " " << "GET_EXIST";
                }

                validateLRU(i, tuple, cache);
            }
            else if (std::get<0>(tuple) == CacheOperationControl::GET_NOT_EXIST)
            {
                EXPECT_FALSE(cache.cached(std::get<1>(tuple))) << i << " " << "GET_NOT_EXIST";
                auto old_top = cache.front();
                auto value = cache.get(std::get<1>(tuple));
                auto top = cache.front();
                EXPECT_FALSE(value.has_value()) << i << " " << "GET_NOT_EXIST";
                EXPECT_EQ(old_top, top) << i << " " << "GET_NOT_EXIST";
            }
            else if (std::get<0>(tuple) == CacheOperationControl::RESERVE)
            {
                auto top = cache.front();
                if (std::get<1>(tuple) < cache.size())
                {
                    expected_size = std::get<1>(tuple);
                }
                cache.reserve(std::get<1>(tuple));
                EXPECT_EQ(cache.capacity(), std::get<1>(tuple)) << i << " " << "RESERVE";
            }
            EXPECT_EQ(cache.size(), expected_size) << i;
        }
    };

    INSTANTIATE_TEST_CASE_P(PutGetTest, ParamTestFixture, ::testing::Values(
        std::vector< std::tuple<CacheOperationControl, int, int>>{
            { CacheOperationControl::RESERVE, 5, 5 },
            { CacheOperationControl::PUT_NEW, 1, 2 },
            { CacheOperationControl::PUT_NEW, 2, 3 },
            { CacheOperationControl::PUT_NEW, 4, 2 },
            { CacheOperationControl::PUT_NEW, 5, 2 },
            { CacheOperationControl::PUT_NEW, 7, 2 },
            { CacheOperationControl::PUT_EXIST, 4, 3 },
            { CacheOperationControl::PUT_EXIST, 1, 3 },
            { CacheOperationControl::GET_EXIST, 1, 3 },
            { CacheOperationControl::GET_EXIST, 2, 3 },
            { CacheOperationControl::GET_EXIST, 4, 3 },
            { CacheOperationControl::GET_EXIST, 5, 2 },
            { CacheOperationControl::GET_EXIST, 7, 2 },
            { CacheOperationControl::RESERVE, 2, 2 },
            { CacheOperationControl::GET_EXIST, 5, 2 },
            { CacheOperationControl::GET_EXIST, 7, 2 }
    },
        std::vector< std::tuple<CacheOperationControl, int, int>>{
            { CacheOperationControl::RESERVE, 1, 1 },
            { CacheOperationControl::PUT_NEW, 1, 2 },
            { CacheOperationControl::PUT_NEW, 2, 3 },
            { CacheOperationControl::GET_NOT_EXIST, 1, 2 },
            { CacheOperationControl::PUT_EXIST, 2, 4 },
            { CacheOperationControl::PUT_NEW, 1, 2 },
            { CacheOperationControl::GET_EXIST, 1, 2 }
    },
        std::vector< std::tuple<CacheOperationControl, int, int>>{
            { CacheOperationControl::RESERVE, 0, 0 },
            { CacheOperationControl::PUT_NEW, 1, 2 },
            { CacheOperationControl::PUT_NEW, 2, 3 },
            { CacheOperationControl::GET_NOT_EXIST, 1, 2 },
            { CacheOperationControl::GET_NOT_EXIST, 2, 3 }
    }
    ));


    TEST(ConstructorTests, ConstructorTest)
    {
        LRUcache cache(4);
        cache.put(1, 2);
        cache.put(3, 4);

        LRUcache cache_copied(cache);

        EXPECT_TRUE(cache_copied.cached(1));
        EXPECT_TRUE(cache_copied.cached(3));
        EXPECT_EQ(cache_copied.front(), std::make_pair(3, 4));
        EXPECT_EQ(cache_copied.get(3), 4);
        EXPECT_EQ(cache_copied.get(1), 2);

        LRUcache cache_moved = std::move(cache);
        
        EXPECT_TRUE(cache_moved.cached(1));
        EXPECT_TRUE(cache_moved.cached(3));
        EXPECT_EQ(cache_moved.front(), std::make_pair(3, 4));
        EXPECT_EQ(cache_moved.get(3), 4);
        EXPECT_EQ(cache_moved.get(1), 2);
        EXPECT_EQ(cache.size(), 0);
    }
}