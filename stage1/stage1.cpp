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
	// main does this: createListingHeader, parser, createListingTrailer
    nextChar();
    token = nextToken();

    if (token != "program")
        processError("keyword 'program' expected");

    prog();
	return;
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

void Compiler::progStmt()       // stage 0, production 2
{
	// BOTH returns AND assigns "token"
	// also, our specs require "x = nextToken()"
	// #deterministicTokenFlow
	
    if (token != "program")
        processError("keyword 'program' expected");

    string x = nextToken();

    if (!isNonKeyId(token))
        processError("program name expected");

    if (nextToken() != ";")
        processError("semicolon expected");

    nextToken();

    code("program", x);

    insert(x, PROG_NAME, CONSTANT, x, NO, 0);
}

void Compiler::consts()         // stage 0, production 3
{
	// only validate identifier, let constStmts() handle it
    if (token != "const")
        processError("keyword 'const' expected");

    token = nextToken();

    if (!isNonKeyId(token))
        processError("non-keyword identifier must follow 'const'");

    constStmts();
}

void Compiler::vars()           // stage 0, production 4
{
    if (token != "var")
        processError("keyword 'var' expected");

    token = nextToken();

    if (!isNonKeyId(token))
        processError("non-keyword identifier must follow 'var'");

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

void Compiler::constStmts()     // stage 0, production 6
{
    while (isNonKeyId(token))
    {
        string x = token;
        token = nextToken();

        if (token != "=")
            processError("'=' expected");

        token = nextToken();

        string y;

        if (token == "+" || token == "-")
        {
            string sign = token;
            token = nextToken();

            if (!isInteger(token))
                processError("integer expected after sign");

            y = sign + token;
        }
        else if (token == "not")
        {
            token = nextToken();

            if (!isBoolean(token))
                processError("boolean literal expected after 'not'");

            y = (token == "true") ? "false" : "true";
        }
        else
        {
            y = token;
        }

        token = nextToken();

        if (token != ";")
            processError("semicolon expected");

        storeTypes t = whichType(y);
        insert(x, t, CONSTANT, whichValue(y), YES, 1);

        token = nextToken();
    }
}

//////////////////// VAR ////////////////////

void Compiler::varStmts()       // stage 0, production 7
{
    while (isNonKeyId(token))
    {
        string idList = ids();

        if (token != ":")
            processError("\":\" expected");

        token = nextToken();

        if (token != "integer" && token != "boolean")
            processError("illegal type follows \":\"");

        string type = token;
        token = nextToken();

        if (token != ";")
            processError("semicolon expected");

        // insert identifiers
        stringstream ss(idList);
        string id;

        while (getline(ss, id, ','))
        {
            insert(id,
                   (type == "integer" ? INTEGER : BOOLEAN),
                   VARIABLE, "", YES, 1);
        }

        token = nextToken();  // move to next declaration OR begin
    }
}

string Compiler::ids()          // stage 0, production 8
{
    if (!isNonKeyId(token))
        processError("non-keyword identifier expected");

    string result = token;

    token = nextToken();

    while (token == ",")
    {
        token = nextToken();

        if (!isNonKeyId(token))
            processError("non-keyword identifier expected");

        result = result + "," + token;

        token = nextToken();
    }

    return result;
}

//////////////////// EXPRESSIONS ////////////////////

void Compiler::execStmts() // stage 1, production 2
{
	// EXEC_STMT EXEC_STMTS | epsilon
	// NON_KEY_ID, 'read', 'write', 'end'
	
    while (token == "read" || token == "write" || isNonKeyId(token))
    {
        try
        {
            execStmt();
        }
        catch (...)
        {
            // synchronize to next statement boundary
            while (token != ";" && token != "end" && token != "EOF")
                token = nextToken();

            if (token == ";")
                token = nextToken();
        }
    }
}

void Compiler::execStmt() // stage 1, production 3
{
	// ASSIGN_STMT | READ_STMT | WRITE_STMT
	// NON_KEY_ID, 'read', 'write'

    if (token == "read")
        readStmt();
    else if (token == "write")
        writeStmt();
    else if (isNonKeyId(token))
        assignStmt();
    else
        processError("statement expected");
}

void Compiler::assignStmt() // stage 1, production 4
{
	// NON_KEY_ID ':=' EXPRESS ';'
	// NON_KEY_ID

    string lhs = token;
    token = nextToken(); // consume id

    if (token != ":=")
        processError("':=' expected");

    token = nextToken(); // move into expression

    express();

    if (token != ";")
        processError("one of \"*\", \"and\", \"div\", \"mod\", \")\", \"+\", \"-\", \";\", \"<\", \"<=\", \"<>\", \"=\", \">\", \">=\", or \"or\" expected");

    string rhs = popOperand();  // expression result

    code(":=", rhs, lhs);

    token = nextToken();
}

void Compiler::readStmt() // stage 1, production 5
{
	// 'read' READ_LIST ';'
	// 'read'

    token = nextToken();  // consume 'read'

    if (token != "(")
        processError("'(' expected");

    token = nextToken();

    string x = ids();

    if (token != ")")
        processError("')' expected");

    token = nextToken();

    if (token != ";")
        processError("semicolon expected");

    code("read", x);

    token = nextToken();   // next statement
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

    while (token == "+" || token == "-" || token == "or")
    {
        string op = token;

        pushOperator(op);

        token = nextToken();

        term();

        code(popOperator(), popOperand(), popOperand());
    }
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

    while (token == "=" || token == "<>" ||
           token == "<" || token == "<=" ||
           token == ">" || token == ">=")
    {
        string op = token;

        pushOperator(op);

        token = nextToken();

        factor();

        code(popOperator(), popOperand(), popOperand());
    }
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

    while (token == "*" || token == "div" ||
           token == "mod" || token == "and")
    {
        string op = token;

        pushOperator(op);

        token = nextToken();

        part();

        code(popOperator(), popOperand(), popOperand());
    }
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

			token = nextToken();

			code("not", popOperand());
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
            processError("expected '(', integer, or non-keyword id; found +");
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

	// integer literal
	else if (isInteger(token))
	{
		pushOperand(token);
		token = nextToken();
	}

	// boolean literal
	else if (token == "true" || token == "false")
	{
		pushOperand(token);
		token = nextToken();
	}

	// identifier
	else if (isNonKeyId(token))
	{
		whichType(token);   // verify declared
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

bool Compiler::isKeyword(string s) const  // determines if s is a keyword
{
    return (s == "program" || s == "const" || s == "var" ||
            s == "begin" || s == "end" ||
            s == "integer" || s == "boolean" ||
            s == "read" || s == "write");
}

bool Compiler::isSpecialSymbol(char c) const
{
	// determines if c is a special symbol
	
    string symbols = "+-*/()=,;:.<>";

    return symbols.find(c) != string::npos;
}

bool Compiler::isNonKeyId(string s) const // determines if s is a non_key_id
{
    if (s.empty() || !islower(s[0]))
        return false;

    for (char c : s)
        if (!isalnum(c) && c != '_')
            return false;

    return !isKeyword(s);
}

bool Compiler::isInteger(string s) const  // determines if s is an integer
{
    if (s.empty()) return false;

    int i = 0;

    if (s[0] == '+' || s[0] == '-')
        i = 1;

    if (i == (int)s.size())
        return false;

    for (; i < (int)s.size(); i++)
        if (!isdigit(s[i]))
            return false;

    return true;
}

bool Compiler::isBoolean(string s) const  // determines if s is a boolean
{
	return (s == "true" || s == "false");
}

bool Compiler::isLiteral(string s) const  // determines if s is a literal
{
	return isInteger(s) || isBoolean(s);
}

//////////////////// ACTION ////////////////////

void Compiler::insert(string externalName,
                      storeTypes inType,
                      modes inMode,
                      string inValue,
                      allocation inAlloc,
                      int inUnits)
{
	// NO PARSING HERE, just insertion
    if (symbolTable.count(externalName) > 0)
	{
        processError("symbol " + externalName + " is multiply defined");
	}

	if (isKeyword(externalName))		// keyword cannot be used as an identifier
		processError("incompatible types for operator ':='");

    string internalName;

    if (isupper(externalName[0]))
        internalName = externalName;
    else
        internalName = genInternalName(inType);

    symbolTable.insert({
        externalName,
        SymbolTableEntry(internalName, inType, inMode,
                         inValue, inAlloc, inUnits)
    });
}
			
storeTypes Compiler::whichType(string name)			// allow other types beside constant
{
    if (isInteger(name))
        return INTEGER;

    if (isBoolean(name))
        return BOOLEAN;

    auto it = symbolTable.find(name);

    if (it == symbolTable.end())
        processError("reference to undefined symbol " + name);
    
    // Check if we are trying to use the program name as a value
    if (it->second.getDataType() == PROG_NAME)
        processError("program name cannot be used in an expression");

    return it->second.getDataType();
}

string Compiler::whichValue(string name)
{
    if (isInteger(name) || isBoolean(name))
        return name;

    auto it = symbolTable.find(name);

    if (it == symbolTable.end())
        processError("reference to undefined constant");

    // Logic safety: if it's in the table but has no value (like a variable), error out
    if (it->second.getValue() == "")
        processError("identifier has no constant value");

    return it->second.getValue();
}

void Compiler::code(string op, string operand1, string operand2)
{
    if (op == "program")
        emitPrologue(operand1);

    else if (op == "end")
        emitEpilogue();

    else if (op == "read")
        emitReadCode(operand1);

    else if (op == "write")
        emitWriteCode(operand1);

    else if (op == "+")              // binary +
        emitAdditionCode(operand1, operand2);

    else if (op == "-")              // binary -
        emitSubtractionCode(operand1, operand2);

    else if (op == "neg")            // unary -
        emitNegationCode(operand1);

    else if (op == "not")
        emitNotCode(operand1);

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

    else if (op == ":=")
        emitAssignCode(operand1, operand2);

    else
        processError("compiler error since function code should not be called with illegal arguments");
}

//////////////////// ACTION ROUTINES ////////////////////

void Compiler::pushOperator(string op)	// push name onto operatorStk
{
	operatorStk.push(op);
	return;
}

void Compiler::pushOperand(string operand)	// push name onto operandStk
{
    // If literal and not already in symbol table, create entry
    if (isLiteral(operand) && symbolTable.count(operand) == 0)
    {
        insert(operand,
               whichType(operand),
               CONSTANT,
               whichValue(operand),
               YES,
               1);
    }

    operandStk.push(operand);
}	

string Compiler::popOperator()	// pop name from operatorStk
{
    if (!operatorStk.empty())
    {
        string top = operatorStk.top();
        operatorStk.pop();
        return top;
    }

    processError("compiler error; operator stack underflow");
    return "";
}

string Compiler::popOperand()		// pop name from operandStk
{
    if (!operandStk.empty())
    {
        string top = operandStk.top();
        operandStk.pop();
        return top;
    }

    processError("compiler error; operand stack underflow");
    return "";
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
        processError("incompatible types for operator ':='");

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
        processError("binary '+' requires integer operands");

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

    // Preserve temp in AReg if needed
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
    else if (contentsOfAReg != "" &&
             !isTemporary(contentsOfAReg) &&
             contentsOfAReg != operand1 &&
             contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
    }

    // Load operand2 if needed
    if (contentsOfAReg != operand2)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand2).getInternalName() + "]",
             "; load " + operand2);

        contentsOfAReg = operand2;
    }

    emit("", "sub",
         "eax,[" + symbolTable.at(operand1).getInternalName() + "]",
         "; AReg = " + operand2 + " - " + operand1);

    if (isTemporary(operand1))
        freeTemp();

    if (isTemporary(operand2))
        freeTemp();

    string temp = getTemp();
    symbolTable.at(temp).setDataType(INTEGER);

    contentsOfAReg = temp;

    pushOperand(temp);
}

