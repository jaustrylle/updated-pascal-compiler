// Amiran Fields, Serena Reese
// CS4301 - Stage 0

#include "stage0.h"

#include <iomanip>
#include <ctime>
#include <cctype>
#include <vector>



using namespace std;

int iCount = 0;
int bCount = 0;

Compiler::Compiler(char **argv)
{
    sourceFile.open(argv[1]);
    listingFile.open(argv[2]);
    objectFile.open(argv[3]);

    if (!sourceFile) processError("Cannot open source file");
    if (!listingFile) processError("Cannot open listing file");
    if (!objectFile) processError("Cannot open object file");

    lineNo = 1;
    ch = ' ';
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

    listingFile << "STAGE0:  Amiran Fields, Serena Reese       "
                << ctime(&now) << endl;

    listingFile << "LINE NO.              SOURCE STATEMENT" << endl << endl;
}

void Compiler::parser()
{
    nextChar();
    if (nextToken() != "program")
        processError("keyword 'program' expected");

    prog();
}

void Compiler::createListingTrailer()
{
    listingFile << "\nCOMPILATION TERMINATED      "
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

    token = nextToken();
    x = token;

    if (!isNonKeyId(x))
        processError("program name expected");

    token = nextToken();
    if (token != ";")
        processError("semicolon expected");

    code("program", x);
    insert(x, PROG_NAME, CONSTANT, x, NO, 0);

    token = nextToken();
}

void Compiler::consts()
{
    token = nextToken();

    if (!isNonKeyId(token))
        processError("non-keyword identifier expected");

    constStmts();
}

void Compiler::vars()
{
    token = nextToken();
    varStmts();
}

void Compiler::beginEndStmt()
{
    token = nextToken();

    if (token != "end")
        processError("keyword 'end' expected");

    token = nextToken();

    if (token != ".")
        processError("period expected");

    token = nextToken();

    code("end", ".");
}

//////////////////// CONST ////////////////////

void Compiler::constStmts()
{
    string x, y;

    x = token;

    token = nextToken();
    if (token != "=")
        processError("'=' expected");

    token = nextToken();

    if (token == "+")
    {
        token = nextToken();
        if (!isInteger(token))
            processError("integer expected after '+'");
        y = token;
        token = nextToken();
    }
    else if (token == "-")
    {
        token = nextToken();
        if (!isInteger(token))
            processError("integer expected after '-'");
        y = "-" + token;
        token = nextToken();
    }
    else if (token == "not")
    {
        token = nextToken();
        if (!isBoolean(token))
            processError("boolean expected after 'not'");
        y = (token == "true") ? "false" : "true";
        token = nextToken();
    }
    else if (isLiteral(token))
    {
        y = token;
        token = nextToken();
    }
    else if (isNonKeyId(token))
    {
        y = whichValue(token);
        token = nextToken();
    }
    else
        processError("illegal token");

    if (token != ";")
        processError("';' expected");

    insert(x, whichType(y), CONSTANT, y, YES, 1);

    token = nextToken();

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

    token = nextToken();

    if (token == "integer")
        type = "integer";
    else if (token == "boolean")
        type = "boolean";
    else
        processError("type expected");

    token = nextToken();

    if (token != ";")
        processError("';' expected");

    insert(x, (type == "integer" ? INTEGER : BOOLEAN),
           VARIABLE, "", YES, 1);

    token = nextToken();

    if (isNonKeyId(token))
        varStmts();
}

