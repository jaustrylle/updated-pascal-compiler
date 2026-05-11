// Amiran Fields, Serena Reese
// CS4301 - Stage 1

#include <stage1.h>

#include <iomanip>
#include <ctime>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <set>
#include <vector>
using namespace std;

int iCount = 0;
int bCount = 0;
static bool usesTrue = false;
static bool usesFalse = false;
static set<string> spilledTemps;
static bool isRelOpToken(const string &s)
{
    return s == "=" || s == "<>" || s == "<" ||
           s == ">" || s == "<=" || s == ">=";
}

static bool isAddLevelOpToken(const string &s)
{
    return s == "+" || s == "-" || s == "or";
}

static bool isMultLevelOpToken(const string &s)
{
    return s == "*" || s == "div" || s == "mod" || s == "and";
}

static bool shouldSpillTempForToken(const string &s)
{
    return false;
}

static void recordTempSpill(const string &temp)
{
    spilledTemps.insert(temp);
}

static string chooseReusableTemp(const string &operand1, const string &operand2)
{
    if (!operand1.empty() && operand1[0] == 'T')
        return operand1;
    if (!operand2.empty() && operand2[0] == 'T')
        return operand2;
    return "";
}

Compiler::Compiler(char **argv)
{
    sourceFile.open(argv[1]);
    listingFile.open(argv[2]);
    objectFile.open(argv[3]);

    if (!sourceFile) processError("Cannot open source file");
    if (!listingFile) processError("Cannot open listing file");
    if (!objectFile) processError("Cannot open object file");

    lineNo = 1;
    errorCount = 0;
    ch = ' ';
    currentTempNo = -1;
    maxTempNo = -1;
    contentsOfAReg = "";
    lastCharWasNewline = false;
    spilledTemps.clear();
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

    listingFile << "STAGE1:  Amiran Fields, Serena Reese       "
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
    listingFile << endl
                << "COMPILATION TERMINATED      "
                << errorCount
                << " ERROR";
    if (errorCount != 1)
        listingFile << "S";
    listingFile << " ENCOUNTERED" << endl;
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
    execStmts();

    if (token != "end")
        processError("keyword 'end' expected");

    token = nextToken();

    if (token != ".")
        processError("period expected");

    token = nextToken();
    code("end", ".");
}

void Compiler::execStmts()
{
    while (isNonKeyId(token) || token == "read" || token == "write")
        execStmt();
}

void Compiler::execStmt()
{
    if (isNonKeyId(token))
        assignStmt();
    else if (token == "read")
        readStmt();
    else if (token == "write")
        writeStmt();
    else
        processError("statement expected");
}

void Compiler::assignStmt()
{
    string lhs = token;

    if (symbolTable.find(lhs) == symbolTable.end())
        processError("reference to undefined symbol " + lhs);

    pushOperand(lhs);

    token = nextToken();
    if (token != ":=")
        processError("\":=\" expected");

    pushOperator(token);
    token = nextToken();

    express();

    if (whichType(lhs) != whichType(operandStk.top()))
        processError("incompatible types for operator ':='");

    if (token != ";")
    {
        if (token != ")" && !isAddLevelOpToken(token) &&
            !isMultLevelOpToken(token) && !isRelOpToken(token))
        {
            processError("one of \"*\", \"and\", \"div\", \"mod\", \")\", \"+\", \"-\", \";\", \"<\", \"<=\", \"<>\", \"=\", \">\", \">=\", or \"or\" expected");
        }
        processError("\";\" expected");
    }

    if (symbolTable.at(lhs).getMode() != VARIABLE)
        processError("symbol on left-hand side of assignment must have a storage mode of VARIABLE");

    string op = popOperator();
    string rhs = popOperand();
    string lhs2 = popOperand();
    code(op, rhs, lhs2);
    token = nextToken();
}



void Compiler::readStmt()
{
    string names;

    token = nextToken();
    if (token != "(")
        processError("\"(\" expected");

    token = nextToken();
    names = ids();

    if (token != ")")
        processError("\")\" expected");

    code("read", names);
    token = nextToken();

    if (token != ";")
        processError("\";\" expected");

    token = nextToken();
}

void Compiler::writeStmt()
{
    string names;

    token = nextToken();
    if (token != "(")
        processError("\"(\" expected");

    token = nextToken();
    names = ids();

    if (token != ")")
        processError("\")\" expected");

    code("write", names);
    token = nextToken();

    if (token != ";")
        processError("\";\" expected");

    token = nextToken();
}

void Compiler::express()
{
    term();
    expresses();
}

