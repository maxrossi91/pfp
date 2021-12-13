//
//  vcf.cpp
//
//  Copyright 2020 Marco Oliva. All rights reserved.
//

#include <vcf.hpp>
#include <pfp_algo.hpp>

const std::string vcfbwt::VCF::vcf_freq = "AF";


//------------------------------------------------------------------------------

vcfbwt::Contig::iterator::iterator(const Contig& contig, std::size_t genotype) :
contig_(contig), genotype(genotype),
ref_it_(0), sam_it_(0), var_it_(0), curr_var_it_(0), prev_variation_it(0),
curr_char_(NULL), contig_length_(contig.reference_.size())
{
    // Compute contig length, might take some time
    long long int indels = 0; // could be negative, so int
    for (std::size_t i = 0; i < this->contig_.variations.size(); i++)
    {
        std::size_t var_id = this->contig_.variations[i];
        int var_genotype = this->contig_.genotypes[i][this->genotype];
        if (var_genotype != 0)
        {
            indels += contig_.variations_list[var_id].alt[var_genotype].size() -
                      contig_.variations_list[var_id].ref_len;
        }
        
    }
    contig_length_ = contig_length_ + indels;
    while (var_it_ < contig_.variations.size() and contig_.genotypes[var_it_][genotype] == 0)
    { var_it_++; }
    this->operator++();
}

bool
vcfbwt::Contig::iterator::end() { return ref_it_ > this->contig_.reference_.size(); }

const char
vcfbwt::Contig::iterator::operator*() { return *curr_char_; }

bool
vcfbwt::Contig::iterator::in_a_variation()
{
    const Variation& curr_variation = contig_.variations_list[contig_.variations[var_it_]];
    return (ref_it_ == curr_variation.pos);
}

std::size_t
vcfbwt::Contig::iterator::next_variation() const
{
    if (var_it_ < contig_.variations.size()) { return contig_.get_variation(var_it_).pos; }
    else { return contig_.reference_.size() - 1; }
}

std::size_t
vcfbwt::Contig::iterator::prev_variation() const
{
    if (var_it_ == 0) { spdlog::error("vcfbwt::Contig::iterator::prev_variation() var_it == 0"); std::exit(EXIT_FAILURE); }
    return contig_.get_variation(prev_variation_it).pos;
}

std::size_t
vcfbwt::Contig::iterator::next_variation_distance() const
{
    return this->next_variation() - (this->ref_it_ - 1);
}

void
vcfbwt::Contig::iterator::operator++()
{
    // Ci sono ancora variazioni da processare
    if (var_it_ < contig_.variations.size())
    {
        const Variation& curr_variation = contig_.variations_list[contig_.variations[var_it_]];
        
        if (ref_it_ < curr_variation.pos)
        {
            curr_char_ = &(contig_.reference_[ref_it_]); ref_it_++; sam_it_++;
            return;
        }
        
        // Più nucleotidi nella variaizione
        int var_genotype = contig_.genotypes[var_it_][genotype];
        if (curr_variation.alt[var_genotype].size() > 1)
        {
            if (curr_var_it_ < curr_variation.alt[var_genotype].size() - 1)
            {
                curr_char_ = & curr_variation.alt[var_genotype][curr_var_it_];
                curr_var_it_++; sam_it_++;
            }
            else
            {
                curr_char_ = & curr_variation.alt[var_genotype].back();
                prev_variation_it = var_it_;
                var_it_++;
                while (var_it_ < contig_.variations.size() and contig_.genotypes[var_it_][genotype] == 0)
                { var_it_++; }
                curr_var_it_ = 0; ref_it_ += curr_variation.ref_len; sam_it_++;
            }
            
        }
        else
        {
            curr_char_ = & curr_variation.alt[var_genotype].front();
            ref_it_ += curr_variation.ref_len; sam_it_++; prev_variation_it = var_it_;
            var_it_++;
            while (var_it_ < contig_.variations.size() and contig_.genotypes[var_it_][genotype] == 0)
            { var_it_++; }
        }
    }
    // Non ci sono più variazioni, itera sulla reference
    else { curr_char_ = &(contig_.reference_[ref_it_]); ref_it_++; sam_it_++; }
}

void
vcfbwt::Contig::iterator::go_to(std::size_t i)
{
    if (i >= this->contig_.reference_.size())
    {
        spdlog::error("vcfbwt::Contig::iterator::go_to(std::size_t i) Error: i >= reference.size()");
        std::exit(EXIT_FAILURE);
    }
    if (i < ref_it_)
    {
        spdlog::error("vcfbwt::Contig::iterator::go_to(std::size_t i) Error: i < ref_it_ (going back not allowed)");
        std::exit(EXIT_FAILURE);
    }
    
    // Not an efficient implementation, TODO: make it more efficient
    while (ref_it_ < i)
    {
        this->operator++();
    }
}

