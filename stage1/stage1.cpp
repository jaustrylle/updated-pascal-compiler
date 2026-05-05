// Amiran Fields, Serena Reese
// CS4301 - Stage 1

#include <stage1.h>

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include <stdio.h>		// to close files
#include <cctype>		// nextChar, nextToken
#include <iomanip>		// format in/out streams using setw
#include <ctime>		// time in listing header
#include <vector>		// for ids() lists
#include <sstream>		// for varStmts(), parsing "many, some"

#include <stack>		// pushOperand, pushOperator...

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

//////////////////// EXPRESSIONS ////////////////////

void Compiler::execStmts() // stage 1, production 2
{
	// EXEC_STMT EXEC_STMTS | epsilon
	// NON_KEY_ID, 'read', 'write', 'end'
	
    while (token == "read" || token == "write" || isNonKeyId(token))
        execStmt();
}

void Compiler::execStmt() // stage 1, production 3
{
	// ASSIGN_STMT | READ_STMT | WRITE_STMT
	// NON_KEY_ID, 'read', 'write'

    if (isNonKeyId(token))
        assignStmt();
    else if (token == "read")
        readStmt();
    else if (token == "write")
        writeStmt();
    else
        processError("statement expected");
}

void Compiler::assignStmt() // stage 1, production 4
{
	// NON_KEY_ID ':=' EXPRESS ';'
	// NON_KEY_ID

    string lhs = token;

    pushOperand(lhs);

    token = nextToken();

    if (token != ":=")
        processError("':=' expected");

    pushOperator(":=");

    token = nextToken();

    express();

    if (token != ";")
        processError("semicolon expected");

    code(popOperator(), popOperand(), popOperand());

    token = nextToken();
}

void Compiler::readStmt() // stage 1, production 5
{
	// 'read' READ_LIST ';'
	// 'read'

    token = nextToken();

    if (token != "(")
        processError("'(' expected");

    token = nextToken();

    string x = ids();   // assumes this returns list of ids

    if (token != ")")
        processError("')' expected");

    token = nextToken();

    code("read", x);

    if (token != ";")
        processError("semicolon expected");

    token = nextToken();
}

void Compiler::writeStmt() // stage 1, production 7
{
	// 'write' WRITE_LIST ';'
	// 'write'

    token = nextToken();

    if (token != "(")
        processError("'(' expected");

    token = nextToken();

    string x = ids();   // assumes ids() handles the identifier list

    if (token != ")")
        processError("')' expected");

    token = nextToken();

    code("write", x);

    if (token != ";")
        processError("semicolon expected");

    token = nextToken();
}

void Compiler::express() // stage 1, production 9
{
	// TERM EXPRESSES
	// 'not', 'true', 'false', '(', '+', '-', INTEGER, NON_KEY_ID

    term();
    expresses();
}

void Compiler::expresses() // stage 1, production 10
{
	// REL_OP TERM EXPRESSES
	// '<>', '=', '<=', '>=', '<', '>', '), ',', ';'

    if (token == "=" || token == "<>" || token == "<" ||
        token == "<=" || token == ">" || token == ">=")
    {
        string op = token;

        pushOperator(op);

        token = nextToken();

        term();

        code(popOperator(), popOperand(), popOperand());

        expresses();
    }
}

void Compiler::term() // stage 1, production 11
{
	// FACTOR TERMS
	// 'not', 'true', 'false', '(', '+', '-', INTEGER, NON_KEY_ID

    factor();
    terms();
}

void Compiler::terms() // stage 1, production 12
{
	// ADD_LEVEL_OP FACTOR TERMS | epsilon
	// '-', '+', 'or', '<>', '=', '<=', '>=', '<', '>', ',', ')', ';'

    if (token == "+" || token == "-" || token == "or")
    {
        string op = token;

        pushOperator(op);

        token = nextToken();

        factor();

        code(popOperator(), popOperand(), popOperand());

        terms();
    }
}

void Compiler::factor() // stage 1, production 13
{
	// PART FACTORS
	// 'not', 'true', 'false', '(', '+', '-', INTEGER, NON_KEY_ID

    part();
    factors();
}

