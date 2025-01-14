//
//  unit_tests.cpp
//
//  Copyright 2020 Marco Oliva. All rights reserved.
//

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <vcf.hpp>
#include <utils.hpp>
#include <pfp_algo.hpp>
#include <au_pair_algo.hpp>

//------------------------------------------------------------------------------

struct listener : Catch::TestEventListenerBase
{
    using TestEventListenerBase::TestEventListenerBase;
    
    virtual void testCaseStarting(Catch::TestCaseInfo const& testInfo) override
    {
        std::cout << testInfo.tagsAsString() << " " << testInfo.name << std::endl;
    }
};
CATCH_REGISTER_LISTENER(listener)

//------------------------------------------------------------------------------

std::string testfiles_dir = "../tests/files";

std::size_t p_global = 75;
std::size_t w_global = 20;

//------------------------------------------------------------------------------
bool
unparse_and_check(std::string& in_prefix, std::string& what_it_should_be, std::size_t window_length, char DOLLAR, bool n = false)
{
    // Unparse
    std::vector<vcfbwt::size_type> parse;
    std::string parse_ext = n ? vcfbwt::EXT::N_PARSE : vcfbwt::EXT::PARSE;
    std::string dictionary_ext = n ? vcfbwt::EXT::N_DICT : vcfbwt::EXT::DICT;
    vcfbwt::pfp::ParserUtils::read_parse(in_prefix + parse_ext, parse);
    std::vector<std::string> dictionary;
    vcfbwt::pfp::ParserUtils::read_dictionary(in_prefix + dictionary_ext, dictionary);
    
    std::string unparsed;
    for (auto& p : parse)
    {
        if (p > dictionary.size()) { spdlog::error("Something wrong in the parse"); exit(EXIT_FAILURE); }
        std::string dict_string = dictionary[p - 1].substr(0, dictionary[p - 1].size() - window_length);
        unparsed.insert(unparsed.end(), dict_string.begin(), dict_string.end());
    }
    unparsed.append(window_length, DOLLAR);
    
    // Compare the two strings
    std::size_t i = 0;
    while ( ((i < unparsed.size()) and (i < what_it_should_be.size()))
    and (unparsed[i] == what_it_should_be[i])) { i++; }
    spdlog::info("First missmatch: {}", i);
    return ((i == (unparsed.size())) and (i == (what_it_should_be.size())));
}

//------------------------------------------------------------------------------
TEST_CASE( "Initialization", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i; }

    bool all_set = true;
    for (vcfbwt::size_type i = 0; i < 10; i++) { all_set = all_set and (linked_list[i] == i); }

    REQUIRE(all_set);
}

TEST_CASE( "Delete from the middle", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(4);
    linked_list.remove_at(5);
    linked_list.remove_at(6);
    linked_list.remove_at(3);

    REQUIRE(*(linked_list.next_at(2)) == 107);
}

TEST_CASE( "Meet deletions", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(7);
    linked_list.remove_at(8);
    linked_list.remove_at(5);
    linked_list.remove_at(6);

    REQUIRE(*(linked_list.next_at(4)) == 109);
}

TEST_CASE( "Remove first 3 left to right", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(0);
    linked_list.remove_at(1);
    linked_list.remove_at(2);

    REQUIRE(*(linked_list.begin()) == 103);
}

TEST_CASE( "Remove first 3 right to left", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(2);
    linked_list.remove_at(1);
    linked_list.remove_at(0);

    REQUIRE(*(linked_list.begin()) == 103);
}

TEST_CASE( "Remove first 3, 021", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(0);
    linked_list.remove_at(2);
    linked_list.remove_at(1);

    REQUIRE(*(linked_list.begin()) == 103);
}

TEST_CASE( "Remove last 3 in order, left to right", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(7);
    linked_list.remove_at(8);
    linked_list.remove_at(9);

    REQUIRE(linked_list.next_at(6) == linked_list.end());
}

TEST_CASE( "Remove last 3 in order, right to left", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(9);
    linked_list.remove_at(8);
    linked_list.remove_at(7);

    REQUIRE(linked_list.next_at(6) == linked_list.end());
}

TEST_CASE( "Meet deletions at the end", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(5);
    linked_list.remove_at(6);
    linked_list.remove_at(9);
    linked_list.remove_at(7);
    linked_list.remove_at(8);

    REQUIRE(linked_list.next_at(4) == linked_list.end());
}

TEST_CASE( "Meet deletions and prev,next", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(7);
    linked_list.remove_at(8);
    linked_list.remove_at(5);
    linked_list.remove_at(6);

    REQUIRE(*(linked_list.next_at(4)) == 109);
    REQUIRE(*(linked_list.prev(linked_list.next_at(4))) == 104);
}