void Compiler::emitMultiplicationCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

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
    else if (contentsOfAReg != "" &&
             !isTemporary(contentsOfAReg) &&
             contentsOfAReg != operand1 &&
             contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
    }

    if (contentsOfAReg != operand2)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand2).getInternalName() + "]",
             "; load " + operand2);

        contentsOfAReg = operand2;
    }

    emit("", "imul",
         "dword [" + symbolTable.at(operand1).getInternalName() + "]",
         "; AReg = " + operand2 + " * " + operand1);

    if (isTemporary(operand1))
        freeTemp();

    if (isTemporary(operand2))
        freeTemp();

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

void Compiler::emitModuloCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

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
    else if (contentsOfAReg != "" &&
             !isTemporary(contentsOfAReg) &&
             contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
    }

    if (contentsOfAReg != operand2)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand2).getInternalName() + "]",
             "; load " + operand2);

        contentsOfAReg = operand2;
    }

    emit("", "cdq", "", "; sign extend dividend");

    emit("", "idiv",
         "dword [" + symbolTable.at(operand1).getInternalName() + "]",
         "; divide");

    emit("", "mov", "eax,edx", "; remainder into eax");

    if (isTemporary(operand1))
        freeTemp();

    if (isTemporary(operand2))
        freeTemp();

    string temp = getTemp();
    symbolTable.at(temp).setDataType(INTEGER);

    contentsOfAReg = temp;

    pushOperand(temp);
}