void Compiler::factors() // stage 1, production 14
{
	// MULT_LEV_OP PART FACTORS | epsilon
	// '*', 'div', 'mod', 'and', '<>', '=', '<=', '>=', '<', '>', ')', ',', ';', '-', '+', 'or'

    if (token == "*" || token == "div" ||
        token == "mod" || token == "and")
    {
        string op = token;

        pushOperator(op);

        token = nextToken();

        part();

        code(popOperator(), popOperand(), popOperand());

        factors();
    }
}

void Compiler::part() // stage 1, production 15
// Handles unary ops, literals, ids, parentheses.
{
    string x;

    // not ...
    if (token == "not")
    {
        token = nextToken();

        // not ( expression )
        if (token == "(")
        {
            token = nextToken();

            express();

            if (token != ")")
                processError("')' expected");

            code("not", popOperand());

            token = nextToken();
        }

        // not true / false
        else if (isBoolean(token))
        {
            x = (token == "true") ? "false" : "true";

            pushOperand(x);

            token = nextToken();
        }

        // not identifier
        else if (isNonKeyId(token))
        {
            code("not", token);

            token = nextToken();
        }
        else
            processError("boolean expression expected");
    }

    // unary +
    else if (token == "+")
    {
        token = nextToken();

        if (token == "(")
        {
            token = nextToken();

            express();

            if (token != ")")
                processError("')' expected");

            token = nextToken();
        }
        else if (isInteger(token) || isNonKeyId(token))
        {
            pushOperand(token);
            token = nextToken();
        }
        else
            processError("integer expected after unary '+'");
    }

    // unary -
    else if (token == "-")
    {
        token = nextToken();

        // -( expression )
        if (token == "(")
        {
            token = nextToken();

            express();

            if (token != ")")
                processError("')' expected");

            code("neg", popOperand());

            token = nextToken();
        }

        // -integer literal
        else if (isInteger(token))
        {
            pushOperand("-" + token);
            token = nextToken();
        }

        // -identifier
        else if (isNonKeyId(token))
        {
            code("neg", token);
            token = nextToken();
        }
        else
            processError("integer expression expected");
    }

    // integer / boolean / identifier
    else if (isInteger(token) || isBoolean(token) || isNonKeyId(token))
    {
        pushOperand(token);
        token = nextToken();
    }

    // parenthesized expression
    else if (token == "(")
    {
        token = nextToken();

        express();

        if (token != ")")
            processError("')' expected");

        token = nextToken();
    }

    else
        processError("invalid part");
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

void pushOperator(string op);
string popOperator();
void pushOperand(string operand);
string popOperand();

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
            if (val == "true") val = "-1";
            else if (val == "false") val = "0";

            emit(entry.getInternalName(), "dd", val, "; " + name.substr(0, 15));
        }
    }

    emit("SECTION", ".bss");

    for (map<string, SymbolTableEntry>::iterator it = symbolTable.begin();
         it != symbolTable.end(); ++it)
    {
        if (it->second.getMode() == VARIABLE)
            emit(it->second.getInternalName(), "resd", "1",
                 "; " + it->first.substr(0, 15));
    }
}

void Compiler::emitReadCode(string operand, string operand2)
{
    stringstream ss(operand);
    string name;

    // operand contains comma-separated identifier list
    while (getline(ss, name, ','))
    {
        if (name == "")
            continue;

        // must exist
        if (symbolTable.count(name) == 0)
            processError("reference to undefined symbol");

        // only integer variables may be read
        if (symbolTable.at(name).getDataType() != INTEGER)
            processError("can't read variables of this type");

        // must be writable
        if (symbolTable.at(name).getMode() != VARIABLE)
            processError("attempting to read to a read-only location");

        // call Irvine ReadInt
        emit("", "call", "ReadInt",
             "; read int; value placed in eax");

        // store eax into variable
        emit("", "mov",
             "[" + symbolTable.at(name).getInternalName() + "],eax",
             "; store eax at " + name);

        contentsOfAReg = name;
    }
}