TEST_CASE( "Remove last and meet", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(7);
    linked_list.remove_at(8);
    linked_list.remove_at(5);
    linked_list.remove_at(6);
    linked_list.remove_at(9);

    REQUIRE(linked_list.next_at(4) == linked_list.end());
}

TEST_CASE( "Remove first and meet", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(3);
    linked_list.remove_at(1);
    linked_list.remove_at(2);
    linked_list.remove_at(0);

    REQUIRE(*(linked_list.begin()) == 104);
    REQUIRE(*(linked_list.next(linked_list.begin())) == 105);
}

TEST_CASE( "Remove last but don't meet", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(7);
    linked_list.remove_at(5);
    linked_list.remove_at(6);
    linked_list.remove_at(9);

    REQUIRE(*(linked_list.next_at(4)) == 108);
    REQUIRE(linked_list.next(linked_list.next_at(4)) == linked_list.end());
}

TEST_CASE( "Remove first but don't meet", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(3);
    linked_list.remove_at(2);
    linked_list.remove_at(0);

    REQUIRE(*(linked_list.begin()) == 101);
    REQUIRE(*(linked_list.next(linked_list.begin())) == 104);
}

TEST_CASE( "Remove everything but first and last", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(7);
    linked_list.remove_at(2);
    linked_list.remove_at(5);
    linked_list.remove_at(6);
    linked_list.remove_at(3);
    linked_list.remove_at(1);
    linked_list.remove_at(4);
    linked_list.remove_at(8);

    REQUIRE(*(linked_list.begin()) == 100);
    REQUIRE(*(linked_list.next(linked_list.begin())) == 109);
}

TEST_CASE( "Remove everything but second and second to last", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(7);
    linked_list.remove_at(2);
    linked_list.remove_at(6);
    linked_list.remove_at(3);
    linked_list.remove_at(0);
    linked_list.remove_at(5);
    linked_list.remove_at(4);
    linked_list.remove_at(9);

    REQUIRE(*(linked_list.begin()) == 101);
    REQUIRE(*(linked_list.next(linked_list.begin())) == 108);
}

TEST_CASE( "Remove everything but third and third to last", "[LinkedList]" )
{
    vcfbwt::pfp::LinkedList<vcfbwt::size_type> linked_list(10);

    for (vcfbwt::size_type i = 0; i < 10; i++) { linked_list[i] = i + 100; }

    linked_list.remove_at(8);
    linked_list.remove_at(2);
    linked_list.remove_at(6);
    linked_list.remove_at(1);
    linked_list.remove_at(0);
    linked_list.remove_at(5);
    linked_list.remove_at(4);
    linked_list.remove_at(9);

    REQUIRE(*(linked_list.begin()) == 103);
    REQUIRE(*(linked_list.next(linked_list.begin())) == 107);
}

//------------------------------------------------------------------------------
TEST_CASE( "Initialization KR", "[KR Window]" )
{
    std::string test_string = "12345";

    vcfbwt::KarpRabinHash kr_window(5);
    kr_window.set_constant(256); kr_window.set_prime(2147483647);
    kr_window.initialize(test_string);

    REQUIRE(kr_window.get_hash() == 842216599);
}

TEST_CASE( "Update 1 charachter", "[KR Window]" )
{
    std::string test_string = "12345";

    vcfbwt::KarpRabinHash kr_window(5);
    kr_window.set_constant(256); kr_window.set_prime(2147483647);
    kr_window.initialize(test_string);

    kr_window.update('1', '6');

    REQUIRE(kr_window.get_hash() == 859059610);
}

TEST_CASE( "Update 2 charachter", "[KR Window]" )
{
    std::string test_string = "12345";

    vcfbwt::KarpRabinHash kr_window(5);
    kr_window.set_constant(256); kr_window.set_prime(2147483647);
    kr_window.initialize(test_string);

    kr_window.update('1', '6');
    kr_window.update('2', '7');

    REQUIRE(kr_window.get_hash() == 875902621);
}

TEST_CASE( "Periodic string", "[KR Window]" )
{
    std::string test_string = "11111";

    vcfbwt::KarpRabinHash kr_window(5);
    kr_window.set_constant(256); kr_window.set_prime(2147483647);
    kr_window.initialize(test_string);

    vcfbwt::hash_type before = kr_window.get_hash();
    kr_window.update('1', '1');
    vcfbwt::hash_type after = kr_window.get_hash();

    REQUIRE(before == 825307539);
    REQUIRE(after  == 825307539);
}

TEST_CASE( "Periodic string of Ns", "[KR Window]" )
{
    std::string test_string = "NNNNNNNNNNNNNNNNNNNN";

    vcfbwt::KarpRabinHash kr_window(test_string.size());
    kr_window.set_constant(256); kr_window.set_prime(2147483647);
    kr_window.initialize(test_string);

    vcfbwt::hash_type before = kr_window.get_hash();
    kr_window.update('N', 'N');
    vcfbwt::hash_type after = kr_window.get_hash();

    REQUIRE(before == 2071690116);
    REQUIRE(after  == 2071690116);
}