//------------------------------------------------------------------------------

vcfbwt::Sample::iterator::iterator(const Sample& sample, std::size_t genotype) :
sample_(sample), genotype(genotype)
{
    v_it_ = sample_.contigs.begin();
    if(v_it_ != sample_.contigs.end())
        c_it_ = new vcfbwt::Contig::iterator(*v_it_, genotype);
}

bool
vcfbwt::Sample::iterator::end() { return (c_it_->end() and (v_it_ == sample_.contigs.end())); }

const char
vcfbwt::Sample::iterator::operator*() { return c_it_->operator*(); }

void
vcfbwt::Sample::iterator::operator++()
{
    c_it_->operator++();
    if(c_it_->end() and (++v_it_ != sample_.contigs.end()))
    {
        delete c_it_;
        c_it_ = new vcfbwt::Contig::iterator(*v_it_, genotype);
    }
}

//------------------------------------------------------------------------------

void
vcfbwt::VCF::init_samples(const std::string& samples_path)
{
    spdlog::info("Reading samples file: {}", samples_path);
    std::ifstream in_stream(samples_path);
    if (not in_stream.is_open()) { spdlog::error("Error while opening {}", samples_path); }

    for (std::string line; getline( in_stream, line);)
    {
        this->input_samples.insert(line);
    }
    spdlog::info("Done reading {}, found {} samples", samples_path, this->input_samples.size());

    for (auto& sample_id : input_samples) { spdlog::info(sample_id); }
}

void
vcfbwt::VCF::init_ref(const std::string& ref_path, bool last)
{
    spdlog::info("Reading reference file: {}", ref_path);
    std::ifstream in_stream(ref_path);
    if (not is_gzipped(in_stream))
    {
        spdlog::error("Reference file expected gzip compressed");
        std::exit(EXIT_FAILURE);
    }
    
    zstr::istream is(in_stream);
    std::string line;
    
    while (getline(is, line)) 
    { 
        // TODO: Remove the second reference append when we can use multiple ReferenceParses.
        if ( not (line.empty() or line[0] == '>') ) { references.back().append(line); reference.append(line);} 
        else 
        { 
            // Update lengths
            if( ref_sum_lengths.size() > 1 ) ref_sum_lengths.push_back(ref_sum_lengths.back());
            else ref_sum_lengths.push_back(0);
            if(references.size()>0) ref_sum_lengths.back() += references.back().size();
            // Create the new reference
            std::string name = line.substr(1,line.find(' ')-1); // -1 to remove the space
            spdlog::info("Read contig {}.", name);
            // TODO: store also the description
            references_id.insert(std::make_pair(name, references.size())); 
            references.push_back(""); 
            // Add contig
            this->variations.push_back(std::vector<Variation>());
            this->contigs.push_back(Contig(name, references.back(), this->variations.back(), ref_sum_lengths.back()));
        }
    }
    if (not last) { references.back().push_back(pfp::SPECIAL_TYPES::DOLLAR_PRIME); reference.push_back(pfp::SPECIAL_TYPES::DOLLAR_PRIME);}

    // This is considered with  the next round
    // if( ref_sum_lengths.size() > 1 ) ref_sum_lengths.push_back(ref_sum_lengths.back())
    // ref_sum_lengths.back() += references.back().size();

    spdlog::info("Done reading {}", ref_path);
}

//------------------------------------------------------------------------------

