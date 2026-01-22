#include <Parsers/ASTExpressionList.h>
#include <Parsers/ASTIdentifier.h>
#include <Parsers/ASTLiteral.h>
#include <Parsers/ASTSelectQuery.h>
#include <Parsers/ASTSubquery.h>
#include <Parsers/ASTTablesInSelectQuery.h>
#include <Parsers/CommonParsers.h>
#include <Parsers/ExpressionElementParsers.h>
#include <Parsers/ExpressionListParsers.h>
#include <Parsers/IParserBase.h>
#include <Parsers/Kusto/ParserKQLLet.h>
#include <Parsers/Kusto/ParserKQLQuery.h>
#include <Parsers/Kusto/ParserKQLStatement.h>
#include <Parsers/Kusto/Utilities.h>
#include <Parsers/ParserTablesInSelectQuery.h>

namespace DB
{

namespace ErrorCodes
{
    extern const int SYNTAX_ERROR;
}

bool ParserKQLLet::parseImpl(Pos & pos, ASTPtr & node, Expected & expected)
{
    // let variable_name = scalar_expression;
    // let table_name = (tabular_expression);
    
    // Skip the "let" keyword if present (should already be consumed)
    String current_token(pos->begin, pos->end);
    if (current_token == "let")
        ++pos;
    
    if (!isValidKQLPos(pos))
        return false;
    
    // Get the variable/table name
    String var_name(pos->begin, pos->end);
    
    if (pos->type != TokenType::BareWord)
        return false;
    
    ++pos;
    
    // Expect '='
    if (!isValidKQLPos(pos) || pos->type != TokenType::Equals)
        return false;
    
    ++pos;
    
    if (!isValidKQLPos(pos))
        return false;
    
    // Check if it's a tabular expression (starts with '(')
    bool is_tabular = (pos->type == TokenType::OpeningRoundBracket);
    
    ASTPtr expression;
    
    if (is_tabular)
    {
        // Parse tabular expression
        ++pos; // skip '('
        
        // Parse the subquery
        if (!ParserKQLQuery().parse(pos, expression, expected))
            return false;
        
        // Expect closing parenthesis
        if (!isValidKQLPos(pos) || pos->type != TokenType::ClosingRoundBracket)
            return false;
        
        ++pos;
    }
    else
    {
        // Parse scalar expression until semicolon
        String expr_str = getExprFromPipe(pos);
        
        // Find the semicolon position to stop at
        auto expr_end = pos;
        BracketCount bracket_count;
        while (isValidKQLPos(expr_end) && expr_end->type != TokenType::Semicolon)
        {
            bracket_count.count(expr_end);
            if (expr_end->type == TokenType::PipeMark && bracket_count.isZero())
                break;
            ++expr_end;
        }
        
        // Parse the expression
        Tokens expr_tokens(expr_str.data(), expr_str.data() + expr_str.size(), 0, true);
        IParser::Pos expr_pos(expr_tokens, pos.max_depth, pos.max_backtracks);
        
        if (!ParserExpressionWithOptionalAlias(false).parse(expr_pos, expression, expected))
            return false;
        
        // Move position forward
        pos = expr_end;
    }
    
    // Skip optional semicolon
    if (isValidKQLPos(pos) && pos->type == TokenType::Semicolon)
        ++pos;
    
    // Store the let binding in the query context
    // We'll add this to a WITH clause in the main query
    
    // Get or create the SELECT query
    auto select_query = node->as<ASTSelectQuery>();
    if (!select_query)
    {
        auto new_select = std::make_shared<ASTSelectQuery>();
        node = new_select;
        select_query = new_select.get();
    }
    
    // Create or get the WITH clause
    ASTPtr with_expression_list;
    if (select_query->with())
    {
        with_expression_list = select_query->with();
    }
    else
    {
        with_expression_list = std::make_shared<ASTExpressionList>();
        auto with_expression_list_copy = with_expression_list;
        select_query->setExpression(ASTSelectQuery::Expression::WITH, std::move(with_expression_list_copy));
    }
    
    // Add the variable to the WITH clause
    if (is_tabular)
    {
        // For tabular expressions, wrap in a subquery
        auto subquery = std::make_shared<ASTSubquery>();
        subquery->children.push_back(expression);
        subquery->setAlias(var_name);
        with_expression_list->children.push_back(subquery);
    }
    else
    {
        // For scalar expressions, set the alias directly
        expression->setAlias(var_name);
        with_expression_list->children.push_back(expression);
    }
    
    return true;
}

}
