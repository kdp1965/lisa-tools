// ------------------------------------------------------------------------------
// (c) Copyright, Ken Pettit, BSD License
//         All Rights Reserved
// ------------------------------------------------------------------------------
//
//  File        : main.cpp
//  Revision    : 1.0
//  Author      : Ken Pettit
//  Created     : 07/11/2011
//
// Description:  
//    Main entry point for assembler framework.
//
// Modifications:
//
//    Author            Date        Ver  Description
//    ================  ==========  ===  =======================================
//    Ken Pettit        07/11/2011  1.0  Initial version
//
// ------------------------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

bool gVerbose = false;

struct ArchiveEntry
{
    std::vector<std::string> lines;
    int  filePos;
};

typedef std::map<std::string, ArchiveEntry> archive_t;

/*
================================================================================
Load an existing archive
================================================================================
*/
bool load_archive(const std::string& filename, archive_t& archive, int create)
{
    std::ifstream infile(filename);
    if (!infile.is_open())
    {
        if (create)
            return true;
        std::cerr << "Error opening archive file: " << filename << "\n";
        return false;
    }

    std::string line;
    std::map<std::string, int> toc;
    int current_line_number = 0;

    // Read header
    if (!std::getline(infile, line) || line != "LISALIB")
    {
        std::cerr << "Invalid archive format (missing LISALIB header).\n";
        return false;
    }
    current_line_number++;

    // Read TOC header
    if (!std::getline(infile, line) || line.substr(0, 6) != "# TOC@")
    {
        std::cerr << "Invalid archive format (missing TOC).\n";
        return false;
    }
    current_line_number++;

    // Read SYM header
    if (!std::getline(infile, line) || line.substr(0, 6) != "# SYM@")
    {
        std::cerr << "Invalid archive format (missing SYM).\n";
        return false;
    }
    current_line_number++;

    // Load members
    while (std::getline(infile, line))
    {
        current_line_number++;
        if (line.substr(0, 9) != "# Member:")
        {
            if (line.substr(0, 6) == "# TOC:")
            {
                break;
            }
            else if (line.size() == 0)
                continue;
            std::cerr << "Expected # Member: line, got: " << line << "\n";
            return false;
        }

        std::istringstream iss(line.substr(9));
        std::string member_name;
        int num_lines;
        if (!std::getline(iss, member_name, ',') || !(iss >> num_lines))
        {
            std::cerr << "Malformed Member header: " << line << "\n";
            return false;
        }
        member_name.erase(0, member_name.find_first_not_of(" ")); // Trim leading space

        ArchiveEntry entry;
        for (int i = 0; i < num_lines; ++i)
        {
            if (!std::getline(infile, line))
            {
                std::cerr << "Unexpected EOF inside member: " << member_name << "\n";
                return false;
            }
            entry.lines.push_back(line);
            current_line_number++;
        }

        archive[member_name] = entry;
    }

    // Test for TOC entry
    if (line.substr(0, 6) == "# TOC:")
    {
        int toc_count = std::stoi(line.substr(6));

        // Read TOC entries
        for (int i = 0; i < toc_count; ++i)
        {
            if (!std::getline(infile, line))
            {
                std::cerr << "Unexpected EOF while reading TOC.\n";
                return false;
            }
            current_line_number++;

            std::istringstream iss(line);
            std::string filename;
            int offset;
            if (!std::getline(iss, filename, ':') || !(iss >> offset))
            {
                std::cerr << "Malformed TOC entry: " << line << "\n";
                return false;
            }

            toc[filename] = offset;
        }

        // Skip any blank separator line
        std::getline(infile, line);
        current_line_number++;
    }

    return true;
}