void Compiler::expresses()
{
    while (isRelOpToken(token))
    {
        pushOperator(token);
        token = nextToken();
        if (!operandStk.empty() && isTemporary(operandStk.top()) &&
            contentsOfAReg == operandStk.top() &&
            (token == "(" || token == "not" || token == "-"))
        {
            string temp = operandStk.top();
            recordTempSpill(temp);
            emit("", "mov", "[" + temp + "],eax", "; deassign AReg");
        }
        term();
        string op = popOperator();
        string right = popOperand();
        string left = popOperand();
        code(op, left, right);
    }
}

void Compiler::term()
{
    factor();
    terms();
}

void Compiler::terms()
{
    while (isAddLevelOpToken(token))
    {
        pushOperator(token);
        token = nextToken();
        if (!operandStk.empty() && isTemporary(operandStk.top()) &&
            contentsOfAReg == operandStk.top() &&
            (token == "(" || token == "not" || token == "-"))
        {
            string temp = operandStk.top();
            recordTempSpill(temp);
            emit("", "mov", "[" + temp + "],eax", "; deassign AReg");
        }
        factor();
        string op = popOperator();
        string right = popOperand();
        string left = popOperand();
        code(op, left, right);
    }
}

void Compiler::factor()
{
    part();
    factors();
}

void Compiler::factors()
{
    while (isMultLevelOpToken(token))
    {
        pushOperator(token);
        token = nextToken();
        if (!operandStk.empty() && isTemporary(operandStk.top()) &&
            contentsOfAReg == operandStk.top() &&
            (token == "(" || token == "not" || token == "-"))
        {
            string temp = operandStk.top();
            recordTempSpill(temp);
            emit("", "mov", "[" + temp + "],eax", "; deassign AReg");
        }
        part();
        string op = popOperator();
        string right = popOperand();
        string left = popOperand();
        code(op, left, right);
    }
}

void Compiler::part()
{
    if (token == "not")
    {
        token = nextToken();

        if (token == "(")
        {
            token = nextToken();
            express();
            if (token != ")")
                processError("\")\" expected");
            code("not", popOperand());
            token = nextToken();
        }
        else if (isBoolean(token))
        {
            pushOperand(token == "true" ? "false" : "true");
            token = nextToken();
        }
        else if (isNonKeyId(token))
        {
            if (whichType(token) != BOOLEAN)
                processError("boolean operand expected");
            code("not", token);
            token = nextToken();
        }
        else
            processError("boolean expected after 'not'");
    }
    else if (token == "+")
    {
        token = nextToken();

        if (token == "(")
        {
            token = nextToken();
            express();
            if (token != ")")
                processError("\")\" expected");
            token = nextToken();
        }
        else if (isInteger(token) || isNonKeyId(token))
        {
            if (isNonKeyId(token) && whichType(token) != INTEGER)
                processError("integer operand expected");
            if (isInteger(token) && symbolTable.find(token) == symbolTable.end())
                insert(token, INTEGER, CONSTANT, token, YES, 1);
            pushOperand(token);
            token = nextToken();
        }
        else
            processError("expected '(', integer, or non-keyword id; found " + token);
    }
    else if (token == "-")
    {
        token = nextToken();

        if (token == "(")
        {
            token = nextToken();
            express();
            if (token != ")")
                processError("\")\" expected");
            code("neg", popOperand());
            token = nextToken();
        }
        else if (isInteger(token))
        {
            if (symbolTable.find("-" + token) == symbolTable.end())
                insert("-" + token, INTEGER, CONSTANT, "-" + token, YES, 1);
            pushOperand("-" + token);
            token = nextToken();
        }
        else if (isNonKeyId(token))
        {
            if (whichType(token) != INTEGER)
                processError("integer operand expected");
            code("neg", token);
            token = nextToken();
        }
        else
            processError("integer expected after unary '-'");
    }
    else if (token == "(")
    {
        token = nextToken();
        express();
        if (token != ")")
            processError("\")\" expected");
        token = nextToken();
    }
    else if (isInteger(token) || isBoolean(token) || isNonKeyId(token))
    {
        if (isNonKeyId(token) && symbolTable.find(token) == symbolTable.end())
            processError("reference to undefined symbol " + token);
        if (isInteger(token) && symbolTable.find(token) == symbolTable.end())
            insert(token, INTEGER, CONSTANT, token, YES, 1);
        pushOperand(token);
        token = nextToken();
    }
    else
        processError("illegal expression");
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
        processError("\":\" expected");


    token = nextToken();

    if (token == "integer")
        type = "integer";
    else if (token == "boolean")
        type = "boolean";
    else
        processError("illegal type follows \":\"");


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
            s == "true" || s == "false" || s == "not" ||
            s == "div" || s == "mod" || s == "and" ||
            s == "or" || s == "read" || s == "write");
}