void Compiler::emitWriteCode(string operand, string operand2)
{
    stringstream ss(operand);
    string name;

    while (getline(ss, name, ','))
    {
        if (name == "")
            continue;

        // symbol must exist
        if (symbolTable.count(name) == 0)
            processError("reference to undefined symbol");

        // load into eax if not already there
        if (contentsOfAReg != name)
        {
            emit("", "mov",
                 "eax,[" + symbolTable.at(name).getInternalName() + "]",
                 "; load " + name);

            contentsOfAReg = name;
        }

        // write integer / boolean value
        if (symbolTable.at(name).getDataType() == INTEGER ||
            symbolTable.at(name).getDataType() == BOOLEAN)
        {
            emit("", "call", "WriteInt",
                 "; write value in eax");
        }

        // newline after each item
        emit("", "call", "Crlf");
    }
}

void Compiler::emitAssignCode(string operand1, string operand2) // op2 = op1
// assign the value of operand1 to operand2
{
    // types must match
    if (whichType(operand1) != whichType(operand2))
        processError("incompatible types");

    // left side must be VARIABLE
    if (symbolTable.at(operand2).getMode() != VARIABLE)
        processError("symbol on left-hand side of assignment must have a storage mode of VARIABLE");

    // no work needed
    if (operand1 == operand2)
        return;

    // load operand1 into eax if not already there
    if (contentsOfAReg != operand1)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand1).getInternalName() + "]",
             "; load " + operand1);
    }

    // store eax into operand2
    emit("", "mov",
         "[" + symbolTable.at(operand2).getInternalName() + "],eax",
         "; " + operand2 + " = AReg");

    // register now reflects operand2
    contentsOfAReg = operand2;

    // free temp if RHS was temporary
    if (isTemporary(operand1))
        freeTemp();

    // operand2 is never temporary
}

void Compiler::emitAdditionCode(string operand1, string operand2)	// op2 + op1
// add operand1 to operand2
{
    // both operands must be integers
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

    // If A register holds temp not operand1 or operand2
    if (contentsOfAReg != "" &&
        isTemporary(contentsOfAReg) &&
        contentsOfAReg != operand1 &&
        contentsOfAReg != operand2)
    {
        emit("", "mov",
             "[" + symbolTable.at(contentsOfAReg).getInternalName() + "],eax",
             "; store temp");

        symbolTable.at(contentsOfAReg).setAlloc(YES);
        contentsOfAReg = "";
    }

    // If A register holds non-temp not operand1 or operand2
    else if (contentsOfAReg != "" &&
             !isTemporary(contentsOfAReg) &&
             contentsOfAReg != operand1 &&
             contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
    }

    // If neither operand already in A register, load operand2
    if (contentsOfAReg != operand1 && contentsOfAReg != operand2)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand2).getInternalName() + "]",
             "; load " + operand2);

        contentsOfAReg = operand2;
    }

    // Perform addition
    emit("", "add",
         "eax,[" + symbolTable.at(operand1).getInternalName() + "]",
         "; AReg = " + operand2 + " + " + operand1);

    // Free temps if used
    if (isTemporary(operand1))
        freeTemp();

    if (isTemporary(operand2))
        freeTemp();

    // Create result temp
    string temp = getTemp();
	symbolTable.at(temp).setDataType(INTEGER);

    contentsOfAReg = temp;

    pushOperand(temp);
}

