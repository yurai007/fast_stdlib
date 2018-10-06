#include "hashmap.hpp"

namespace basics
{

common::Hashmap<500> hashmap;
std::map<int, common::int_holder> stl_map;

static inline char basic_get_operation()
{
    return (rand()%2 == 1)? 'I' : 'M';
}

static void basic_test_case()
{
    printf("\n%s\n\n", __FUNCTION__);
    srand(time(nullptr));

    constexpr unsigned operations_number {100};
    constexpr unsigned uniwersum_size {100};
    unsigned inserts_counter {0};
    unsigned members_counter {0};
    constexpr bool debug {false};

    unsigned members_hits {0};
    unsigned stl_members_hits {0};

    hashmap.reset();
    stl_map.clear();

    common::int_holder basic_config;
    basic_config.mark = false;

    printf("operations_number = %d\n", operations_number);

    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = basic_get_operation();

        if (operation == 'I')
        {
            basic_config.content = (rand()%uniwersum_size);
            hashmap.insert(basic_config);
            stl_map[basic_config.content] = basic_config;

            inserts_counter++;

            if (debug)
                printf("insert = %d\n", basic_config.content);
        }
        else
            if (operation == 'M')
            {
                basic_config.content = (rand()%uniwersum_size);
                bool hit = hashmap.member(basic_config);
                if (hit)
                    members_hits++;
                auto stl_iter = stl_map.find(basic_config.content);
                if (stl_iter != stl_map.end())
                    stl_members_hits++;

                members_counter++;

                if (debug)
                {
                    printf("member = %d, hit = %d\n", basic_config.content, hit);
                    printf("member = %d, hit = %d STL\n", basic_config.content,
                           (stl_iter != stl_map.end()));
                }
            }
    }
    printf("inserts = %d, members = %d, hits = %d, stl hits = %d\n",
           inserts_counter, members_counter, members_hits, stl_members_hits);
}

}


namespace real_tests
{

common::Hashmap<100003> hashmap;
std::map<int, common::int_holder> stl_map;

static inline char get_operation()
{
    return (rand()%2 == 1)? 'I' : 'M';
}

static void real_test_case()
{
    constexpr bool debug {false};
    constexpr unsigned operations_number {190000};
    constexpr unsigned uniwersum_size {1000000000};

    unsigned inserts_counter {0};
    unsigned members_counter {0};

    unsigned members_hits {0};
    unsigned stl_members_hits {0};

    hashmap.reset();
    stl_map.clear();

    assert(hashmap.size() == 0);
    assert(stl_map.size() == 0);

    common::int_holder basic_config;
    basic_config.mark = false;

    std::vector<std::pair<int, int>> hashmap_log, stl_map_log;

    printf("\n%s\n\n", __FUNCTION__);
    printf("hashmap capacity = %u, operations_number = %u, uniwersum_size = %u\n",
           hashmap.capacity(), operations_number, uniwersum_size);

    srand(time(nullptr));

    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = get_operation();

        if (operation == 'I')
        {
            basic_config.content = (rand()%uniwersum_size);
            hashmap.insert(basic_config);
            assert(hashmap.member(basic_config));
            stl_map[basic_config.content] = basic_config;
            assert(stl_map.find(basic_config.content) != stl_map.end());

            assert(hashmap.size() == stl_map.size());
            inserts_counter++;

            if (debug)
                printf("insert = %d\n", basic_config.content);
        }
        else
            if (operation == 'M')
            {
                basic_config.content = (rand()%uniwersum_size);
                bool hit = hashmap.member(basic_config);
                if (hit)
                    members_hits++;
                auto stl_iter = stl_map.find(basic_config.content);
                if (stl_iter != stl_map.end())
                    stl_members_hits++;

                members_counter++;
                hashmap_log.push_back({(int)basic_config.content, hit});
                stl_map_log.push_back({(int)basic_config.content, (stl_iter != stl_map.end())});
            }
    }