void Compiler::emitNegationCode(string operand1, string)
{
    if (whichType(operand1) != INTEGER)
        processError("illegal type");

    if (contentsOfAReg != operand1)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand1).getInternalName() + "]",
             "; load " + operand1);

        contentsOfAReg = operand1;
    }

    emit("", "neg", "eax",
         "; negate " + operand1);

    if (isTemporary(operand1))
        freeTemp();

    string temp = getTemp();
    symbolTable.at(temp).setDataType(INTEGER);

    contentsOfAReg = temp;

    pushOperand(temp);
}

	// bitwise not is not like Pascal (TRUE = -1, FALSE = 0)
	// keep not eax if compiler explicitly uses 0/-1 -> emit("", "not", "eax");
	
void Compiler::emitNotCode(string operand1, string)	// NOT op1
{
    if (whichType(operand1) != BOOLEAN)
        processError("illegal type");

    if (contentsOfAReg != operand1)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand1).getInternalName() + "]",
             "; load " + operand1);

        contentsOfAReg = operand1;
    }

    emit("", "not", "eax",
         "; not " + operand1);

    if (isTemporary(operand1))
        freeTemp();

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
        processError("binary 'and' requires boolean operands");

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

    emit("", "or", "eax,[" + symbolTable.at(operand1).getInternalName() + "]");

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