string Compiler::ids()
{
    string id = token;

    if (!isNonKeyId(id))
        processError("identifier expected");

    token = nextToken();

    if (token == ",")
    {
        token = nextToken();
        return id + "," + ids();
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
    if (s.empty()) return false;

    for (char c : s)
        if (!isdigit(c)) return false;

    return true;
}

bool Compiler::isBoolean(string s) const
{
    return s == "true" || s == "false";
}

bool Compiler::isLiteral(string s) const
{
    return isInteger(s) || isBoolean(s);
}

//////////////////// ACTION ////////////////////

void Compiler::insert(string name, storeTypes type, modes mode,
                      string value, allocation alloc, int units)
{
    size_t start = 0;

    while (start < name.size())
    {
        size_t pos = name.find(',', start);

        string one;

        if (pos == string::npos)
            one = name.substr(start);
        else
            one = name.substr(start, pos - start);

        if (one.empty())
            break;

        if (symbolTable.count(one) > 0)
            processError("multiple definition");

        string internalName = genInternalName(type);

        symbolTable.insert({
            one,
            SymbolTableEntry(internalName, type, mode, value, alloc, units)
        });
		
		

        start = (pos == string::npos) ? name.size() : pos + 1;
    }
}

storeTypes Compiler::whichType(string name)
{
    if (isBoolean(name)) return BOOLEAN;

   
    if (!name.empty() && name[0] == '-' && isInteger(name.substr(1)))
        return INTEGER;

    if (isInteger(name)) return INTEGER;

    auto it = symbolTable.find(name);
    if (it == symbolTable.end())
        processError("undefined identifier");

    return it->second.getDataType();
}

string Compiler::whichValue(string name)
{
   
    if (isInteger(name) || isBoolean(name))
        return name;

    if (!name.empty() && name[0] == '-' && isInteger(name.substr(1)))
        return name;

    auto it = symbolTable.find(name);
    if (it == symbolTable.end())
        processError("undefined identifier");

    return it->second.getValue();
}

string Compiler::genInternalName(storeTypes stype) const
{
    if (stype == INTEGER)
        return "I" + to_string(iCount++);
    else if (stype == BOOLEAN)
        return "B" + to_string(bCount++);
    else
        return "P0";
}

void Compiler::code(string op, string operand1, string operand2)
{
    if (op == "program")
        emitPrologue(operand1);
    else if (op == "end")
		emitEpilogue("", "");
    else
        processError("compiler error");
}

//////////////////// EMIT ////////////////////

void Compiler::emit(string label, string instruction, string operands, string comment)
{
    objectFile << left << setw(8) << label
               << setw(8) << instruction
               << setw(24) << operands
               << comment << endl;
}

void Compiler::emitPrologue(string progName, string)
{
    time_t now = time(0);

    objectFile << "; Amiran Fields, Serena Reese "
               << ctime(&now);

    objectFile << "%INCLUDE \"Along32.inc\"" << endl;
    objectFile << "%INCLUDE \"Macros_Along.inc\"" << endl;
    objectFile << endl;

    emit("SECTION", ".text");
    objectFile << left << setw(8) << "global"
               << setw(8) << "_start"
               << setw(24) << ""
               << "; program " << progName.substr(0, 15) << endl;
    emit("_start:", "", "", "");
}





void Compiler::emitEpilogue(string, string)
{
    emit("", "Exit", "{0}");
    emitStorage();
}





void Compiler::emitStorage()
{
    emit("SECTION", ".data");

    for (map<string, SymbolTableEntry>::iterator it = symbolTable.begin();
         it != symbolTable.end(); ++it)
    {
        string name = it->first;
        SymbolTableEntry entry = it->second;

        if (entry.getMode() == CONSTANT && entry.getDataType() != PROG_NAME)
        {
            string val = entry.getValue();

            if (val == "true")
                val = "-1";
            else if (val == "false")
                val = "0";

            emit(entry.getInternalName(), "dd", val, "; " + name.substr(0, 15));
        }
    }

    emit("SECTION", ".bss");

    for (map<string, SymbolTableEntry>::iterator it = symbolTable.begin();
         it != symbolTable.end(); ++it)
    {
        string name = it->first;
        SymbolTableEntry entry = it->second;

        if (entry.getMode() == VARIABLE)
            emit(entry.getInternalName(), "resd", "1", "; " + name.substr(0, 15));
    }
}





//////////////////// LEXER ////////////////////

char Compiler::nextChar()
{
    if (!sourceFile.get(ch))
    {
        ch = END_OF_FILE;
        return ch;
    }

    if (listingFile && sourceFile.tellg() == 1)
        listingFile << setw(5) << right << lineNo << "|";

    listingFile << ch;

    if (ch == '\n')
{
    lineNo++;

    if (sourceFile.peek() != EOF)
        listingFile << setw(5) << right << lineNo << "|";
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
    listingFile << "Error: Line " << lineNo << ": " << err << endl;
    errorCount++;
    createListingTrailer();
    exit(EXIT_FAILURE);
}
