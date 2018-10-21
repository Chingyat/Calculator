#pragma once
#include "ast.hpp"

#include <sstream>

namespace lince {
struct ASTFormatter {
    int NIndent = 0;
    static std::ostream &Indent(std::ostream &OS)
    {
        return OS << "  ";
    }

    static std::ostream &Indent(std::ostream &OS, int N)
    {
        while (N--)
            OS << Indent;
        return OS;
    }

    std::ostream &writeChar(std::ostream &OS, char C)
    {
        OS << C;
        if (C == '\n')
            Indent(OS, NIndent);
        return OS;
    }

    std::string operator()(std::string_view ASTS)
    {
        NIndent = 0;
        std::ostringstream OSS;
        for (char X : ASTS) {
            if (X == '{' || X == '[') {
                NIndent++;
                writeChar(OSS, X);
                writeChar(OSS, '\n');
            } else if (X == '}' || X == ']') {
                NIndent--;
                writeChar(OSS, '\n');
                writeChar(OSS, X);
            } else if (X == ',') {
                writeChar(OSS, X);
                writeChar(OSS, '\n');
            } else {
                writeChar(OSS, X);
            }
        }
        return OSS.str();
    }
};

inline std::string format(AST *A)
{
    ASTFormatter AF;
    return AF(A->dump());
}

}