TEST_CASE( "String Hash", "[KR Window]" )
{
    std::string test_string = "12345";

    vcfbwt::KarpRabinHash kr_window(5);
    kr_window.initialize(test_string);

    REQUIRE(kr_window.get_hash() == vcfbwt::KarpRabinHash::string_hash(test_string));
}

TEST_CASE( "String Hash 2", "[KR Window]" )
{
    std::string test_string = "12345";

    vcfbwt::KarpRabinHash kr_window(5);
    kr_window.initialize(test_string);
    kr_window.update('1', '6');

    REQUIRE(kr_window.get_hash() == vcfbwt::KarpRabinHash::string_hash("23456"));
}

TEST_CASE( "Reproducing bug", "[KR Window]" )
{
    std::string test_string_1 = ".a.little.late;You.fo";
    std::string test_string_2 = "\5\5\5\5\4You.fo";

    vcfbwt::KarpRabinHash kr_window_1(5, true);
    kr_window_1.initialize(test_string_1.substr(0,5));
    for (std::size_t i = 0; i <= 15; i++) { kr_window_1.update(test_string_1[i], test_string_1[i + 5]); }

    vcfbwt::KarpRabinHash kr_window_2(5, true);
    kr_window_2.initialize(test_string_2.substr(0,5));
    for (std::size_t i = 0; i <= 5; i++) { kr_window_2.update(test_string_2[i], test_string_2[i + 5]); }

    REQUIRE(kr_window_1.get_hash() == vcfbwt::KarpRabinHash::string_hash("ou.fo"));
    REQUIRE(kr_window_2.get_hash() == vcfbwt::KarpRabinHash::string_hash("ou.fo"));
    REQUIRE(kr_window_1.get_hash() % 30 == 0);
    REQUIRE(kr_window_2.get_hash() % 30 == 0);
}

//------------------------------------------------------------------------------

TEST_CASE( "Dictionary size", "[Dictionary]")
{
    vcfbwt::pfp::Dictionary dictionary;

    vcfbwt::size_type elem, tot_elem = 100000;
    for (elem = 0; elem < tot_elem; elem++)
    {
        dictionary.check_and_add(std::to_string(elem));
    }

    bool all_elements_in_dict = true;
    for (elem = 0; elem < tot_elem; elem++)
    {
        all_elements_in_dict + all_elements_in_dict and dictionary.contains(std::to_string(elem));
    }
    REQUIRE(all_elements_in_dict);
    REQUIRE(dictionary.size() == elem);
}

//------------------------------------------------------------------------------

TEST_CASE( "Constructor with samples specified", "[VCF parser]" )
{
    std::string vcf_file_name = testfiles_dir + "/ALL.chrY.phase3_integrated_v2a.20130502.genotypes.vcf.gz";
    std::string ref_file_name = testfiles_dir + "/Y.fa.gz";
    vcfbwt::VCF vcf(ref_file_name, vcf_file_name, "");

    // read_samples list from file
    std::ifstream samples_file(testfiles_dir + "/samples_list.txt");
    std::vector<std::string> samples_list;
    std::copy(std::istream_iterator<std::string>(samples_file),
              std::istream_iterator<std::string>(),
              std::back_inserter(samples_list));

    bool all_match = true;
    for (std::size_t i = 0; i < vcf.size(); i++)
    {
        all_match = all_match  & (vcf[i].id() == samples_list[i]);
    }

    REQUIRE(vcf.size() == samples_list.size());
    REQUIRE(all_match);
}

TEST_CASE( "Constructor", "[VCF parser]" )
{
    std::string vcf_file_name = testfiles_dir + "/ALL.chrY.phase3_integrated_v2a.20130502.genotypes.vcf.gz";
    std::string ref_file_name = testfiles_dir + "/Y.fa.gz";
    vcfbwt::VCF vcf(ref_file_name, vcf_file_name, "");

    // read_samples list from file
    std::ifstream samples_file(testfiles_dir + "/samples_list.txt");
    std::vector<std::string> samples_list;
    std::copy(std::istream_iterator<std::string>(samples_file),
              std::istream_iterator<std::string>(),
              std::back_inserter(samples_list));

    bool all_match = true;
    for (std::size_t i = 0; i < vcf.size(); i++)
    {
        all_match = all_match  & (vcf[i].id() == samples_list[i]);
    }

    REQUIRE(vcf.size() == samples_list.size());
    REQUIRE(all_match);
}