void Compiler::emitSubtractionCode(string operand1, string operand2) // op2 - op1
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

    if (contentsOfAReg != operand2)
        emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]");

    emit("", "sub", "eax,[" + symbolTable.at(operand1).getInternalName() + "]");

    if (isTemporary(operand1)) freeTemp();
    if (isTemporary(operand2)) freeTemp();

    string temp = getTemp();
	symbolTable.at(temp).setDataType(INTEGER);
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitMultiplicationCode(string operand1, string operand2) // op2 * op1
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

    if (contentsOfAReg != operand2)
        emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]");

    emit("", "imul", "dword [" + symbolTable.at(operand1).getInternalName() + "]");

    if (isTemporary(operand1)) freeTemp();
    if (isTemporary(operand2)) freeTemp();

    string temp = getTemp();
	symbolTable.at(temp).setDataType(INTEGER);
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitDivisionCode(string operand1, string operand2)	// op2 / op1
// divide operand2 by operand1
{
    // both operands must be integers
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

    // if A register holds a temp not operand2
    if (contentsOfAReg != "" &&
        isTemporary(contentsOfAReg) &&
        contentsOfAReg != operand2)
    {
        emit("", "mov",
             "[" + symbolTable.at(contentsOfAReg).getInternalName() + "],eax",
             "; store temp");

        symbolTable.at(contentsOfAReg).setAlloc(YES);
        contentsOfAReg = "";
    }

    // if A register holds a non-temp not operand2
    else if (contentsOfAReg != "" &&
             !isTemporary(contentsOfAReg) &&
             contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
    }

    // load operand2 into eax if needed
    if (contentsOfAReg != operand2)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand2).getInternalName() + "]",
             "; load " + operand2);

        contentsOfAReg = operand2;
    }

    // sign extend eax into edx:eax
    emit("", "cdq", "", "; sign extend dividend");

    // divide edx:eax by operand1
    emit("", "idiv",
         "dword [" + symbolTable.at(operand1).getInternalName() + "]",
         "; AReg = " + operand2 + " div " + operand1);

    // free temps used as operands
    if (isTemporary(operand1))
        freeTemp();

    if (isTemporary(operand2))
        freeTemp();

    // result now in eax
    string temp = getTemp();
	symbolTable.at(temp).setDataType(INTEGER);

    contentsOfAReg = temp;

    pushOperand(temp);
}

void Compiler::emitModuloCode(string operand1, string operand2) // op2 % op1
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

    if (contentsOfAReg != operand2)
        emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]");

    emit("", "cdq");
    emit("", "idiv", "dword [" + symbolTable.at(operand1).getInternalName() + "]");
    emit("", "mov", "eax,edx");

    if (isTemporary(operand1)) freeTemp();
    if (isTemporary(operand2)) freeTemp();

    string temp = getTemp();
	symbolTable.at(temp).setDataType(INTEGER);
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitNegationCode(string operand1, string)
// -operand1
{
    if (whichType(operand1) != INTEGER)
        processError("illegal type");

    if (contentsOfAReg != operand1)
        emit("", "mov", "eax,[" + symbolTable.at(operand1).getInternalName() + "]");

    emit("", "neg", "eax");

    if (isTemporary(operand1)) freeTemp();

    string temp = getTemp();
	symbolTable.at(temp).setDataType(INTEGER);
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitNotCode(string operand1, string)
// not operand1
{
    if (whichType(operand1) != BOOLEAN)
        processError("illegal type");

    if (contentsOfAReg != operand1)
        emit("", "mov", "eax,[" + symbolTable.at(operand1).getInternalName() + "]");

    emit("", "not", "eax");

    if (isTemporary(operand1)) freeTemp();

    string temp = getTemp();
	symbolTable.at(temp).setDataType(BOOLEAN);
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitAndCode(string operand1, string operand2)	// op2 && op1
// and operand1 to operand2
{
    // both operands must be boolean
    if (whichType(operand1) != BOOLEAN || whichType(operand2) != BOOLEAN)
        processError("illegal type");

    // if A register holds temp not operand1 nor operand2
    if (contentsOfAReg != "" &&
        isTemporary(contentsOfAReg) &&
        contentsOfAReg != operand1 &&
        contentsOfAReg != operand2)
    {
        emit("", "mov",
             "[" + symbolTable.at(contentsOfAReg).getInternalName() + "],eax",
             "; store temp");

		symbolTable.at(contentsOfAReg).setAlloc(YES);
        contentsOfAReg = "";
    }

    // if A register holds non-temp not operand1 nor operand2
    else if (contentsOfAReg != "" &&
             !isTemporary(contentsOfAReg) &&
             contentsOfAReg != operand1 &&
             contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
    }

    // if neither operand is already in eax, load operand2
    if (contentsOfAReg != operand1 && contentsOfAReg != operand2)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand2).getInternalName() + "]",
             "; load " + operand2);

        contentsOfAReg = operand2;
    }

    // boolean AND
    emit("", "and",
         "eax,[" + symbolTable.at(operand1).getInternalName() + "]",
         "; AReg = " + operand2 + " and " + operand1);

    // free temporaries used
    if (isTemporary(operand1))
        freeTemp();

    if (isTemporary(operand2))
        freeTemp();

    // result in eax is boolean
    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);

    contentsOfAReg = temp;

    pushOperand(temp);
}