void Compiler::emitLessThanCode(string operand1, string operand2)
{
    if (whichType(operand1) != INTEGER || whichType(operand2) != INTEGER)
        processError("illegal type");

    // A register handling (same as yours)
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
    else if (contentsOfAReg != "" &&
             !isTemporary(contentsOfAReg) &&
             contentsOfAReg != operand1 &&
             contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
    }

    if (contentsOfAReg != operand2)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand2).getInternalName() + "]",
             "; load " + operand2);

        contentsOfAReg = operand2;
    }

    emit("", "cmp",
         "eax,[" + symbolTable.at(operand1).getInternalName() + "]",
         "; compare");

    string trueLabel = getLabel();
    string endLabel  = getLabel();

    emit("", "jl", trueLabel);
    emit("", "mov", "eax,0");
    emit("", "jmp", endLabel);

    emit(trueLabel + ":", "", "", "");
    emit("", "mov", "eax,-1");

    emit(endLabel + ":", "", "", "");

    // NOW create temp result
    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);

    contentsOfAReg = temp;
    pushOperand(temp);

    if (isTemporary(operand1))
        freeTemp();

    if (isTemporary(operand2))
        freeTemp();
}

void Compiler::emitLessThanOrEqualToCode(string operand1, string operand2)
{
    if (whichType(operand1) != whichType(operand2))
        processError("incompatible types");

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
    else if (contentsOfAReg != "" &&
             !isTemporary(contentsOfAReg) &&
             contentsOfAReg != operand1 &&
             contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
    }

    if (contentsOfAReg != operand2)
    {
        emit("", "mov",
             "eax,[" + symbolTable.at(operand2).getInternalName() + "]",
             "; load " + operand2);

        contentsOfAReg = operand2;
    }

    emit("", "cmp",
         "eax,[" + symbolTable.at(operand1).getInternalName() + "]",
         "; compare");

    string trueLabel = getLabel();
    string endLabel  = getLabel();

    emit("", "jle", trueLabel);
    emit("", "mov", "eax,0");
    emit("", "jmp", endLabel);

    emit(trueLabel + ":", "", "", "");
    emit("", "mov", "eax,-1");

    emit(endLabel + ":", "", "", "");

    if (isTemporary(operand1)) freeTemp();
    if (isTemporary(operand2)) freeTemp();

    string temp = getTemp();
    symbolTable.at(temp).setDataType(BOOLEAN);

    contentsOfAReg = temp;
    pushOperand(temp);
}

void Compiler::emitGreaterThanCode(string operand1, string operand2) // op2 > op1
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

	emit("", "cmp",
		 "eax,[" + symbolTable.at(operand1).getInternalName() + "]",
		 "; compare");

	emit("", "jg", trueLabel);

	emit("", "mov", "eax,0");
	emit("", "jmp", endLabel);

	emit(trueLabel + ":", "", "", "");
	emit("", "mov", "eax,-1");

	emit(endLabel + ":", "", "", "");

	// store result properly
	string temp = getTemp();
	symbolTable.at(temp).setDataType(BOOLEAN);

	emit("", "mov",
		 "[" + symbolTable.at(temp).getInternalName() + "],eax",
		 "; store result");

	contentsOfAReg = temp;
	pushOperand(temp);
}

