#ifndef FR_PARSER_H
#define FR_PARSER_H

#include <vector>
#include <string>
//-----------------------------------------------------------------------------
// collection of functions to parse SQL scripts
class Parser
{
public:
	static bool stripSql(std::string &sql);
	static std::string::size_type nextToken(std::string& in, std::string& out);
	static std::string::size_type getTableNames(std::vector<std::string>& list, std::string sql);
};
//-----------------------------------------------------------------------------
#endif