void Compiler::emitOrCode(string operand1, string operand2) // op2 || op1
{
    if (whichType(operand1) != BOOLEAN || whichType(operand2) != BOOLEAN)
        processError("illegal type");

    if (contentsOfAReg != operand2)
        emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]");

    emit("", "or", "eax,[" + symbolTable.at(operand1).getInternalName() + "]");

    if (isTemporary(operand1)) freeTemp();
    if (isTemporary(operand2)) freeTemp();

    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitEqualityCode(string operand1, string operand2) // op2 == op1
// test whether operand2 equals operand1
{
    // operands must be same type
    if (whichType(operand1) != whichType(operand2))
        processError("incompatible types");

    // if A register holds temp not operand1 nor operand2
    if (contentsOfAReg != "" &&
        isTemporary(contentsOfAReg) &&
        contentsOfAReg != operand1 &&
        contentsOfAReg != operand2)
    {
        emit("", "mov",
             "[" + symbolTable.at(contentsOfAReg).getInternalName() + "],eax",
             "; store temp");

		symbolTable.at(contentsOfAReg).setAlloc(YES);
        contentsOfAReg = "";
    }

    // if A register holds non-temp not operand1 nor operand2
    else if (contentsOfAReg != "" &&
             !isTemporary(contentsOfAReg) &&
             contentsOfAReg != operand1 &&
             contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
    }

    // if neither operand in A register, load operand2
    if (contentsOfAReg != operand1 && contentsOfAReg != operand2)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand2).getInternalName() + "]",
             "; load " + operand2);

        contentsOfAReg = operand2;
    }

    // compare eax with operand1
    emit("", "cmp",
         "eax,[" + symbolTable.at(operand1).getInternalName() + "]",
         "; compare");

    string trueLabel = getLabel();
    string endLabel  = getLabel();

    // jump if equal
    emit("", "je", trueLabel);

    // false result = 0
    if (symbolTable.count("false") == 0)
        insert("false", BOOLEAN, CONSTANT, "false", YES, 1);

    emit("", "mov",
         "eax,[" + symbolTable.at("false").getInternalName() + "]",
         "; load FALSE");

    emit("", "jmp", endLabel);

    // true label
    emit(trueLabel + ":", "", "", "");

    if (symbolTable.count("true") == 0)
        insert("true", BOOLEAN, CONSTANT, "true", YES, 1);

    emit("", "mov",
         "eax,[" + symbolTable.at("true").getInternalName() + "]",
         "; load TRUE");

    // end label
    emit(endLabel + ":", "", "", "");

    // free temps used
    if (isTemporary(operand1))
        freeTemp();

    if (isTemporary(operand2))
        freeTemp();

    // boolean result now in eax
    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);

    contentsOfAReg = temp;

    pushOperand(temp);
}

void Compiler::emitInequalityCode(string operand1, string operand2) // op2 != op1
{
    emitEqualityCode(operand1, operand2);
    emit("", "not", "eax", "; negate equality");

    // result is boolean in eax, not a temp symbol
}

