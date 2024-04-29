#include <span>
#include <gtest/gtest.h>
#include <csp/worker.h>
#include <csp/utils.h>

csp::ScanResult test_pattern(const std::string pattern,const std::string data,size_t offset)
{
    return csp::test_for_pattern(pattern,pattern.length(),std::span(data.c_str(),data.length()),offset);
}

TEST(CspTests, TestPatternScan_NotFound)
{
    //pattern not present in data
    ASSERT_EQ(csp::ScanResult::NothingFound,test_pattern("abc","abefgt",0));
    ASSERT_EQ(csp::ScanResult::NothingFound,test_pattern("abc","abefgt",1));
    ASSERT_EQ(csp::ScanResult::NothingFound,test_pattern("abc","abefgt",2));
    ASSERT_EQ(csp::ScanResult::NothingFound,test_pattern("abc","abefgt",3));
    //pattern present in data
    ASSERT_EQ(csp::ScanResult::NothingFound,test_pattern("abc","ababct",0));
    ASSERT_EQ(csp::ScanResult::NothingFound,test_pattern("abc","ababct",1));
}

TEST(CspTests, TestPatternScan_PatternFound)
{
    //pattern present and offset match
    ASSERT_EQ(csp::ScanResult::PatternFound,test_pattern("abc","ababct",2));
}

TEST(CspTests, TestSplits_Count)
{
    ASSERT_EQ(3,
              csp::calculate_optimal_splits_count(3,1024));
    ASSERT_EQ(1024,
              csp::calculate_optimal_splits_count(3,100*1024*1024));
    ASSERT_EQ(1,
              csp::calculate_optimal_splits_count(1000,1024));
    ASSERT_EQ(1,csp::calculate_optimal_splits_count(1024,1024));
    ASSERT_EQ(1024,
              csp::calculate_optimal_splits_count(1000,100*1024*1024));


}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}