    if (debug)
    {
        printf("hashmap log\n");
        for (unsigned i = 0; i < hashmap_log.size(); i++)
            printf("member = %d, hit = %d\n", hashmap_log[i].first, hashmap_log[i].second);

        printf("STL log\n");
        for (unsigned i = 0; i < stl_map_log.size(); i++)
            printf("member = %d, hit = %d\n", stl_map_log[i].first, stl_map_log[i].second);
    }

    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, stl hits = %d, hashmap.size = %d, stl map size = %ld\n",
           inserts_counter, members_counter, members_hits, stl_members_hits,
           hashmap.size(), stl_map.size());
    printf("hashmap.collisions = %d, colisions per insert = %d\n", hashmap.collisions,
           (hashmap.collisions/inserts_counter));

    assert(members_hits == stl_members_hits);
    assert(hashmap.size() == stl_map.size());

    assert(hashmap_log.size() == stl_map_log.size());

    for (unsigned i = 0; i < hashmap_log.size(); i++)
    {
        assert(hashmap_log[i].first == stl_map_log[i].first);
        assert(hashmap_log[i].second == stl_map_log[i].second);
    }
    printf("OK :)\n");
}

}


namespace hashmap_tests
{

common::Hashmap<100003> hashmap;

static inline char get_operation()
{
    return (rand()%2 == 1)? 'I' : 'M';
}

static void real_test_case_only_hashmap()
{
    constexpr unsigned operations_number {190000};
    constexpr unsigned uniwersum_size {1000000000};
    constexpr bool dump {false};

    unsigned inserts_counter {0};
    unsigned members_counter {0};
    unsigned members_hits {0};

    hashmap.reset();

    assert(hashmap.size() == 0);

    common::int_holder basic_config;
    basic_config.mark = false;

    printf("\n%s\n\n", __FUNCTION__);
    printf("hashmap capacity = %u, operations_number = %u, uniwersum_size = %u\n",
           hashmap.capacity(), operations_number, uniwersum_size);

    srand(time(nullptr));

    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = get_operation();

        if (operation == 'I')
        {
            basic_config.content = (rand()%uniwersum_size);
            hashmap.insert(basic_config);
            assert(hashmap.member(basic_config));
            inserts_counter++;
        }
        else
            if (operation == 'M')
            {
                basic_config.content = (rand()%uniwersum_size);
                bool hit = hashmap.member(basic_config);
                if (hit)
                    members_hits++;
                members_counter++;
            }
    }

    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, hashmap.size = %d\n",
           inserts_counter, members_counter, members_hits, hashmap.size());
    printf("hashmap.collisions = %d, colisions per insert = %d\n", hashmap.collisions,
           (hashmap.collisions/inserts_counter));

    unsigned i = 0, number = 0, max_len = 0;
    uint64_t sum = 0;
    for (; i < hashmap.table.size();)
    {
        unsigned base = i;
        while ((i < hashmap.table.size()) && !hashmap.table[i++].is_empty());

        unsigned len = i-base-1;
        if (!hashmap.table[base].is_empty())
        {
            sum += len;
            number++;

            if (len > max_len)
                max_len = len;
        }
    }

    printf("hashmap.islands = %u, hashmap.sum = %lu, avg length = %lu, max len = %u\n", number, sum,
           (sum/number), max_len);

    if (dump)
    {
        for (unsigned j = 0; j < hashmap.table.size(); j++)
        {
            printf("%u", (!hashmap.table[j].is_empty()));
            if (j % 250 == 0)
                printf("\n");
        }
    }
    printf("OK :)\n");
}

