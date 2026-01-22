#pragma once

#include <Parsers/IParserBase.h>
#include <Parsers/Kusto/ParserKQLQuery.h>

namespace DB
{

class ParserKQLLet : public ParserKQLBase
{
protected:
    const char * getName() const override { return "KQL let"; }
    bool parseImpl(Pos & pos, ASTPtr & node, Expected & expected) override;
};

}