void Compiler::emitGreaterThanOrEqualToCode(string operand1, string operand2)
// operand2 >= operand1
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

	emit("", "cmp",
		 "eax,[" + symbolTable.at(operand1).getInternalName() + "]",
		 "; compare");

	emit("", "jle", trueLabel);

	emit("", "mov", "eax,0");
	emit("", "jmp", endLabel);

	emit(trueLabel + ":", "", "", "");
	emit("", "mov", "eax,-1");

	emit(endLabel + ":", "", "", "");

	// NOW store result
	string temp = getTemp();
	symbolTable.at(temp).setDataType(BOOLEAN);

	// store eax -> temp
	emit("", "mov",
		 "[" + symbolTable.at(temp).getInternalName() + "],eax",
		 "; store result");

	contentsOfAReg = temp;
	pushOperand(temp);
}

//////////////////// LEXICON ////////////////////

char Compiler::nextChar()			// gets raw chars
{
    char next;

    if (!sourceFile.get(next)) {
        ch = END_OF_FILE;
        return ch;   // removed extra newline
    }

    ch = next;

    if (listingHeaderCreated && begChar) {
        listingFile << std::right << std::setw(5) << lineNo << "|";
        begChar = false;
    }

    if (listingHeaderCreated)
        listingFile << ch;

	// int errLine = (ch == '\n') ? lineNo - 1 : lineNo;
    if (ch == '\n') {
        lineNo++;
        begChar = true;
    }

    return ch;
}
	
string Compiler::nextToken()			// builds tokens out of raw chars
{
    token = "";

    while (token == "")
    {
        // skip whitespace
        if (isspace(ch))
        {
            nextChar();
            continue;
        }

        // comments
        if (ch == '{')
        {
            do {
                nextChar();
            } while (ch != '}' && ch != END_OF_FILE);

            if (ch == END_OF_FILE)
                processError("unexpected end of file");

            nextChar();
            continue;
        }

        if (ch == '}')
        {
            processError("'}' cannot begin token");
            nextChar();
            continue;
        }

        // EOF
        if (ch == END_OF_FILE)
        {
            token = "EOF";
            return token;
        }

        // =========================
        // multi-character operators
        // =========================

        // :=
        if (ch == ':')
        {
            nextChar();

            if (ch == '=')
            {
                token = ":=";
                nextChar();
            }
            else
            {
                token = ":";
            }
            return token;
        }

        // <=, <>, <
        if (ch == '<')
        {
            nextChar();

            if (ch == '=')
            {
                token = "<=";
                nextChar();
            }
            else if (ch == '>')
            {
                token = "<>";
                nextChar();
            }
            else
            {
                token = "<";
            }
            return token;
        }

        // >=, >
        if (ch == '>')
        {
            nextChar();

            if (ch == '=')
            {
                token = ">=";
                nextChar();
            }
            else
            {
                token = ">";
            }
            return token;
        }

        // =========================
        // identifiers
        // =========================
        if (islower(ch))
        {
            token = ch;

            nextChar();
            while (isalnum(ch) || ch == '_')
            {
                token += ch;
                nextChar();
            }

            return token;
        }

        // integers
        if (isdigit(ch))
        {
            token = ch;

            nextChar();
            while (isdigit(ch))
            {
                token += ch;
                nextChar();
            }

            return token;
        }

        // single-character symbols
        if (isSpecialSymbol(ch))
        {
            token = ch;
            nextChar();
            return token;
        }

        // fallback error
        processError("illegal symbol");
        nextChar();
    }

    return token;
}
  
//////////////////// ERROR ////////////////////

string Compiler::genInternalName(storeTypes stype) const
{
    if (stype == INTEGER)
    {
        return "I" + to_string(I_count++);
    }
    else if (stype == BOOLEAN)
    {
        return "B" + to_string(B_count++);
    }
    else
    {
        return "X";
    }
}

void Compiler::processError(string err)
{
    errorCount++;

    int errLine = lineNo;

    // if we are past newline, we are already on NEXT line
    if (begChar == true)
        errLine--;   // adjust back to actual source line

    listingFile << endl
                << "Error: "
                << "Line " << errLine << ": "
                << err << endl << endl;

    createListingTrailer();

    listingFile.flush();
    objectFile.flush();

    exit(EXIT_FAILURE);
}

//////////////////// TEMP HANDLING ////////////////////

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