/*
================================================================================
Display program usage
================================================================================
*/
bool save_archive(const std::string& filename, const std::map<std::string, ArchiveEntry>& archive)
{
    std::ofstream outfile(filename);
    if (!outfile.is_open())
    {
        std::cerr << "Error opening archive file for writing: " << filename << "\n";
        return false;
    }

    // Write magic header
    outfile << "LISALIB\n";

    // Write TOC header location
    outfile << "# TOC@ ";
    int tocSeek = outfile.tellp();
    outfile << "         \n";

    // Write SYM header location
    outfile << "# SYM@ ";
    int symSeek = outfile.tellp();
    outfile << "         \n";

    // Reserve space to record where each file starts
    std::map<std::string, int> toc_offsets;

    // Blank line separator (optional, keeps things clean)
    outfile << "\n";

    // Create an empty public symbol table
    std::map<std::string, std::string> symTable;

    // Write members
    for (const auto& [name, entry] : archive)
    {
        toc_offsets[name] = outfile.tellp();

        outfile << "# Member: " << name << ", " << entry.lines.size() << "\n";
        for (const std::string& line : entry.lines)
        {
            // Write the line to the output
            outfile << line << "\n";

            // Test for public symbol
            if (line[0] == 'p')
            {
                // Find first space
                size_t first_space = line.find(' ');
                if (first_space != std::string::npos)
                {
                    // Find second space after first
                    size_t second_space = line.find(' ', first_space + 1);
                    if (second_space != std::string::npos)
                    {
                        // Now extract symbol name between first_space and second_space
                        std::string symbol = line.substr(first_space + 1, second_space - first_space - 1);

                        // Test if this symbol duplicated
                        auto s = symTable.find(symbol);
                        if (s != symTable.end())
                        {
                            printf("Error: symbol %s duplicated in %s and %s\n", 
                                    symbol.c_str(), name.c_str(), s->second.c_str());
                            continue;
                        }

                        // Add this symbol to the symTable
                        symTable[symbol] = name;
                    }
                }
            }
        }
        outfile << "\n";
    }

    int tocLoc = outfile.tellp();
    outfile << "# TOC: " << archive.size() << "\n";

    // Write TOC entries
    for (const auto& [name, offset] : toc_offsets)
    {
        outfile << name << ": " << offset << "\n";
    }

    int symLoc = outfile.tellp();
    outfile << "# SYM: " << archive.size() << "\n";

    // Write SYM entries
    for (const auto& [name, file] : symTable)
    {
        outfile << name << ": " << file << "\n";
    }

    // Update TOC location
    outfile.seekp(tocSeek);
    outfile << tocLoc;

    // Update SYM location
    outfile.seekp(symSeek);
    outfile << symLoc;

    return true;
}

/*
================================================================================
Display program usage
================================================================================
*/
bool load_member_file(const std::string& filename, ArchiveEntry& entry)
{
    std::ifstream infile(filename);
    if (!infile.is_open())
    {
        std::cerr << "Error opening member file: " << filename << "\n";
        return false;
    }

    std::string line;
    while (std::getline(infile, line))
    {
        // Strip \r if present (in case of Windows CRLF)
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        entry.lines.push_back(line);
    }

    return true;
}

/*
================================================================================
Display program usage
================================================================================
*/
void usage(const char *name)
{
    printf("\nusage:  %s [-acdhrstx] archive [member]...\n", name);
    printf("\nOptions:\n");
    printf("   -a    Add members to the archive\n");
    printf("   -c    Create the archive\n");
    printf("   -d    Delete members from the archive\n");
    printf("   -h    Show this help\n");
    printf("   -t    List members of the archive\n");
    printf("   -s    Build symbol table\n");
    printf("   -r    Replace (or add) members to the archive\n");
    printf("   -x    Extract members from the archive\n\n");
}