void
vcfbwt::VCF::init_vcf(const std::string& vcf_path,
                      std::vector<Sample>& l_samples, 
                      std::unordered_map<std::string, std::size_t>& l_samples_id)
{
    // open VCF file
    htsFile * inf = bcf_open(vcf_path.c_str(), "r");
    if (inf == NULL)
    {
        spdlog::error("Can't open vcf file: {}", vcf_path);
        std::exit(EXIT_FAILURE);
    }
    
    spdlog::info("Parsing vcf: {}", vcf_path);
    
    // read header
    bcf_hdr_t *hdr = bcf_hdr_read(inf);
    
    // get l_samples ids from header
    std::size_t n_samples = bcf_hdr_nsamples(hdr);
    // TODO: We need to make this general to be able to handle different species
    if (this->max_samples == 0) { set_max_samples(n_samples); } 

    // We can add the list of samples 
    std::size_t size_before = l_samples.size();
    for (std::size_t i = 0; i < std::min(n_samples, this->max_samples); i++)
    {
        vcfbwt::Sample s(std::string(hdr->samples[i]));
        if (l_samples_id.find(s.id()) == l_samples_id.end())
        {
            l_samples.push_back(s);
            l_samples_id.insert(std::make_pair(s.id(), i));
        }
    }
    spdlog::debug("{} new l_samples in the vcf, tot: {}", l_samples.size() - size_before, l_samples.size());
    
    // struct for storing each record
    bcf1_t *rec = bcf_init();
    if (rec == NULL)
    {
        spdlog::error("Error while parsing vcf file: {}", vcf_path);
        bcf_close(inf);
        bcf_hdr_destroy(hdr);
        std::exit(EXIT_FAILURE);
    }
    
    int rid = -1; // Current contig id in the vcf file;
    std::string contig_name = ""; // Current contig name in the vcf file;
    size_t contig_id = 0; // Current contig id in the vcf file;
    size_t offset = 0;
    std::vector<size_t> c_id_list; 

    // start parsing
    while (bcf_read(inf, hdr, rec) == 0)
    {
        // Check if we have a different contig
        if(rid != rec->rid)
        {
            rid = rec->rid;
            const char* c_name = bcf_hdr_id2name(hdr, rid);
            // Get the contig id in the list of contigs
            auto it = this->references_id.find(std::string(c_name));
            if (it != this->references_id.end())
            {
                contig_id = it->second;
                c_id_list.push_back(contig_id);
                contig_name = std::string(c_name);
                offset = contigs[contig_id].offset(); // when using multiple vcfs
                // Add contigs to samples
                for (std::size_t i = 0; i < std::min(n_samples, this->max_samples); i++)
                {
                    std::string sample_name = std::string(hdr->samples[i]);
                    if((input_samples.empty()) or input_samples.contains(sample_name))
                    {
                        vcfbwt::Contig c(this->contigs[contig_id]);
                        auto id = l_samples_id.find(sample_name);
                        l_samples[id->second].contigs.push_back(c); 
                        // I am assuming that each contig comes from only one VCF file
                        // TODO: Add check that a conting has not been already included. 
                    }
                }   
            }
            else spdlog::error("Unknown contig {} in VCF file.", c_name); // TODO: Consider skipping it
        }

        vcfbwt::Variation var;
        
        var.ref_len = rec->rlen;
        // std::size_t offset = i != 0 ? ref_sum_lengths[i-1] : 0; // when using multiple vcfs
        var.pos = rec->pos + offset;
        var.freq = 0;
        
        // get all alternate alleles
        bcf_unpack(rec, BCF_UN_ALL);
        int type = bcf_get_variant_types(rec);
        for (int allele_idx = 0; allele_idx < rec->n_allele; allele_idx++)
        {
            var.alt.push_back(rec->d.allele[allele_idx]);
        }
        
        int32_t *gt_arr = NULL, ngt_arr = 0;
        int ngt = bcf_get_genotypes(hdr, rec, &gt_arr, &ngt_arr);
        if ( ngt > 0 )
        {
            int max_ploidy = ngt/n_samples;
            bool skip_this_variation = false;
            for (std::size_t i_s = 0; i_s < n_samples; i_s++)
            {
                if (skip_this_variation) { break; }
                int32_t *ptr = gt_arr + i_s * max_ploidy;
                std::vector<int> alleles_idx(max_ploidy, 0);
                bool alt_alleles_set = false;
                for (std::size_t j = 0; j < max_ploidy; j++)
                {
                    // if true, the sample has smaller ploidy
                    if ( ptr[j]==bcf_int32_vector_end ) { break; }

                    // missing allele
                    if ( bcf_gt_is_missing(ptr[j]) ) { continue; }
                    
                    if (bcf_gt_allele(ptr[j]))
                    {
                        // the VCF 0-based allele index
                        int allele_index = bcf_gt_allele(ptr[j]);
                        
                        // Skip symbolic allele
                        if (var.alt[allele_index][0] == '<')
                        {
                            spdlog::debug("vcfbwt::VCF::init_vcf: Skipping symbolic allele at pos {}", var.pos);
                            skip_this_variation = true;
                            continue;
                        }
                        alleles_idx[j] = allele_index;
                        alt_alleles_set = true;
                    }
                }
                
                if (alt_alleles_set)
                {
                    auto id = l_samples_id.find(std::string(hdr->samples[i_s]));
                    if ((id != l_samples_id.end() and id->second < max_samples) and
                    ((input_samples.empty()) or input_samples.contains(id->first))) // Process only wanted l_samples
                    {
                        // Update frequency, to be normalized by the number of samples when parsing ends
                        var.freq += 1;
                        var.used = true;
                        // Add variation to sample, size() because we have not added the variations to the list yet
                        l_samples[id->second].contigs.back().variations.emplace_back(this->variations[contig_id].size());
                        l_samples[id->second].contigs.back().genotypes.emplace_back(alleles_idx);
                    }
                }
            }
        }
        if (var.used) { this->variations[contig_id].push_back(var); }
        free(gt_arr);
    }
    
    // free allocated memory
    bcf_hdr_destroy(hdr);
    bcf_close(inf);
    bcf_destroy(rec);
    
    // Compute normalized variations frequency
    std::size_t number_of_samples = 0;
    for (auto& s : l_samples) { if (s.contigs.size() > 0) { number_of_samples += 1; } }
    for (auto& c : c_id_list) 
    {
        spdlog::info("Contig {}", this->contigs[c].id());
        for (auto& v : this->variations[c]) { v.freq = v.freq / double(number_of_samples); }
        // print some statistics
        spdlog::info("Variations size [{}]: {}GB", this->variations[c].size(), inGigabytes(this->variations[c].size() * sizeof(Variation)));
        spdlog::info("Reference size: {} GB", inGigabytes(references[c].size()));

    }
    std::size_t tot_a_s = 0;
    for (auto& s : l_samples)
        for(auto& t : s.contigs) tot_a_s += t.variations.size();
    
    spdlog::info("Samples size: {} GB", inGigabytes(tot_a_s * 8));
    
    
}