static void real_test_case_theory_vs_practice(float alpha, bool record_hits)
{
    printf("\n%s\n\n", __FUNCTION__);

    constexpr unsigned capacity {100003};
    constexpr unsigned search_num {50000};
    constexpr unsigned uniwersum_size {1000000000};

    static common::Hashmap<capacity, common::int_holder, common::Limited_linear_hash> linear_hashmap;
    static common::Hashmap<capacity, common::int_holder, common::Limited_quadratic_hash> quadratic_hashmap;
    static common::Hashmap<capacity, common::int_holder, common::Double_hash> double_hashmap;

    common::int_holder basic_config;
    basic_config.mark = false;
    std::vector<int> log_hits;

    srand(time(nullptr));

    // only inserts;
    unsigned inserts = (alpha*capacity*1.0f);
    for (unsigned i = 0; i < inserts; i++)
    {
        basic_config.content = (rand()%uniwersum_size);
        if (record_hits)
            log_hits.push_back(basic_config.content);
        linear_hashmap.insert(basic_config);
        quadratic_hashmap.insert(basic_config);
        double_hashmap.insert(basic_config);
    }

    printf("Linear alpha = %f, quadratic alpha = %f, double alpha = %f\n",
           (linear_hashmap.size()*1.0f/capacity),
           (quadratic_hashmap.size()*1.0f/capacity),
           (double_hashmap.size()*1.0f/capacity));

    // only search;Here collisions as comparisions number.
    linear_hashmap.collisions = 0;
    quadratic_hashmap.collisions = 0;
    double_hashmap.collisions = 0;
    unsigned members_hits = 0;
    for (unsigned i = 0; i < search_num; i++)
    {
        if (!record_hits)
            basic_config.content = (rand()%uniwersum_size);
        else
        {
            unsigned index = (rand()%log_hits.size());
            basic_config.content = log_hits[index];
        }
        bool hit = linear_hashmap.member(basic_config);
        if (hit)
            members_hits++;
        hit = quadratic_hashmap.member(basic_config);
        if (hit)
            members_hits++;
        hit = double_hashmap.member(basic_config);
        if (hit)
            members_hits++;
    }

    printf("Measured:\n");
    printf("Linear comparisions per search = %f, quadratic comparisions per search = %f,"
           "double comparisions per search = %f\n",
           (linear_hashmap.collisions*1.0f/search_num),
           (quadratic_hashmap.collisions*1.0f/search_num),
           (double_hashmap.collisions*1.0f/search_num));
    printf("%u\n", members_hits);

    printf("Theory:\n");

    if (!record_hits)
    {
        const float linear_comp = 0.5f + 1.0f/(2.0f*(1.0f-alpha)*(1.0f-alpha));
        const float double_comp = 1.0f/(1.0f - alpha);
        printf("Linear comparisions per search = %f, quadratic comparisions per search = %f,"
               "double comparisions per search = %f\n",
               linear_comp, 0.0f, double_comp);
    }
    else
    {
        const float linear_comp = 0.5f + 1.0f/(2.0f*(1.0f-alpha));
        const float double_comp = (-1.0f/alpha)*log(1.0f - alpha);
        printf("Linear comparisions per search = %f, quadratic comparisions per search = %f,"
               "double comparisions per search = %f\n",
               linear_comp, 0.0f, double_comp);
    }
    printf(":)");
}

}

int main()
{
    basics::basic_test_case();
    hashmap_tests::real_test_case_theory_vs_practice(0.65f, false);
    hashmap_tests::real_test_case_theory_vs_practice(0.75f, false);
    hashmap_tests::real_test_case_theory_vs_practice(0.85f, false);
    hashmap_tests::real_test_case_theory_vs_practice(0.95f, false);

    hashmap_tests::real_test_case_theory_vs_practice(0.65f, true);
    hashmap_tests::real_test_case_theory_vs_practice(0.75f, true);
    hashmap_tests::real_test_case_theory_vs_practice(0.85f, true);
    hashmap_tests::real_test_case_theory_vs_practice(0.95f, true);

    real_tests::real_test_case();
    hashmap_tests::real_test_case_only_hashmap();
    return 0;
}