TEST_CASE("Sample: HG00101", "[VCF parser]")
{
    std::string vcf_file_name = testfiles_dir + "/ALL.chrY.phase3_integrated_v2a.20130502.genotypes.vcf.gz";
    std::string ref_file_name = testfiles_dir + "/Y.fa.gz";
    vcfbwt::VCF vcf(ref_file_name, vcf_file_name, "", 2);

    REQUIRE(vcf[1].id() == "HG00101");

    std::string test_sample_path = testfiles_dir + "/HG00101_chrY_H1.fa.gz";
    std::ifstream in_stream(test_sample_path);

    REQUIRE(vcfbwt::is_gzipped(in_stream));

    zstr::istream is(in_stream);
    std::string line, from_fasta;
    while (getline(is, line))
    {
        if (not(line.empty() or line[0] == '>'))
        {
            from_fasta.append(line);
        }
    }

    vcfbwt::Sample::iterator it(vcf[1]);
    std::string from_vcf;
    while (not it.end())
    {
        from_vcf.push_back(*it);
        ++it;
    }

    std::size_t i = 0;
    while (((i < from_vcf.size()) and (i < from_fasta.size())) and (from_vcf[i] == from_fasta[i]))
    {
        i++;
    }
    REQUIRE(((i == (from_vcf.size())) and (i == (from_fasta.size()))));
}

TEST_CASE( "Sample: HG00103", "[VCF parser]" )
{
    std::string vcf_file_name = testfiles_dir + "/ALL.chrY.phase3_integrated_v2a.20130502.genotypes.vcf.gz";
    std::string ref_file_name = testfiles_dir + "/Y.fa.gz";
    vcfbwt::VCF vcf(ref_file_name, vcf_file_name, "", 3);

    REQUIRE(vcf[2].id() == "HG00103");

    std::string test_sample_path = testfiles_dir + "/HG00103_chrY_H1.fa.gz";
    std::ifstream in_stream(test_sample_path);

    REQUIRE(vcfbwt::is_gzipped(in_stream));

    zstr::istream is(in_stream);
    std::string line, from_fasta;
    while (getline(is, line)) { if ( not (line.empty() or line[0] == '>') ) { from_fasta.append(line); } }

    vcfbwt::Sample::iterator it(vcf[2]);
    std::string from_vcf;
    while (not it.end()) { from_vcf.push_back(*it); ++it; }

    std::size_t i = 0;
    while ( ((i < from_vcf.size()) and (i < from_fasta.size()))
    and (from_vcf[i] == from_fasta[i])) { i++; }
    REQUIRE(((i == (from_vcf.size())) and (i == (from_fasta.size()))));
}

TEST_CASE( "Selecting only Sample: HG00103", "[VCF parser]" )
{
    std::string vcf_file_name = testfiles_dir + "/ALL.chrY.phase3_integrated_v2a.20130502.genotypes.vcf.gz";
    std::string ref_file_name = testfiles_dir + "/Y.fa.gz";
    std::string samples_file_name = testfiles_dir + "/allowed_samples_list.txt";
    vcfbwt::VCF vcf(ref_file_name, vcf_file_name, samples_file_name);

    REQUIRE(vcf[0].id() == "HG00103");

    std::string test_sample_path = testfiles_dir + "/HG00103_chrY_H1.fa.gz";
    std::ifstream in_stream(test_sample_path);

    REQUIRE(vcfbwt::is_gzipped(in_stream));

    zstr::istream is(in_stream);
    std::string line, from_fasta;
    while (getline(is, line)) { if ( not (line.empty() or line[0] == '>') ) { from_fasta.append(line); } }

    vcfbwt::Sample::iterator it(vcf[0]);
    std::string from_vcf;
    while (not it.end()) { from_vcf.push_back(*it); ++it; }

    std::size_t i = 0;
    while ( ((i < from_vcf.size()) and (i < from_fasta.size()))
            and (from_vcf[i] == from_fasta[i])) { i++; }
    REQUIRE(((i == (from_vcf.size())) and (i == (from_fasta.size()))));
}

