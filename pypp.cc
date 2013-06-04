/* pypp.cc: Python-style C preprocessor.
 *
 * Copyright (C) 2007 Bas Wijnen <wijnen@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <shevek/args.hh>
#include <shevek/iostring.hh>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>

#define my_error(x) \
	do { std::cerr << file << ':' << lineno << ':' << x << '\n'; have_error = true; } while (0)

bool have_error = false;
std::string const whitespace (" \t\n\r\v\a\f\0", 8);

class pypp
{
	struct indentation_t
	{
		bool finish, comma;
		unsigned depth;
		indentation_t (unsigned d = 0)
			: finish (false), comma (false), depth (d) {}
	};
	std::stack <indentation_t> indentation;
	bool needarg, bracket, opened, comma;
	std::istream *in;
	std::ostream *out;
	unsigned tabwidth;
	unsigned empty;
	unsigned lineno;
	std::string file;
	bool clean;
	std::string line_info;

	void write_line_info (int offset)
	{
		if (clean)
			return;
		line_info = shevek::rostring ("#line %d \"%s\"\n", lineno + offset - empty, file);
	}

	std::string untab (std::string const &line)
	{
		std::string result;
		std::string::size_type p (line.find ('\t')), old (0);
		while (p != std::string::npos)
		{
			result += line.substr (old, p - old);
			unsigned len = (result.size () + tabwidth - 1)
				% tabwidth + 1;
			result += std::string (len, ' ');
			old = p + 1;
			p = line.find ('\t', p + 1);
		}
		result += line.substr (old);
		return result;
	}

	void read_header ()
	{
		std::string line;
		if (!std::getline (*in, line))
		{
			my_error ("no input");
			return;
		}
		++lineno;
		char dummy;
		unsigned version;
		std::string word;
		std::istringstream s (line);
		s >> dummy >> word >> version;
		if (!s || dummy != '#' || word != "pypp")
		{
			my_error ("invalid header for pypp file");
			return;
		}
		if (version > MY_VERSION)
		{
			my_error ("unsupported version " << version
					<< " (this is version "
					<< MY_VERSION << ')');
			return;
		}
		s >> tabwidth;
		if (!s)
			tabwidth = 8;
	}

	std::string::size_type indent;

	void prefix (bool use_empty = true)
	{
		// terminate current line
		if (!bracket)
		{
			if (comma)
			{
				if (use_empty)
					(*out) << ',';
			}
			else
				(*out) << ';';
		}
		bracket = false;

		if (use_empty)
		{
			if (empty)
			{
				(*out) << '\n';
				--empty;
			}
		}
		else
			(*out) << '\n';

		if (!line_info.empty ())
		{
			(*out) << line_info;
			line_info.clear ();
		}

		if (use_empty)
		{
			// add newlines as needed
			for (unsigned i = 0; i < empty; ++i)
				(*out) << '\n';
			empty = 1;
		}

		// add indentation
		(*out) << std::string (indentation.top ().depth, ' ');
	}

	void close_block (bool real = true)
	{
		if (opened)
			my_error ("closing block while open");
		if (real)
			indentation.pop ();
		prefix (false);
		(*out) << '}';
		write_line_info (1);
		bracket = !indentation.top ().finish;
		indentation.top ().finish = false;
		indentation.top ().comma = false;
		comma = false;
	}

	void parse_line (std::string const &line)
	{
		indent = line.find_first_not_of (whitespace);

		// skip empty lines
		if (indent == std::string::npos)
		{
			++empty;
			return;
		}

		// handle the case if the previous line needs an argument
		if (needarg)
		{
			if (indent > indentation.top ().depth)
			{
				comma = indentation.top ().comma;
				indentation.push (indentation_t (indent));
			}
			else
			{
				// there is no argument
				close_block (false);
			}
		}

		// decreased indentation: close blocks
		while (indent < indentation.top ().depth)
			close_block ();

		// increased indentation without needarg: line continuation
		if (indent > indentation.top ().depth)
		{
			bracket = true;
		}

		// print the prefix, it is needed in any case now
		prefix ();

		// check if this line needs an argument
		std::string::size_type e = line.find_last_not_of (whitespace);
		std::string l;
		if (line[e] == ':')
		{
			needarg = true;
			l = line.substr (indent, e - indent);
		}
		else
		{
			needarg = false;
			l = line.substr (indent);
		}

		// comments should not get a ;
		if (e != 0 && line[e] == '/' && line[e - 1] == '*')
		{
			bracket = true;
		}

		// check if this is a for, while, or if, needing parenthesis
		e = l.find_first_of (whitespace);
		if (e == std::string::npos)
			e = l.size ();
		std::string word = l.substr (0, e);
		std::string::size_type f = l.find_first_not_of (whitespace, e);
		if (f != std::string::npos)
		{
			std::string word2;
			std::string::size_type
				g = l.find_first_of (whitespace, f);
			if (g != std::string::npos)
				word2 = l.substr (f, g - f);
			else
				word2 = l.substr (f);
			if (word == "else" && word2 == "if")
			{
				word = "else if";
				e = g;
			}
		}
		if (word == "else if" || word == "if" || word == "while"
				|| word == "for" || word == "switch"
				|| word == "catch")
		{
			if (opened)
				my_error ("double open");
			(*out) << l.substr (0, e + 1) << '('
				<< l.substr (e + 1);
			opened = true;
		}
		else
		{
			(*out) << l;
		}

		// line starting with '#' shouldn't have a ';'
		if (word[0] == '#')
		{
			bracket = true;
		}

		// if this line needs an argument, close the parenthesis
		if (needarg)
		{
			if (opened)
			{
				(*out) << ')';
				opened = false;
			}
			else
			{
				if (word == "struct" || word == "union" ||
						word == "class")
					indentation.top ().finish = true;
				else if (word == "enum")
				{
					indentation.top ().finish = true;
					indentation.top ().comma = true;
				}
				else if (word == "public" || word == "private"
						|| word == "protected")
				{
					(*out) << ':';
					bracket = true;
					needarg = false;
					return;
				}
				else if (word == "case" || word == "default")
				{
					(*out) << ':';
				}
			}
			// add a new prefix
			bracket = true;
			prefix ();
			(*out) << "{";
			write_line_info (2);
			bracket = true;
		}
	}
public:
	pypp (std::istream &i, std::ostream &o, std::string const &filename,
			bool c)
	{
		clean = c;
		file = filename;
		needarg = false;
		bracket = true;
		opened = false;
		comma = false;
		empty = 0;
		lineno = 0;
		in = &i;
		out = &o;
		indentation.push (indentation_t (0));
		std::string line;
		read_header ();

		if (have_error)
			return;

		write_line_info (1);
		while (std::getline (*in, line))
		{
			++lineno;
			line = untab (line);
			parse_line (line);
		}

		if (opened)
			my_error ("still open at end of input");
		if (needarg)
		{
			prefix ();
			(*out) << '}';
			bracket = true;
		}

		// close remaining blocks
		while (indentation.top ().depth > 0)
			close_block ();
		(*out) << (bracket ? "\n" : ";\n");
	}
};
	
int main (int argc, char **argv)
{
	bool clean (false);
	std::string filename ("standard input");

	shevek::args::option opts[] = {
		shevek::args::option ('n', "name",
				"filename for error reporting.", true,
				filename),
		shevek::args::option (0, "clean", "don't add #line directives.",
				clean, true)
	};
	shevek::args args (argc, argv, opts, 0, 0, "Python-style preprocessor for C code.");

	pypp foo (std::cin, std::cout, filename, clean);

	return have_error;
}