bool Compiler::isSpecialSymbol(char c) const
{
    string s = "=,:;.+-*()<>";
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
            processError("symbol " + one + " is multiply defined");


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
        processError("reference to undefined symbol " + name);

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
        processError("reference to undefined symbol " + name);

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

string Compiler::getTemp()
{
    currentTempNo++;
    if (currentTempNo > maxTempNo)
        maxTempNo = currentTempNo;
    string temp = "T" + to_string(currentTempNo);
    if (symbolTable.find(temp) == symbolTable.end())
    {
        insert(temp, UNKNOWN, VARIABLE, "", YES, 1);
        symbolTable.find(temp)->second.setInternalName(temp);
    }
    return temp;
}

void Compiler::pushOperator(string name)
{
    operatorStk.push(name);
}

void Compiler::pushOperand(string name)
{
    operandStk.push(name);
}

string Compiler::popOperator()
{
    string top = operatorStk.top();
    operatorStk.pop();
    return top;
}

string Compiler::popOperand()
{
    string top = operandStk.top();
    operandStk.pop();
    return top;
}

void Compiler::freeTemp()
{
    if (currentTempNo >= 0)
        currentTempNo--;
}

string Compiler::getLabel()
{
    static int labelNo = 0;
    return "L" + to_string(labelNo++);
}

bool Compiler::isTemporary(string s) const
{
    return !s.empty() && s[0] == 'T';
}

void Compiler::code(string op, string operand1, string operand2)
{
    if (op == "program")
        emitPrologue(operand1);
    else if (op == "end")
		emitEpilogue("", "");
    else if (op == ":=")
        emitAssignCode(operand1, operand2);
    else if (op == "read")
        emitReadCode(operand1);
    else if (op == "write")
        emitWriteCode(operand1);
    else if (op == "not" || op == "neg")
    {
        if (op == "not")
            emitNotCode(operand1);
        else
            emitNegationCode(operand1);
    }
    else if (op == "+")
        emitAdditionCode(operand1, operand2);
    else if (op == "-")
        emitSubtractionCode(operand1, operand2);
    else if (op == "*")
        emitMultiplicationCode(operand1, operand2);
    else if (op == "div")
        emitDivisionCode(operand1, operand2);
    else if (op == "mod")
        emitModuloCode(operand1, operand2);
    else if (op == "and")
        emitAndCode(operand1, operand2);
    else if (op == "or")
        emitOrCode(operand1, operand2);
    else if (op == "=")
        emitEqualityCode(operand1, operand2);
    else if (op == "<>")
        emitInequalityCode(operand1, operand2);
    else if (op == "<")
        emitLessThanCode(operand1, operand2);
    else if (op == "<=")
        emitLessThanOrEqualToCode(operand1, operand2);
    else if (op == ">")
        emitGreaterThanCode(operand1, operand2);
    else if (op == ">=")
        emitGreaterThanOrEqualToCode(operand1, operand2);
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
    objectFile << endl;
    emit("_start:", "", "", "");
}

void Compiler::emitEpilogue(string, string)
{
    emit("", "Exit", "{0}");
    objectFile << endl;
    emitStorage();
}

void Compiler::emitStorage()
{
    emit("SECTION", ".data");
    vector<string> constantNames;

    for (map<string, SymbolTableEntry>::iterator it = symbolTable.begin();
         it != symbolTable.end(); ++it)
    {
        if (it->second.getMode() == CONSTANT && it->second.getDataType() != PROG_NAME)
            constantNames.push_back(it->first);
    }

    if (usesFalse)
        constantNames.push_back("false");
    if (usesTrue)
        constantNames.push_back("true");

    sort(constantNames.begin(), constantNames.end());
    constantNames.erase(unique(constantNames.begin(), constantNames.end()),
                        constantNames.end());

    for (vector<string>::const_iterator it = constantNames.begin();
         it != constantNames.end(); ++it)
    {
        const string &name = *it;

        if (name == "false")
        {
            emit("FALSE", "dd", "0", "; false");
            continue;
        }

        if (name == "true")
        {
            emit("TRUE", "dd", "-1", "; true");
            continue;
        }

        SymbolTableEntry entry = symbolTable.at(name);
        string val = entry.getValue();
        if (val == "true")
            val = "-1";
        else if (val == "false")
            val = "0";

        emit(entry.getInternalName(), "dd", val, "; " + name.substr(0, 15));
    }

    objectFile << endl;
    emit("SECTION", ".bss");

    vector<string> tempNames(spilledTemps.begin(), spilledTemps.end());
    sort(tempNames.begin(), tempNames.end());

    for (vector<string>::const_iterator it = tempNames.begin();
         it != tempNames.end(); ++it)
        emit(*it, "resd", "1", "; " + *it);

    for (map<string, SymbolTableEntry>::iterator it = symbolTable.begin();
         it != symbolTable.end(); ++it)
    {
        if (it->second.getMode() == VARIABLE && !isTemporary(it->first))
            emit(it->second.getInternalName(), "resd", "1",
                 "; " + it->first.substr(0, 15));
    }
}

void Compiler::emitReadCode(string operand, string)
{
    stringstream ss(operand);
    string name;

    while (getline(ss, name, ','))
    {
        if (symbolTable.find(name) == symbolTable.end())
            processError("reference to undefined variable '" + name + "'");
        if (symbolTable.at(name).getMode() != VARIABLE)
            processError("reading in of read-only location '" + name + "'");
        if (whichType(name) != INTEGER)
            processError("can't read variables of this type");
        emit("", "call", "ReadInt", "; read int; value placed in eax");
        emit("", "mov", "[" + symbolTable.at(name).getInternalName() + "],eax",
             "; store eax at " + name);
        contentsOfAReg = name;
    }
}

void Compiler::emitWriteCode(string operand, string)
{
    stringstream ss(operand);
    string name;

    while (getline(ss, name, ','))
    {
        string location;
        if (name == "true")
        {
            usesTrue = true;
            location = "[TRUE]";
        }
        else if (name == "false")
        {
            usesFalse = true;
            location = "[FALSE]";
        }
        else if (symbolTable.find(name) == symbolTable.end())
            processError("reference to undefined variable '" + name + "'");
        else
            location = "[" + symbolTable.at(name).getInternalName() + "]";

        if (contentsOfAReg != name)
            emit("", "mov", "eax," + location, "; load " + name + " in eax");
        emit("", "call", "WriteInt", "; write int in eax to standard out");
        emit("", "call", "Crlf", "; write \\r\\n to standard out");
        contentsOfAReg = name;
    }
}

void Compiler::emitAssignCode(string operand1, string operand2)
{
    if (contentsOfAReg != operand1)
    {
        string rhs;
        if (operand1 == "true")
        {
            usesTrue = true;
            rhs = "[TRUE]";
        }
        else if (operand1 == "false")
        {
            usesFalse = true;
            rhs = "[FALSE]";
        }
        else if ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
                 && symbolTable.find(operand1) != symbolTable.end())
            rhs = "[" + symbolTable.at(operand1).getInternalName() + "]";
        else if (isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
            rhs = operand1;
        else
            rhs = "[" + symbolTable.at(operand1).getInternalName() + "]";

        emit("", "mov", "eax," + rhs, "; AReg = " + operand1);
    }
    emit("", "mov", "[" + symbolTable.at(operand2).getInternalName() + "],eax",
         "; " + operand2 + " = AReg");
    contentsOfAReg = operand2;
    if (isTemporary(operand1))
    {
        while (currentTempNo >= 0)
            freeTemp();
    }
}

void Compiler::emitAdditionCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("binary '+' requires integer operands");

    string temp = chooseReusableTemp(operand1, operand2);
    if (temp.empty())
        temp = getTemp();
    symbolTable.at(temp).setDataType(INTEGER);
    string rhs1 = ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
        && symbolTable.find(operand1) != symbolTable.end())
        ? "[" + symbolTable.at(operand1).getInternalName() + "]"
        : ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
           ? operand1 : "[" + symbolTable.at(operand1).getInternalName() + "]");
    string rhs2 = ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1))))
        && symbolTable.find(operand2) != symbolTable.end())
        ? "[" + symbolTable.at(operand2).getInternalName() + "]"
        : ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1))))
           ? operand2 : "[" + symbolTable.at(operand2).getInternalName() + "]");
    if (contentsOfAReg == operand1)
        emit("", "add", "eax," + rhs2, "; AReg = " + operand1 + " + " + operand2);
    else if (contentsOfAReg == operand2)
        emit("", "add", "eax," + rhs1, "; AReg = " + operand2 + " + " + operand1);
    else
    {
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
        emit("", "add", "eax," + rhs2, "; AReg = " + operand1 + " + " + operand2);
    }
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[T" + to_string(currentTempNo) + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitSubtractionCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("binary '-' requires integer operands");

    string temp = chooseReusableTemp(operand1, operand2);
    if (temp.empty())
        temp = getTemp();
    symbolTable.at(temp).setDataType(INTEGER);
    string rhs1 = ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
        && symbolTable.find(operand1) != symbolTable.end())
        ? "[" + symbolTable.at(operand1).getInternalName() + "]"
        : ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
           ? operand1 : "[" + symbolTable.at(operand1).getInternalName() + "]");
    string rhs2 = ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1))))
        && symbolTable.find(operand2) != symbolTable.end())
        ? "[" + symbolTable.at(operand2).getInternalName() + "]"
        : ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1))))
           ? operand2 : "[" + symbolTable.at(operand2).getInternalName() + "]");
    if (contentsOfAReg == operand2 && isTemporary(operand2))
    {
        recordTempSpill(operand2);
        emit("", "mov", "[" + operand2 + "],eax", "; deassign AReg");
    }
    if (contentsOfAReg != operand1)
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
    emit("", "sub", "eax," + rhs2, "; AReg = " + operand1 + " - " + operand2);
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[T" + to_string(currentTempNo) + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitMultiplicationCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("binary '*' requires integer operands");

    string temp = chooseReusableTemp(operand1, operand2);
    if (temp.empty())
        temp = getTemp();
    symbolTable.at(temp).setDataType(INTEGER);
    string rhs1 = ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
        && symbolTable.find(operand1) != symbolTable.end())
        ? "[" + symbolTable.at(operand1).getInternalName() + "]"
        : ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
           ? operand1 : "[" + symbolTable.at(operand1).getInternalName() + "]");
    string rhs2 = ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1))))
        && symbolTable.find(operand2) != symbolTable.end())
        ? "[" + symbolTable.at(operand2).getInternalName() + "]"
        : ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1))))
           ? operand2 : "[" + symbolTable.at(operand2).getInternalName() + "]");
    if (contentsOfAReg == operand1)
        emit("", "imul", "dword " + rhs2, "; AReg = " + operand1 + " * " + operand2);
    else if (contentsOfAReg == operand2)
        emit("", "imul", "dword " + rhs1, "; AReg = " + operand2 + " * " + operand1);
    else
    {
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
        emit("", "imul", "dword " + rhs2, "; AReg = " + operand1 + " * " + operand2);
    }
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[T" + to_string(currentTempNo) + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitDivisionCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("binary 'div' requires integer operands");

    string temp = chooseReusableTemp(operand1, operand2);
    if (temp.empty())
        temp = getTemp();
    symbolTable.at(temp).setDataType(INTEGER);
    string rhs1 = ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
        && symbolTable.find(operand1) != symbolTable.end())
        ? "[" + symbolTable.at(operand1).getInternalName() + "]"
        : ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
           ? operand1 : "[" + symbolTable.at(operand1).getInternalName() + "]");
    string rhs2 = ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1))))
        && symbolTable.find(operand2) != symbolTable.end())
        ? "[" + symbolTable.at(operand2).getInternalName() + "]"
        : ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1))))
           ? operand2 : "[" + symbolTable.at(operand2).getInternalName() + "]");
    if (contentsOfAReg == operand2 && isTemporary(operand2))
    {
        recordTempSpill(operand2);
        emit("", "mov", "[" + operand2 + "],eax", "; deassign AReg");
    }
    if (contentsOfAReg != operand1)
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
    emit("", "cdq", "", "; sign extend dividend from eax to edx:eax");
    emit("", "idiv", "dword " + rhs2, "; AReg = " + operand1 + " div " + operand2);
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[T" + to_string(currentTempNo) + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitModuloCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("binary 'mod' requires integer operands");

    string temp = chooseReusableTemp(operand1, operand2);
    if (temp.empty())
        temp = getTemp();
    symbolTable.at(temp).setDataType(INTEGER);
    string rhs1 = ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
        && symbolTable.find(operand1) != symbolTable.end())
        ? "[" + symbolTable.at(operand1).getInternalName() + "]"
        : ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
           ? operand1 : "[" + symbolTable.at(operand1).getInternalName() + "]");
    string rhs2 = ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1))))
        && symbolTable.find(operand2) != symbolTable.end())
        ? "[" + symbolTable.at(operand2).getInternalName() + "]"
        : ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1))))
           ? operand2 : "[" + symbolTable.at(operand2).getInternalName() + "]");
    if (contentsOfAReg == operand2 && isTemporary(operand2))
    {
        recordTempSpill(operand2);
        emit("", "mov", "[" + operand2 + "],eax", "; deassign AReg");
    }
    if (contentsOfAReg != operand1)
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
    emit("", "cdq", "", "; sign extend dividend from eax to edx:eax");
    emit("", "idiv", "dword " + rhs2, "; AReg = " + operand1 + " div " + operand2);
    emit("", "xchg", "eax,edx", "; exchange quotient and remainder");
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[T" + to_string(currentTempNo) + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitNegationCode(string operand1, string)
{
    if (whichType(operand1) != INTEGER)
        processError("unary '-' requires an integer operand");

    string temp = getTemp();
    symbolTable.at(temp).setDataType(INTEGER);
    string rhs = ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
        && symbolTable.find(operand1) != symbolTable.end())
        ? "[" + symbolTable.at(operand1).getInternalName() + "]"
        : ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1))))
           ? operand1 : "[" + symbolTable.at(operand1).getInternalName() + "]");
    if (contentsOfAReg != operand1)
        emit("", "mov", "eax," + rhs, "; AReg = " + operand1);
    emit("", "neg", "eax", "; AReg = -AReg");
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[T" + to_string(currentTempNo) + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitNotCode(string operand1, string)
{
    if (whichType(operand1) != BOOLEAN)
        processError("unary 'not' requires a boolean operand");

    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    string rhs;
    if (operand1 == "true")
    {
        usesTrue = true;
        rhs = "[TRUE]";
    }
    else if (operand1 == "false")
    {
        usesFalse = true;
        rhs = "[FALSE]";
    }
    else
        rhs = "[" + symbolTable.at(operand1).getInternalName() + "]";
    if (contentsOfAReg != operand1)
        emit("", "mov", "eax," + rhs, "; AReg = " + operand1);
    emit("", "not", "eax", "; AReg = !AReg");
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[T" + to_string(currentTempNo) + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitAndCode(string operand1, string operand2)
{
    if (whichType(operand1) != BOOLEAN || whichType(operand2) != BOOLEAN)
        processError("binary 'and' requires boolean operands");

    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    string rhs1;
    string rhs2;
    if (operand1 == "true") { usesTrue = true; rhs1 = "[TRUE]"; }
    else if (operand1 == "false") { usesFalse = true; rhs1 = "[FALSE]"; }
    else rhs1 = "[" + symbolTable.at(operand1).getInternalName() + "]";

    if (operand2 == "true") { usesTrue = true; rhs2 = "[TRUE]"; }
    else if (operand2 == "false") { usesFalse = true; rhs2 = "[FALSE]"; }
    else rhs2 = "[" + symbolTable.at(operand2).getInternalName() + "]";

    if (contentsOfAReg == operand1)
        emit("", "and", "eax," + rhs2, "; AReg = " + operand1 + " and " + operand2);
    else if (contentsOfAReg == operand2)
        emit("", "and", "eax," + rhs1, "; AReg = " + operand2 + " and " + operand1);
    else
    {
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
        emit("", "and", "eax," + rhs2, "; AReg = " + operand1 + " and " + operand2);
    }
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[" + temp + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitOrCode(string operand1, string operand2)
{
    if (whichType(operand1) != BOOLEAN || whichType(operand2) != BOOLEAN)
        processError("binary 'or' requires boolean operands");

    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    string rhs1;
    string rhs2;
    if (operand1 == "true") { usesTrue = true; rhs1 = "[TRUE]"; }
    else if (operand1 == "false") { usesFalse = true; rhs1 = "[FALSE]"; }
    else rhs1 = "[" + symbolTable.at(operand1).getInternalName() + "]";

    if (operand2 == "true") { usesTrue = true; rhs2 = "[TRUE]"; }
    else if (operand2 == "false") { usesFalse = true; rhs2 = "[FALSE]"; }
    else rhs2 = "[" + symbolTable.at(operand2).getInternalName() + "]";

    if (contentsOfAReg == operand1)
        emit("", "or", "eax," + rhs2, "; AReg = " + operand1 + " or " + operand2);
    else if (contentsOfAReg == operand2)
        emit("", "or", "eax," + rhs1, "; AReg = " + operand2 + " or " + operand1);
    else
    {
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
        emit("", "or", "eax," + rhs2, "; AReg = " + operand1 + " or " + operand2);
    }
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[" + temp + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitEqualityCode(string operand1, string operand2)
{
    if (whichType(operand1) != whichType(operand2))
        processError("binary '=' requires operands of the same type");

    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    string rhs1;
    string rhs2;
    if (operand1 == "true") { usesTrue = true; rhs1 = "[TRUE]"; }
    else if (operand1 == "false") { usesFalse = true; rhs1 = "[FALSE]"; }
    else if ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1)))) &&
             symbolTable.find(operand1) != symbolTable.end()) rhs1 = "[" + symbolTable.at(operand1).getInternalName() + "]";
    else if (isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1)))) rhs1 = operand1;
    else rhs1 = "[" + symbolTable.at(operand1).getInternalName() + "]";

    if (operand2 == "true") { usesTrue = true; rhs2 = "[TRUE]"; }
    else if (operand2 == "false") { usesFalse = true; rhs2 = "[FALSE]"; }
    else if ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1)))) &&
             symbolTable.find(operand2) != symbolTable.end()) rhs2 = "[" + symbolTable.at(operand2).getInternalName() + "]";
    else if (isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1)))) rhs2 = operand2;
    else rhs2 = "[" + symbolTable.at(operand2).getInternalName() + "]";

    string trueLabel = getLabel();
    string endLabel = getLabel();
    if (contentsOfAReg != operand1)
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
    emit("", "cmp", "eax," + rhs2, "; compare " + operand1 + " and " + operand2);
    emit("", "je", "." + trueLabel, "; if " + operand1 + " = " + operand2 + " then jump to set eax to TRUE");
    usesFalse = true;
    emit("", "mov", "eax,[FALSE]", "; else set eax to FALSE");
    emit("", "jmp", "." + endLabel, "; unconditionally jump");
    emit("." + trueLabel + ":", "", "", "");
    usesTrue = true;
    emit("", "mov", "eax,[TRUE]", "; set eax to TRUE");
    emit("." + endLabel + ":", "", "", "");
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[" + temp + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitInequalityCode(string operand1, string operand2)
{
    if (whichType(operand1) != whichType(operand2))
        processError("binary '<>' requires operands of the same type");

    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    string rhs1;
    string rhs2;
    if (operand1 == "true") { usesTrue = true; rhs1 = "[TRUE]"; }
    else if (operand1 == "false") { usesFalse = true; rhs1 = "[FALSE]"; }
    else if ((isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1)))) &&
             symbolTable.find(operand1) != symbolTable.end()) rhs1 = "[" + symbolTable.at(operand1).getInternalName() + "]";
    else if (isInteger(operand1) || (!operand1.empty() && operand1[0] == '-' && isInteger(operand1.substr(1)))) rhs1 = operand1;
    else rhs1 = "[" + symbolTable.at(operand1).getInternalName() + "]";

    if (operand2 == "true") { usesTrue = true; rhs2 = "[TRUE]"; }
    else if (operand2 == "false") { usesFalse = true; rhs2 = "[FALSE]"; }
    else if ((isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1)))) &&
             symbolTable.find(operand2) != symbolTable.end()) rhs2 = "[" + symbolTable.at(operand2).getInternalName() + "]";
    else if (isInteger(operand2) || (!operand2.empty() && operand2[0] == '-' && isInteger(operand2.substr(1)))) rhs2 = operand2;
    else rhs2 = "[" + symbolTable.at(operand2).getInternalName() + "]";

    string trueLabel = getLabel();
    string endLabel = getLabel();
    if (contentsOfAReg != operand1)
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
    emit("", "cmp", "eax," + rhs2, "; compare " + operand1 + " and " + operand2);
    emit("", "jne", "." + trueLabel, "; if " + operand1 + " <> " + operand2 + " then jump to set eax to TRUE");
    usesFalse = true;
    emit("", "mov", "eax,[FALSE]", "; else set eax to FALSE");
    emit("", "jmp", "." + endLabel, "; unconditionally jump");
    emit("." + trueLabel + ":", "", "", "");
    usesTrue = true;
    emit("", "mov", "eax,[TRUE]", "; set eax to TRUE");
    emit("." + endLabel + ":", "", "", "");
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[" + temp + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitLessThanCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("binary '<' requires integer operands");

    string temp = chooseReusableTemp(operand1, operand2);
    if (temp.empty())
        temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    string rhs1 = symbolTable.find(operand1) != symbolTable.end()
        ? "[" + symbolTable.at(operand1).getInternalName() + "]" : operand1;
    string rhs2 = symbolTable.find(operand2) != symbolTable.end()
        ? "[" + symbolTable.at(operand2).getInternalName() + "]" : operand2;
    string trueLabel = getLabel();
    string endLabel = getLabel();
    if (contentsOfAReg == operand2 && isTemporary(operand2))
    {
        recordTempSpill(operand2);
        emit("", "mov", "[" + operand2 + "],eax", "; deassign AReg");
    }
    if (contentsOfAReg != operand1)
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
    emit("", "cmp", "eax," + rhs2, "; compare " + operand1 + " and " + operand2);
    emit("", "jl", "." + trueLabel, "; if " + operand1 + " < " + operand2 + " then jump to set eax to TRUE");
    usesFalse = true;
    emit("", "mov", "eax,[FALSE]", "; else set eax to FALSE");
    emit("", "jmp", "." + endLabel, "; unconditionally jump");
    emit("." + trueLabel + ":", "", "", "");
    usesTrue = true;
    emit("", "mov", "eax,[TRUE]", "; set eax to TRUE");
    emit("." + endLabel + ":", "", "", "");
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[" + temp + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitLessThanOrEqualToCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("binary '<=' requires integer operands");

    string temp = chooseReusableTemp(operand1, operand2);
    if (temp.empty())
        temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    string rhs1 = symbolTable.find(operand1) != symbolTable.end()
        ? "[" + symbolTable.at(operand1).getInternalName() + "]" : operand1;
    string rhs2 = symbolTable.find(operand2) != symbolTable.end()
        ? "[" + symbolTable.at(operand2).getInternalName() + "]" : operand2;
    string trueLabel = getLabel();
    string endLabel = getLabel();
    if (contentsOfAReg == operand2 && isTemporary(operand2))
    {
        recordTempSpill(operand2);
        emit("", "mov", "[" + operand2 + "],eax", "; deassign AReg");
    }
    if (contentsOfAReg != operand1)
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
    emit("", "cmp", "eax," + rhs2, "; compare " + operand1 + " and " + operand2);
    emit("", "jle", "." + trueLabel, "; if " + operand1 + " <= " + operand2 + " then jump to set eax to TRUE");
    usesFalse = true;
    emit("", "mov", "eax,[FALSE]", "; else set eax to FALSE");
    emit("", "jmp", "." + endLabel, "; unconditionally jump");
    emit("." + trueLabel + ":", "", "", "");
    usesTrue = true;
    emit("", "mov", "eax,[TRUE]", "; set eax to TRUE");
    emit("." + endLabel + ":", "", "", "");
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[" + temp + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitGreaterThanCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("binary '>' requires integer operands");

    string temp = chooseReusableTemp(operand1, operand2);
    if (temp.empty())
        temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    string rhs1 = symbolTable.find(operand1) != symbolTable.end()
        ? "[" + symbolTable.at(operand1).getInternalName() + "]" : operand1;
    string rhs2 = symbolTable.find(operand2) != symbolTable.end()
        ? "[" + symbolTable.at(operand2).getInternalName() + "]" : operand2;
    string trueLabel = getLabel();
    string endLabel = getLabel();
    if (contentsOfAReg == operand2 && isTemporary(operand2))
    {
        recordTempSpill(operand2);
        emit("", "mov", "[" + operand2 + "],eax", "; deassign AReg");
    }
    if (contentsOfAReg != operand1)
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
    emit("", "cmp", "eax," + rhs2, "; compare " + operand1 + " and " + operand2);
    emit("", "jg", "." + trueLabel, "; if " + operand1 + " > " + operand2 + " then jump to set eax to TRUE");
    usesFalse = true;
    emit("", "mov", "eax,[FALSE]", "; else set eax to FALSE");
    emit("", "jmp", "." + endLabel, "; unconditionally jump");
    emit("." + trueLabel + ":", "", "", "");
    usesTrue = true;
    emit("", "mov", "eax,[TRUE]", "; set eax to TRUE");
    emit("." + endLabel + ":", "", "", "");
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[" + temp + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitGreaterThanOrEqualToCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("binary '>=' requires integer operands");

    string temp = chooseReusableTemp(operand1, operand2);
    if (temp.empty())
        temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    string rhs1 = symbolTable.find(operand1) != symbolTable.end()
        ? "[" + symbolTable.at(operand1).getInternalName() + "]" : operand1;
    string rhs2 = symbolTable.find(operand2) != symbolTable.end()
        ? "[" + symbolTable.at(operand2).getInternalName() + "]" : operand2;
    string trueLabel = getLabel();
    string endLabel = getLabel();
    if (contentsOfAReg == operand2 && isTemporary(operand2))
    {
        recordTempSpill(operand2);
        emit("", "mov", "[" + operand2 + "],eax", "; deassign AReg");
    }
    if (contentsOfAReg != operand1)
        emit("", "mov", "eax," + rhs1, "; AReg = " + operand1);
    emit("", "cmp", "eax," + rhs2, "; compare " + operand1 + " and " + operand2);
    emit("", "jge", "." + trueLabel, "; if " + operand1 + " >= " + operand2 + " then jump to set eax to TRUE");
    usesFalse = true;
    emit("", "mov", "eax,[FALSE]", "; else set eax to FALSE");
    emit("", "jmp", "." + endLabel, "; unconditionally jump");
    emit("." + trueLabel + ":", "", "", "");
    usesTrue = true;
    emit("", "mov", "eax,[TRUE]", "; set eax to TRUE");
    emit("." + endLabel + ":", "", "", "");
    if (shouldSpillTempForToken(token))
    {
        recordTempSpill(temp);
        emit("", "mov", "[" + temp + "],eax", "; deassign AReg");
    }
    contentsOfAReg = temp;
    pushOperand(temp);
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
    lastCharWasNewline = (ch == '\n');

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
            if (ch == END_OF_FILE)
                processError("unexpected end of file");
            nextChar();
        }
        else if (isSpecialSymbol(ch))
        {
            if (ch == ':')
            {
                token += ch;
                nextChar();
                if (ch == '=')
                {
                    token += ch;
                    nextChar();
                }
            }
            else if (ch == '<')
            {
                token += ch;
                nextChar();
                if (ch == '=' || ch == '>')
                {
                    token += ch;
                    nextChar();
                }
            }
            else if (ch == '>')
            {
                token += ch;
                nextChar();
                if (ch == '=')
                {
                    token += ch;
                    nextChar();
                }
            }
            else
            {
                token = ch;
                nextChar();
            }
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
    if (!lastCharWasNewline && ch != END_OF_FILE)
        listingFile << endl;
    listingFile << "Error: Line " << lineNo << ": " << err << endl;
    errorCount++;
    createListingTrailer();
    exit(EXIT_FAILURE);
}