TEST_CASE( "Reference + Sample HG00096, No acceleration", "[PFP algorithm]" )
{
    std::string vcf_file_name = testfiles_dir + "/ALL.chrY.phase3_integrated_v2a.20130502.genotypes.vcf.gz";
    std::string ref_file_name = testfiles_dir + "/Y.fa.gz";
    vcfbwt::VCF vcf(ref_file_name, vcf_file_name, "", 1);

    // Produce dictionary and parsing
    vcfbwt::pfp::Params params;
    params.w = w_global; params.p = p_global;
    params.use_acceleration = false;
    params.compute_occurrences = true;
    vcfbwt::pfp::ReferenceParse reference_parse(vcf.get_reference(), params);

    std::string out_prefix = testfiles_dir + "/parser_out";
    vcfbwt::pfp::ParserVCF main_parser(params, out_prefix, reference_parse);

    vcfbwt::pfp::ParserVCF worker;
    std::size_t tag = 0;
    tag = tag | vcfbwt::pfp::ParserVCF::WORKER;
    tag = tag | vcfbwt::pfp::ParserVCF::UNCOMPRESSED;

    worker.init(params, out_prefix, reference_parse, tag);
    main_parser.register_worker(worker);

    // Run
    worker(vcf[0]);

    // Close the main parser
    main_parser.close();

    // Generate the desired outcome from the test files, reference first
    std::string what_it_should_be;
    what_it_should_be.append(1, vcfbwt::pfp::DOLLAR);
    what_it_should_be.insert(what_it_should_be.end(), vcf.get_reference().begin(), vcf.get_reference().end());
    what_it_should_be.append(params.w - 1, vcfbwt::pfp::DOLLAR_PRIME);
    what_it_should_be.append(1, vcfbwt::pfp::DOLLAR_SEQUENCE);

    std::string test_sample_path = testfiles_dir + "/HG00096_chrY_H1.fa.gz";
    std::ifstream in_stream(test_sample_path);
    zstr::istream is(in_stream);
    std::string line, from_fasta;
    while (getline(is, line)) { if ( not (line.empty() or line[0] == '>') ) { from_fasta.append(line); } }

    what_it_should_be.insert(what_it_should_be.end(), from_fasta.begin(), from_fasta.end());
    what_it_should_be.append(params.w - 1, vcfbwt::pfp::DOLLAR_PRIME);
    //what_it_should_be.append(1, vcfbwt::pfp::DOLLAR_SEQUENCE);
    what_it_should_be.append(params.w, vcfbwt::pfp::DOLLAR);

    // Check
    bool check = unparse_and_check(out_prefix, what_it_should_be, params.w, vcfbwt::pfp::DOLLAR);
    REQUIRE(check);
}

TEST_CASE( "Reference + Sample HG00096, WITH acceleration", "[PFP algorithm]" )
{
    std::string vcf_file_name = testfiles_dir + "/ALL.chrY.phase3_integrated_v2a.20130502.genotypes.vcf.gz";
    std::string ref_file_name = testfiles_dir + "/Y.fa.gz";
    vcfbwt::VCF vcf(ref_file_name, vcf_file_name, "", 1);

    // Produce dictionary and parsing
    vcfbwt::pfp::Params params;
    params.w = w_global; params.p = p_global;
    params.use_acceleration = true;
    params.compute_occurrences = true;
    vcfbwt::pfp::ReferenceParse reference_parse(vcf.get_reference(), params);

    std::string out_prefix = testfiles_dir + "/parser_out";
    vcfbwt::pfp::ParserVCF main_parser(params, out_prefix, reference_parse);

    vcfbwt::pfp::ParserVCF worker;
    std::size_t tag = 0;
    tag = tag | vcfbwt::pfp::ParserVCF::WORKER;
    tag = tag | vcfbwt::pfp::ParserVCF::UNCOMPRESSED;

    worker.init(params, out_prefix, reference_parse, tag);
    main_parser.register_worker(worker);

    // Run
    worker(vcf[0]);

    // Close the main parser
    main_parser.close();

    // Generate the desired outcome from the test files, reference first
    std::string what_it_should_be;
    what_it_should_be.append(1, vcfbwt::pfp::DOLLAR);
    what_it_should_be.insert(what_it_should_be.end(), vcf.get_reference().begin(), vcf.get_reference().end());
    what_it_should_be.append(params.w - 1, vcfbwt::pfp::DOLLAR_PRIME);
    what_it_should_be.append(1, vcfbwt::pfp::DOLLAR_SEQUENCE);

    std::string test_sample_path = testfiles_dir + "/HG00096_chrY_H1.fa.gz";
    std::ifstream in_stream(test_sample_path);
    zstr::istream is(in_stream);
    std::string line, from_fasta;
    while (getline(is, line)) { if ( not (line.empty() or line[0] == '>') ) { from_fasta.append(line); } }

    what_it_should_be.insert(what_it_should_be.end(), from_fasta.begin(), from_fasta.end());
    what_it_should_be.append(params.w - 1, vcfbwt::pfp::DOLLAR_PRIME);
    //what_it_should_be.append(1, vcfbwt::pfp::DOLLAR_SEQUENCE);
    what_it_should_be.append(params.w, vcfbwt::pfp::DOLLAR);

    // Check
    bool check = unparse_and_check(out_prefix, what_it_should_be, params.w, vcfbwt::pfp::DOLLAR);
    REQUIRE(check);
}