/*
================================================================================
Main entry point for asm engine framework
================================================================================
*/
int main(int argc, char* argv[])
{
    int         c;
    int         add = 0;
    int         extract = 0;
    int         del = 0;
    int         create = 0;
    int         list = 0;
    int         replace = 0;
    int         symbol = 0;
    int         sum;
    bool        ret;
    FILE       *ar;
    char       *pFile;
    archive_t   archive;

    // Parse options
    while ((c = getopt(argc, argv, "acdhrstvx")) != -1)
    {
        switch (c)
        {
           case 'a':
              add = 1;
              break;

           case 'd':
              del = 1;
              break;

           case 'c':
              create = 1;
              break;

           case 'r':
              replace = 1;
              break;

           case 's':
              symbol = 1;
              break;

           case 't':
              list = 1;
              break;

           case 'h':
              usage(argv[0]);
              return 0;

           case 'x':
              extract = 1;
              break;

           case 'v':
              gVerbose = true;
              break;
        }
    }

    // Ensure exactly one operation specified
    sum = add + del + extract + list + replace;
    if (sum == 0)
    {
        printf("Please specify one of '-a -d -r -t -x'\n");
        return 1;
    }

    if (sum > 1)
    {
        printf("Please specify ONLY one of '-a -d -r -t -x'\n");
        return 1;
    }
   
    // Test if archive name given
    if (optind == argc)
    {
        usage(argv[0]);
        return 1;
    }

    // If member needed, verify it was provided
    if ((extract || del || replace || add) && (optind + 1 == argc))
    {
        const char *pOp;

        if (extract)
            pOp = "extract";
        else if (del)
            pOp = "delete";
        else if (add)
            pOp = "add";
        else if (replace)
            pOp = "replace";
        printf("\nPlease provide member for %s operation\n", pOp);
        usage(argv[0]);
        return 1;
    }

    // Open the archive
    std::string filename = argv[optind++];
    ret = load_archive(filename, archive, create);

    // Test if adding or replacing 
    if (add || replace)
    {
        ArchiveEntry new_entry;
        while (optind < argc)
        {
            // Get the base filename
            pFile = argv[optind] + strlen(argv[optind]) - 1;
            while (pFile > argv[optind] && *pFile != '/')
                pFile--;
            if (*pFile == '/')
                pFile++;

            new_entry.lines.clear();
            // Test if entry already exists
            if (archive.find(pFile) != archive.end() && add)
            {
                printf("Member %s already exists.  Use -r to replace\n", pFile);
            }
            else
            {
                if (load_member_file(argv[optind], new_entry))
                {
                    if (gVerbose)
                        printf("Adding %s, Lines: %ld\n", pFile, new_entry.lines.size());
                    archive[pFile] = new_entry;
                }
            }
            optind++;
        }

        // Save the archive
        save_archive(filename, archive);
    }

    //====================================================================== 
    // Test for list request
    //====================================================================== 
    if (list)
    {
        // Loop for all entries
        for (auto it = archive.begin(); it != archive.end(); it++)
        {
            // Print this entry
            printf("%s: %ld lines\n", it->first.c_str(), it->second.lines.size());

            // Now print any public symbols
            for (auto lit = it->second.lines.begin(); lit != it->second.lines.end(); lit++)
            {
                if (lit->substr(0,1) == "p")
                    printf("   %s\n", lit->c_str());
            }
        } 
    }

    //====================================================================== 
    // Test for delete request
    //====================================================================== 
    if (del)
    {
        int delCount = 0;

        // Loop for all args and delete them
        for (; optind < argc; optind++)
        {
            // Get next key
            std::string key = argv[optind];

            // Test if this member is present
            if (archive.find(key) != archive.end())
            {
                // Delete the member
                archive.erase(key);
                delCount++;
            }
            else
                printf("Member %s not found in archive\n", argv[optind]);
        }

        // If we deleted anything, save the archive
        save_archive(filename, archive);
    }

    //====================================================================== 
    // Test for extract request
    //====================================================================== 
    if (extract)
    {
        // Loop for all args and delete them
        for (; optind < argc; optind++)
        {
            // Get next key
            std::string key = argv[optind];

            // Test if this member is present
            auto it = archive.find(key);
            if (it != archive.end())
            {
                // Extract the member
                std::ofstream outfile(key);
                if (!outfile.is_open())
                {
                    std::cerr << "Error opening " << key << " for writing\n";
                    continue;
                }

                // Write contents to file
                for (const std::string& line : it->second.lines)
                {
                    outfile << line << "\n";
                }
            }
        }
    }
    return 0;
}

// vim: et sw=4 ts=4

