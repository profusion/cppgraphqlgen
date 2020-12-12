// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLGrammar.h"
#include "graphqlservice/GraphQLParse.h"

#include <cstdio>
#include <iostream>
#include <iterator>
#include <stdexcept>

#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <tao/pegtl/contrib/print.hpp>
#include <tao/pegtl/contrib/trace.hpp>

using namespace graphql;

int main(int argc, char** argv)
{
	if (peg::analyze<peg::document>() != 0)
	{
		std::cerr << "cycles without progress detected!" << std::endl;
		return 1;
	}

	try
	{
		if (argc > 1)
		{
			bool trace = false;

			for (int i = 1; i < argc; i++)
			{
				const std::string_view arg = argv[i];
				if (arg == "-t")
				{
					trace = true;
				}
				else if (arg == "-d")
				{
					peg::print_debug<peg::document>(std::cout);
					break;
				}
				else
				{
					if (trace)
					{
						peg::file_input in(arg);
						peg::standard_trace<peg::document>(in);
					}
					else
					{
						peg::ast query = peg::parseFile(arg);
						peg::parse_tree::print_dot(std::cout, *query.root);
					}
				}
			}
		}
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