TEST_CASE( "Sample: HG00096, twice chromosome Y", "[VCF parser]" )
{
    std::vector<std::string> vcf_file_names =
            {
                    testfiles_dir + "/ALL.chrY.phase3_integrated_v2a.20130502.genotypes.vcf.gz",
                    testfiles_dir + "/ALL.chrY.phase3_integrated_v2a.20130502.genotypes.vcf.gz"
            };

    std::vector<std::string> ref_file_names =
            {
                    testfiles_dir + "/Y.fa.gz",
                    testfiles_dir + "/Y.fa.gz"
            };

    vcfbwt::VCF vcf(ref_file_names, vcf_file_names, "", 1);

    REQUIRE(vcf[0].id() == "HG00096");

    std::string test_sample_path = testfiles_dir + "/HG00096_chrY_H1.fa.gz";
    std::ifstream in_stream(test_sample_path);

    REQUIRE(vcfbwt::is_gzipped(in_stream));

    zstr::istream is(in_stream);
    std::string line, from_fasta;
    while (getline(is, line)) { if ( not (line.empty() or line[0] == '>') ) { from_fasta.append(line); } }
    from_fasta.push_back(vcfbwt::pfp::DOLLAR_PRIME);
    from_fasta.append(from_fasta);
    from_fasta.pop_back();


    vcfbwt::Sample::iterator it(vcf[0]);
    std::string from_vcf;
    while (not it.end()) { from_vcf.push_back(*it); ++it; }

    std::size_t i = 0;
    while ( ((i < from_vcf.size()) and (i < from_fasta.size()))
            and (from_vcf[i] == from_fasta[i])) { i++; }
    REQUIRE(((i == (from_vcf.size())) and (i == (from_fasta.size()))));
}

TEST_CASE( "Sample: HG00096, fasta", "[PFP Algo]" )
{
    // Produce dictionary and parsing
    vcfbwt::pfp::Params params;
    params.w = w_global; params.p = p_global;
    params.compute_occurrences = true;

    std::string test_sample_path = testfiles_dir + "/HG00096_chrY_H1.fa.gz";
    std::string out_prefix = testfiles_dir + "/HG00096_chrY_H1_tpfa";
    vcfbwt::pfp::ParserFasta main_parser(params, test_sample_path, out_prefix);

    // Run
    main_parser();

    // Close the main parser
    main_parser.close();

    // Generate the desired outcome from the test files, reference first
    std::string what_it_should_be;
    what_it_should_be.append(1, vcfbwt::pfp::DOLLAR);

    std::ifstream in_stream(test_sample_path);
    zstr::istream is(in_stream);
    std::string line, from_fasta;
    while (getline(is, line)) { if ( not (line.empty() or line[0] == '>') ) { from_fasta.append(line); } }

    what_it_should_be.insert(what_it_should_be.end(), from_fasta.begin(), from_fasta.end());
    what_it_should_be.append(params.w - 1, vcfbwt::pfp::DOLLAR_PRIME);
    //what_it_should_be.append(1, vcfbwt::pfp::DOLLAR_SEQUENCE);
    what_it_should_be.append(params.w, vcfbwt::pfp::DOLLAR);

    // Check
    bool check = unparse_and_check(out_prefix, what_it_should_be, params.w, vcfbwt::pfp::DOLLAR);
    REQUIRE(check);
}