//------------------------------------------------------------------------------

void
vcfbwt::VCF::init_vcf(const std::string &vcf_path, std::size_t i)
{
    init_vcf(vcf_path, samples, samples_id);
    std::size_t tot_a_s = 0, tot_samples = 0;
    for (auto& s : this->samples)
    {
        for(auto& c : s.contigs) tot_a_s += c.variations.size();
        if (s.contigs.size() > 0) { tot_samples += 1; }
    }
    spdlog::info("Average variations per sample: {}", tot_a_s / tot_samples);
}

//------------------------------------------------------------------------------


void
vcfbwt::VCF::init_multi_ref(const std::vector<std::string>& refs_path)
{
    if (refs_path.size() == 0) { spdlog::error("No reference file provided"); std::exit(EXIT_FAILURE); }
    
    spdlog::info("Opening {} ref files, assuming input order reflects the intended genome order", refs_path.size());
    for (std::size_t i = 0; i < refs_path.size(); i++) { init_ref(refs_path[i], i == (refs_path.size() - 1)); }
}

//------------------------------------------------------------------------------

// We can make the assumption/request that each Sample contig is contained in only one VCF file
// If that is not true, we can ask to merge the VCF files.
void
vcfbwt::VCF::init_multi_vcf(const std::vector<std::string>& vcfs_path)
{
    if (vcfs_path.empty()) { spdlog::error("No vcf file provided"); std::exit(EXIT_FAILURE); }
    
    spdlog::info("Opening {} vcf files, assuming input order reflects the intended genome order", vcfs_path.size());

    std::vector<std::vector<Sample>> tmp_samples_array;
    std::vector<std::vector<Variation>> tmp_variations_array;
    std::vector<std::unordered_map<std::string, std::size_t>> tmp_samples_id;

    tmp_samples_array.resize(vcfs_path.size());
    tmp_samples_id.resize(vcfs_path.size());

    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < vcfs_path.size(); i++)
    {
        init_vcf(vcfs_path[i],
                 tmp_samples_array[i],
                 tmp_samples_id[i]);
    }

    // Merge tmp structures into global structures
    spdlog::info("Merging variations");
    for (std::size_t i = 0; i < vcfs_path.size(); i++)
    {

        for (auto& sample : tmp_samples_array[i])
        {
            if (this->samples_id.find(sample.id()) == this->samples_id.end())
            {
                Sample s(sample.id());
                this->samples.push_back(s);
                this->samples_id.insert(std::make_pair(sample.id(), this->samples.size() - 1));
            }

            for (auto& contig : sample.contigs)
            {
                this->samples[samples_id[sample.id()]].contigs.push_back(std::move(contig));
            }

        }
        tmp_samples_array[i].clear();
        tmp_samples_id[i].clear();
    }

    // print some statistics
    for (size_t c = 0; c < this->contigs.size(); ++c) 
    {
        spdlog::info("Contig {}", this->contigs[c].id());
        spdlog::info("Variations size [{}]: {}GB", this->variations[c].size(), inGigabytes(this->variations[c].size() * sizeof(Variation)));
        spdlog::info("Reference size: {} GB", inGigabytes(references[c].size()));

    }

    std::size_t tot_a_s = 0, tot_samples = 0;
    for (auto& s : this->samples)
    {
        for(auto& c : s.contigs) tot_a_s += s.contigs.size();
        if (s.contigs.size() > 0) { tot_samples += 1; }
    }
    spdlog::info("Samples size: {} GB", inGigabytes(tot_a_s * 8));
    spdlog::info("Average variations per sample: {}", tot_a_s / tot_samples);
}

//------------------------------------------------------------------------------

