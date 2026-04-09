// Amiran Fields, Serena Reese
// CS4301 - Stage 0

#include <stage0.h>


#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <iomanip>
#include <ctime>
#include <cctype>

using namespace std;

Compiler::Compiler(char **argv)
{
    sourceFile.open(argv[1]);
    listingFile.open(argv[2]);
    objectFile.open(argv[3]);

    if (!sourceFile) processError("Cannot open source file");
    if (!listingFile) processError("Cannot open listing file");
    if (!objectFile) processError("Cannot open object file");

    lineNo = 1;
    nextChar();
}

Compiler::~Compiler()
{
    sourceFile.close();
    listingFile.close();
    objectFile.close();
}

//////////////////// OUTPUT ////////////////////

void Compiler::createListingHeader()
{
    time_t now = time(0);

    listingFile << "STAGE0: Amiran Fields, Serena Reese "
                << ctime(&now);

    listingFile << "LINE NO. SOURCE STATEMENT\n";

    listingFile << setw(5) << lineNo << "|";
}

void Compiler::parser()
{
    if (nextToken() != "program")
        processError("keyword 'program' expected");

    prog();
}

void Compiler::createListingTrailer()
{
    listingFile << "\nCOMPILATION TERMINATED "
                << errorCount
                << " ERRORS ENCOUNTERED\n";
}

//////////////////// GRAMMAR ////////////////////

void Compiler::prog()
{
    progStmt();

    if (token == "const")
        consts();

    if (token == "var")
        vars();

    if (token != "begin")
        processError("keyword 'begin' expected");

    beginEndStmt();

    if (token != "EOF")
        processError("no text may follow 'end'");
}

void Compiler::progStmt()
{
    string x;

    nextToken(); // program name
    x = token;

    if (!isNonKeyId(x))
        processError("program name expected");

    nextToken();
    if (token != ";")
        processError("semicolon expected");

    nextToken();

    code("program", x);
    insert(x, PROG_NAME, CONSTANT, x, NO, 0);
}

void Compiler::consts()
{
    nextToken();

    if (!isNonKeyId(token))
        processError("identifier expected after const");

    constStmts();
}

void Compiler::vars()
{
    nextToken();

    if (!isNonKeyId(token))
        processError("identifier expected after var");

    varStmts();
}

void Compiler::beginEndStmt()
{
    nextToken();

    if (token != "end")
        processError("keyword 'end' expected");

    nextToken();

    if (token != ".")
        processError("period expected");

    nextToken();

    code("end", ".");
}

//////////////////// CONST ////////////////////

void Compiler::constStmts()
{
    string x, y;

    x = token;

    nextToken();
    if (token != "=")
        processError("'=' expected");

    nextToken();

    if (token == "not")
    {
        nextToken();
        y = token;
        y = "not " + y;
    }
    else
    {
        y = token;
    }

    nextToken();
    if (token != ";")
        processError("';' expected");

    insert(x, whichType(y), CONSTANT, whichValue(y), YES, 1);

    nextToken();

    if (isNonKeyId(token))
        constStmts();
}

//////////////////// VAR ////////////////////

void Compiler::varStmts()
{
    string x, type;

    x = ids();

    if (token != ":")
        processError("':' expected");

    nextToken();

    if (token == "integer")
        type = "integer";
    else if (token == "boolean")
        type = "boolean";
    else
        processError("type expected");

    nextToken();

    if (token != ";")
        processError("';' expected");

    // insert each id (simplified: assumes single for now)
    insert(x, (type == "integer" ? INTEGER : BOOLEAN),
           VARIABLE, "", YES, 1);

    nextToken();

    if (isNonKeyId(token))
        varStmts();
}

string Compiler::ids()
{
    string id = token;

    if (!isNonKeyId(id))
        processError("identifier expected");

    nextToken();

    if (token == ",")
    {
        nextToken();
        ids();
    }

    return id;
}

//////////////////// HELPER ////////////////////

bool Compiler::isKeyword(string s) const
{
    return (s == "program" || s == "const" || s == "var" ||
            s == "begin" || s == "end" ||
            s == "integer" || s == "boolean" ||
            s == "true" || s == "false" || s == "not");
}

bool Compiler::isSpecialSymbol(char c) const
{
    string s = "=,:;.+-";
    return s.find(c) != string::npos;
}

bool Compiler::isNonKeyId(string s) const
{
    if (s.empty() || !islower(s[0])) return false;

    for (char c : s)
        if (!isalnum(c) && c != '_') return false;

    return !isKeyword(s);
}

bool Compiler::isInteger(string s) const
{
    for (char c : s)
        if (!isdigit(c)) return false;
    return true;
}

bool Compiler::isBoolean(string s) const
{
    return (s == "true" || s == "false");
}

bool Compiler::isLiteral(string s) const
{
    return isInteger(s) || isBoolean(s);
}

//////////////////// ACTION ////////////////////

void Compiler::insert(string name, storeTypes type, modes mode,
                      string value, allocation alloc, int units)
{
    if (symbolTable.count(name) > 0)
        processError("multiple definition");

    symbolTable.insert({
        name,
        SymbolTableEntry(name, type, mode, value, alloc, units)
    });
}

storeTypes Compiler::whichType(string name)
{
    if (isInteger(name)) return INTEGER;
    if (isBoolean(name)) return BOOLEAN;

    auto it = symbolTable.find(name);

    if (it == symbolTable.end())
        processError("undefined identifier");

    return it->second.getDataType();
}

string Compiler::whichValue(string name)
{
    if (isInteger(name) || isBoolean(name))
        return name;

    auto it = symbolTable.find(name);

    if (it == symbolTable.end())
        processError("undefined identifier");

    return it->second.getValue();
}

void Compiler::code(string op, string operand1, string operand2)
{
    // minimal stage0
}

//////////////////// LEXICON ////////////////////

char Compiler::nextChar()
{
    if (!sourceFile.get(ch))
        ch = END_OF_FILE;
    else
    {
        listingFile << ch;

        if (ch == '\n')
        {
            lineNo++;
            listingFile << setw(5) << lineNo << "|";
        }
    }
    return ch;
}

string Compiler::nextToken()
{
    token = "";

    while (token == "")
    {
        if (isspace(ch))
            nextChar();

        else if (ch == '{')
        {
            do { nextChar(); }
            while (ch != '}' && ch != END_OF_FILE);

            if (ch == END_OF_FILE)
                processError("unterminated comment");

            nextChar();
        }
        else if (isSpecialSymbol(ch))
        {
            token = ch;
            nextChar();
        }
        else if (islower(ch))
        {
            while (isalnum(ch) || ch == '_')
            {
                token += ch;
                nextChar();
            }
        }
        else if (isdigit(ch))
        {
            while (isdigit(ch))
            {
                token += ch;
                nextChar();
            }
        }
        else if (ch == END_OF_FILE)
            token = "EOF";
        else
            processError("illegal symbol");
    }

    return token;
}

//////////////////// ERROR ////////////////////

void Compiler::processError(string err)
{
    listingFile << "\nError: " << err << endl;
    errorCount++;
    exit(EXIT_FAILURE);
}