TEST_CASE( "Sample: HG00096, text", "[PFP Algo]" )
{
    // Produce dictionary and parsing
    vcfbwt::pfp::Params params;
    params.w = w_global; params.p = p_global;
    params.compute_occurrences = true;

    std::string test_sample_path = testfiles_dir + "/HG00096_chrY_H1.fa.gz";
    std::string out_prefix = testfiles_dir + "/HG00096_chrY_H1_tptxt";
    vcfbwt::pfp::ParserText main_parser(params, test_sample_path, out_prefix);

    // Run
    main_parser();

    // Close the main parser
    main_parser.close();

    // Generate the desired outcome from the test files, reference first
    std::string what_it_should_be;
    what_it_should_be.append(1, vcfbwt::pfp::DOLLAR);

    std::ifstream in_stream(test_sample_path);
    zstr::istream is(in_stream);
    std::string from_text_file((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());

    what_it_should_be.insert(what_it_should_be.end(), from_text_file.begin(), from_text_file.end());
    what_it_should_be.append(params.w, vcfbwt::pfp::DOLLAR);

    // Check
    bool check = unparse_and_check(out_prefix, what_it_should_be, params.w, vcfbwt::pfp::DOLLAR);
    REQUIRE(check);
}

TEST_CASE( "AuPair both removals", "[AuPair]" )
{
    std::string S = "!ACCACATAGGTGAACCTTGAAAATGTTACACTGTGTGAAAAAGTCAGATACAAGAGGCC####"
                    "ACCACATAGGTGAACCTTGAAAATGTTACATTGTGTGAAAAAGTCAGATACAAGAGGCC!!!!";

    std::vector<std::string> dictionary =
    {
    "!ACCACATAGGTG",
    "####ACCACATAGGTG",
    "AATGTTACACTGTGTGAAAAAGTCAG",
    "AATGTTACATTGTGTGAAAAAGTCAG",
    "CTTGAAAATG",
    "GGTGAACCTTG",
    "TCAGATACAAGAGGCC!!!!",
    "TCAGATACAAGAGGCC####"
    };

    std::ofstream dict_file(testfiles_dir + "/au_pair_test_1" + vcfbwt::EXT::DICT);
    for (auto& phrase : dictionary)
    {
        dict_file.write(phrase.c_str(), phrase.size());
        dict_file.put(vcfbwt::pfp::ENDOFWORD);
    }
    dict_file.put(vcfbwt::pfp::ENDOFDICT);
    dict_file.close();

    std::vector<vcfbwt::size_type> parse = {1, 6, 5, 3, 8, 2, 6, 5, 4, 7};
    std::ofstream parse_file(testfiles_dir + "/au_pair_test_1" + vcfbwt::EXT::PARSE);
    parse_file.write((char*)&parse[0], parse.size() * sizeof(vcfbwt::size_type));
    parse_file.close();

    vcfbwt::pfp::AuPair au_pair_algo(testfiles_dir + "/au_pair_test_1", 4, false);

    std::set<std::string_view> removed_trigger_strings;
    std::size_t removed_bytes = au_pair_algo(removed_trigger_strings, 10);

    au_pair_algo.close();

    REQUIRE(!removed_trigger_strings.empty());
    REQUIRE(removed_bytes > 0);

    // Check
    std::string out_prefix = testfiles_dir + "/au_pair_test_1";
    bool check = unparse_and_check(out_prefix, S, 4, '!', true);
    REQUIRE(check);
}

TEST_CASE( "AuPair Reference + Sample HG00096, No acceleration", "[AuPair]" )
{
    std::string vcf_file_name = testfiles_dir + "/ALL.chrY.phase3_integrated_v2a.20130502.genotypes.vcf.gz";
    std::string ref_file_name = testfiles_dir + "/Y.fa.gz";
    vcfbwt::VCF vcf(ref_file_name, vcf_file_name, "", 1);

    // Produce dictionary and parsing
    vcfbwt::pfp::Params params;
    params.w = w_global; params.p = p_global;
    params.use_acceleration = false;
    params.compute_occurrences = true;
    vcfbwt::pfp::ReferenceParse reference_parse(vcf.get_reference(), params);

    std::string out_prefix = testfiles_dir + "/parser_out";
    vcfbwt::pfp::ParserVCF main_parser(params, out_prefix, reference_parse);

    vcfbwt::pfp::ParserVCF worker;
    std::size_t tag = 0;
    tag = tag | vcfbwt::pfp::ParserVCF::WORKER;
    tag = tag | vcfbwt::pfp::ParserVCF::UNCOMPRESSED;

    worker.init(params, out_prefix, reference_parse, tag);
    main_parser.register_worker(worker);

    // Run
    worker(vcf[0]);

    // Close the main parser
    main_parser.close();

    vcfbwt::pfp::AuPair au_pair_algo(out_prefix, w_global, 1);

    std::set<std::string_view> removed_trigger_strings;
    std::size_t removed_bytes = au_pair_algo(removed_trigger_strings, 1000);
    au_pair_algo.close();

    REQUIRE(!removed_trigger_strings.empty());
    REQUIRE(removed_bytes > 0);

    // Generate the desired outcome from the test files, reference first
    std::string what_it_should_be;
    what_it_should_be.append(1, vcfbwt::pfp::DOLLAR);
    what_it_should_be.insert(what_it_should_be.end(), vcf.get_reference().begin(), vcf.get_reference().end());
    what_it_should_be.append(params.w - 1, vcfbwt::pfp::DOLLAR_PRIME);
    what_it_should_be.append(1, vcfbwt::pfp::DOLLAR_SEQUENCE);

    std::string test_sample_path = testfiles_dir + "/HG00096_chrY_H1.fa.gz";
    std::ifstream in_stream(test_sample_path);
    zstr::istream is(in_stream);
    std::string line, from_fasta;
    while (getline(is, line)) { if ( not (line.empty() or line[0] == '>') ) { from_fasta.append(line); } }

    what_it_should_be.insert(what_it_should_be.end(), from_fasta.begin(), from_fasta.end());
    what_it_should_be.append(params.w - 1, vcfbwt::pfp::DOLLAR_PRIME);
    //what_it_should_be.append(1, vcfbwt::pfp::DOLLAR_SEQUENCE);
    what_it_should_be.append(params.w, vcfbwt::pfp::DOLLAR);

    // Check
    bool check = unparse_and_check(out_prefix, what_it_should_be, params.w, vcfbwt::pfp::DOLLAR, true);
    REQUIRE(check);
}

TEST_CASE( "AuPair Reference + Sample HG00096, WITH acceleration", "[AuPair]" )
{
    std::string vcf_file_name = testfiles_dir + "/ALL.chrY.phase3_integrated_v2a.20130502.genotypes.vcf.gz";
    std::string ref_file_name = testfiles_dir + "/Y.fa.gz";
    vcfbwt::VCF vcf(ref_file_name, vcf_file_name, "", 1);

    // Produce dictionary and parsing
    vcfbwt::pfp::Params params;
    params.w = w_global; params.p = p_global;
    params.use_acceleration = true;
    params.compute_occurrences = true;
    vcfbwt::pfp::ReferenceParse reference_parse(vcf.get_reference(), params);

    std::string out_prefix = testfiles_dir + "/parser_out";
    vcfbwt::pfp::ParserVCF main_parser(params, out_prefix, reference_parse);

    vcfbwt::pfp::ParserVCF worker;
    std::size_t tag = 0;
    tag = tag | vcfbwt::pfp::ParserVCF::WORKER;
    tag = tag | vcfbwt::pfp::ParserVCF::UNCOMPRESSED;

    worker.init(params, out_prefix, reference_parse, tag);
    main_parser.register_worker(worker);

    // Run
    worker(vcf[0]);

    // Close the main parser
    main_parser.close();

    vcfbwt::pfp::AuPair au_pair_algo(out_prefix, w_global, false);

    std::set<std::string_view> removed_trigger_strings;
    std::size_t removed_bytes = au_pair_algo(removed_trigger_strings, 1000);
    spdlog::info("Removed: {} bytes", removed_bytes);
    au_pair_algo.close();

    REQUIRE(!removed_trigger_strings.empty());
    REQUIRE(removed_bytes > 0);

    // Generate the desired outcome from the test files, reference first
    std::string what_it_should_be;
    what_it_should_be.append(1, vcfbwt::pfp::DOLLAR);
    what_it_should_be.insert(what_it_should_be.end(), vcf.get_reference().begin(), vcf.get_reference().end());
    what_it_should_be.append(params.w - 1, vcfbwt::pfp::DOLLAR_PRIME);
    what_it_should_be.append(1, vcfbwt::pfp::DOLLAR_SEQUENCE);

    std::string test_sample_path = testfiles_dir + "/HG00096_chrY_H1.fa.gz";
    std::ifstream in_stream(test_sample_path);
    zstr::istream is(in_stream);
    std::string line, from_fasta;
    while (getline(is, line)) { if ( not (line.empty() or line[0] == '>') ) { from_fasta.append(line); } }

    what_it_should_be.insert(what_it_should_be.end(), from_fasta.begin(), from_fasta.end());
    what_it_should_be.append(params.w - 1, vcfbwt::pfp::DOLLAR_PRIME);
    //what_it_should_be.append(1, vcfbwt::pfp::DOLLAR_SEQUENCE);
    what_it_should_be.append(params.w, vcfbwt::pfp::DOLLAR);

    // Check
    bool check = unparse_and_check(out_prefix, what_it_should_be, params.w, vcfbwt::pfp::DOLLAR, true);
    REQUIRE(check);
}

//------------------------------------------------------------------------------

#include <indexed_pq/indexMaxPQ.h>
TEST_CASE("understanding pq", "[PQ]")
{
    std::map<std::string, vcfbwt::size_type> ts_ids;
    ts_ids["TEST1"] = 0;
    ts_ids["TEST2"] = 1;
    ts_ids["TEST3"] = 2;

    indexMaxPQ pq; pq.init(ts_ids.size());
    pq.push(ts_ids["TEST1"], 10);
    pq.push(ts_ids["TEST2"], 20);
    pq.push(ts_ids["TEST3"], 30);

    std::pair<int, int> max = pq.get_max();
    REQUIRE((max.first == 30 and max.second == 2));

    pq.demote(ts_ids["TEST3"], 15);
    max = pq.get_max();

    REQUIRE((max.first == 20 and max.second == 1));

    pq.promote(ts_ids["TEST1"], 40);
    max = pq.get_max();

    REQUIRE((max.first == 40 and max.second == 0));
}

//------------------------------------------------------------------------------

int main( int argc, char* argv[] )
{
    Catch::Session session;

    using namespace Catch::clara;

    auto cli = session.cli() |
    Opt( testfiles_dir, "dir" ) ["--test-dir"] ("specify the directory containing the test dna sequences files") |
    Opt( w_global, "int" ) ["-W"] ("specify w") |
    Opt( p_global, "int" ) ["-P"] ("specify p");

    session.cli(cli);

    int returnCode = session.applyCommandLine(argc, argv);

    if( returnCode != 0 ) return returnCode;

    spdlog::info("Tests running with w: {}\tp: {}", w_global, p_global);

    session.run();
}