void Compiler::emitLessThanCode(string operand1, string operand2) // op2 < op1
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

    if (contentsOfAReg != operand2)
        emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]");

    emit("", "cmp", "eax,[" + symbolTable.at(operand1).getInternalName() + "]");

    string t = getLabel(), e = getLabel();

    emit("", "jl", t);
    emit("", "mov", "eax,0");
    emit("", "jmp", e);
    emit(t + ":", "", "", "");
    emit("", "mov", "eax,-1");
    emit(e + ":", "", "", "");

    if (isTemporary(operand1)) freeTemp();
    if (isTemporary(operand2)) freeTemp();

    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitLessThanOrEqualToCode(string operand1, string operand2)
// operand2 <= operand1
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

    if (contentsOfAReg != operand2)
        emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]");

    emit("", "cmp", "eax,[" + symbolTable.at(operand1).getInternalName() + "]");

    string t = getLabel(), e = getLabel();

    emit("", "jle", t);
    emit("", "mov", "eax,0");
    emit("", "jmp", e);
    emit(t + ":", "", "", "");
    emit("", "mov", "eax,-1");
    emit(e + ":", "", "", "");

    if (isTemporary(operand1)) freeTemp();
    if (isTemporary(operand2)) freeTemp();

    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitGreaterThanCode(string operand1, string operand2) // op2 > op1
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

    if (contentsOfAReg != operand2)
        emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]");

    emit("", "cmp", "eax,[" + symbolTable.at(operand1).getInternalName() + "]");

    string t = getLabel(), e = getLabel();

    emit("", "jg", t);
    emit("", "mov", "eax,0");
    emit("", "jmp", e);
    emit(t + ":", "", "", "");
    emit("", "mov", "eax,-1");
    emit(e + ":", "", "", "");

    if (isTemporary(operand1)) freeTemp();
    if (isTemporary(operand2)) freeTemp();

    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitGreaterThanOrEqualToCode(string operand1, string operand2)
// operand2 >= operand1
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

    if (contentsOfAReg != operand2)
        emit("", "mov", "eax,[" + symbolTable.at(operand2).getInternalName() + "]");

    emit("", "cmp", "eax,[" + symbolTable.at(operand1).getInternalName() + "]");

    string t = getLabel(), e = getLabel();

    emit("", "jge", t);
    emit("", "mov", "eax,0");
    emit("", "jmp", e);
    emit(t + ":", "", "", "");
    emit("", "mov", "eax,-1");
    emit(e + ":", "", "", "");

    if (isTemporary(operand1)) freeTemp();
    if (isTemporary(operand2)) freeTemp();

    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);
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
    listingFile << endl;
    listingFile << "Error: Line " << lineNo << ": " << err << endl;
    errorCount++;
    createListingTrailer();
    exit(EXIT_FAILURE);
}

void Compiler::freeTemp()
{
    currentTempNo--;

    if (currentTempNo < -1)
        processError("compiler error, currentTempNo should be >= -1");
}

string Compiler::getTemp()
{
    currentTempNo++;

    string temp = "T" + to_string(currentTempNo);

    if (currentTempNo > maxTempNo)
    {
        insert(temp, UNKNOWN, VARIABLE, "", NO, 1);
        maxTempNo++;
    }

    return temp;
}

string Compiler::getLabel()
{
    static int labelNo = 0;			// resets label to 0 every time

    return "L" + to_string(labelNo++);
}

bool Compiler::isTemporary(string s) const	// determines if s represents a temporary
{
    return !s.empty() && s[0] == 'T';
}

// To test the program:
/*
mkdir stage1
cd stage1

cp /usr/local/4301/src/Makefile .
> edit Makefile -> targets2srcfiles - stage1
cp /usr/local/4301/include/stage1.h .
cp /usr/local/4301/src/stage1main.C .

make stage1

DATA FILES:
ls /usr/local/4301/data/stage1/

cp /usr/local/4301/data/stage1/101.dat .
cat 101.dat

./stage1 101.dat 101.lst 101.asm

cat 101.lst
cat 101.asm

EDIT MAKEFILE:
targetsAsmLanguage = 101
make 101
./101

diff /usr/local/4301/data/stage1/101.lst 101.lst
diff /usr/local/4301/data/stage1/101.asm 101.asm
*/
