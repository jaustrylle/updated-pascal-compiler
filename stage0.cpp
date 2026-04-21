// Amiran Fields, Serena Reese
// CS4301
// Stage 0

#include <stage0.h>

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include <stdio.h>		// to close files
#include <cctype>		// nextChar, nextToken
#include <iomanip>		// format in/out streams using setw
#include <ctime>		// time in listing header
#include <vector>		// for ids() lists

using namespace std;

/*
Our specs enforce this structure STRICTLY:
1. program
2. progStmt
3. const (optional)
4. var (optional)
5. begin
6. beginEndStmt
7. EOF

Each grammar function assumes token already holds the current symbol.
*/

// ------------------------------------------------------------- //
// PUBLIC functions declared in stage0.h //
/*
string getInternalName() const
storeTypes getDataType() const
modes getMode() const
string getValue() const
allocation getAlloc() const
int getUnits() const
void setInternalName(string s)
void setDataType(storeTypes st)
void setMode(modes m)
void setValue(string s)
void setAlloc(allocation a)
void setUnits(int i)
*/

// ------------------------------------------------------------- //
// GLOBAL VARIABLES AND FUNCTIONS

int I_count = 0;
int B_count = 0;
bool listingHeaderCreated = false;
bool begChar = true;

// ------------------------------------------------------------- //

Compiler::Compiler(char **argv)		// constructor
{
    if (!argv || !argv[1] || !argv[2] || !argv[3])
        processError("missing command line arguments");

    sourceFile.open(argv[1]);
    listingFile.open(argv[2]);
    objectFile.open(argv[3]);
	
    if (!sourceFile)
        processError("Cannot open source file");
    if (!listingFile)
        processError("Cannot open listing file");
    if (!objectFile)
        processError("Cannot open object file");

    lineNo = 1;
}

// this function closes all files
Compiler::~Compiler()							// destructor
{
	// add error handling for if file already open
	sourceFile.close();
	listingFile.close();
	objectFile.close();
}

//////////////////// OUTPUT ////////////////////

void Compiler::createListingHeader()
{
    time_t now = time(0);

    listingFile << "STAGE0:  AMIRAN FIELDS, SERENA REESE"
                << "\t\t" << ctime(&now)
                << "\n\n";

	listingFile << std::left
				<< std::setw(22) << "LINE NO."
				<< "SOURCE STATEMENT\n\n";

    listingHeaderCreated = true;
}

void Compiler::parser()
{
    nextChar();
    token = nextToken();

    if (token != "program")
        processError("keyword 'program' expected");

    prog();

    createListingTrailer();
}

void Compiler::createListingTrailer()
{
	listingFile << "COMPILATION TERMINATED      "
				<< errorCount
				<< " ERRORS ENCOUNTERED\n";
}

//////////////////// GRAMMAR ////////////////////

void Compiler::prog()           // stage 0, production 1
{
	// EOF is NOT the same as END_OF_FILE!
    if (token != "program")
        processError("keyword 'program' expected");

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

void Compiler::beginEndStmt()   // stage 0, production 5
{	
	if (token != "begin")
		processError("keyword 'begin' expected");

	token = nextToken();

	if (token != "end")
		processError("keyword 'end' expected");

	token = nextToken();

	if (token != ".")
		processError("period expected");

	token = nextToken();   // should now be EOF or nothing

	code("end", ".");
}

//////////////////// CONST ////////////////////

void Compiler::constStmts()     // stage 0, production 6
{
    while (isNonKeyId(token))
    {
        string x = token;

        // already ensured by while, but kept for safety
        if (!isNonKeyId(x))
            processError("non-keyword identifier expected");

        token = nextToken();

        if (token != "=")
            processError("'=' expected");

        token = nextToken(); // start RHS

        string y;

        // -------------------------
        // CASE 1: + or -
        // -------------------------
        if (token == "+" || token == "-")
        {
            string sign = token;

            token = nextToken();   // move to number

            if (!isInteger(token))
                processError("integer expected after sign");

            y = sign + token;
        }

        // -------------------------
        // CASE 2: not
        // -------------------------
        else if (token == "not")
        {
            token = nextToken();   // move to boolean

            if (!isBoolean(token))
                processError("boolean expected after 'not'");

            y = (token == "true") ? "false" : "true";
        }

        // -------------------------
        // CASE 3: normal literal / id
        // -------------------------
        else
        {
            y = token;
        }

        token = nextToken();

        if (token != ";")
            processError("semicolon expected");

        // determine type (this is enough — no extra check needed)
        storeTypes t = whichType(y);

        insert(x, t, CONSTANT, whichValue(y), YES, 1);

        token = nextToken(); // next constant or exit
    }
}

//////////////////// VAR ////////////////////

void Compiler::varStmts()       // stage 0, production 7
{
    string x, y;

    if (!isNonKeyId(token))
        processError("non-keyword identifier expected");

    x = ids();

    if (token != ":")
        processError("':' expected");

    token = nextToken();

    if (token != "integer" && token != "boolean")
        processError("illegal type follows ':'");

    y = token;

    if (nextToken() != ";")
        processError("semicolon expected");

    insert(x,
           (y == "integer" ? INTEGER : BOOLEAN),
           VARIABLE, "", YES, 1);

    token = nextToken();

    if (token == "begin")
        return;

    if (isNonKeyId(token))
        varStmts();
    else
        processError("non-keyword identifier or 'begin' expected");
}

string Compiler::ids()          // stage 0, production 8
{
    if (!isNonKeyId(token))
        processError("non-keyword identifier expected");

    string result = token;

    token = nextToken();

    if (token == ",")
    {
        token = nextToken();
        result = result + "," + ids();
    }

    return result;
}

//////////////////// HELPER ////////////////////

bool Compiler::isKeyword(string s) const  // determines if s is a keyword
{
	return (s == "program" || s == "const" || s == "var" ||
			s == "begin" || s == "end" ||
			s == "integer" || s == "boolean" ||
			s == "true" || s == "false" || s == "not");
}

bool Compiler::isSpecialSymbol(char c) const
{
	// determines if c is a special symbol
	
	string symbols = "+-*/()=,;:.";
	return symbols.find(c) != string::npos;
}

bool Compiler::isNonKeyId(string s) const // determines if s is a non_key_id
{
	if (s.empty() || !islower(s[0]))
		return false;
	
	for (char c : s)
		if (!isalnum(c) && c != '_')
			return false;
		
	// KEYWORDS TO EXCLUDE
	return !isKeyword(s);
		
	return true;
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
        processError("multiple name definition");

    if (isKeyword(externalName))
        processError("illegal use of keyword");

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
			
storeTypes Compiler::whichType(string name)
{
    if (isInteger(name))
        return INTEGER;

    if (isBoolean(name))
        return BOOLEAN;

    auto it = symbolTable.find(name);

    if (it == symbolTable.end())
        processError("reference to undefined constant");

    return it->second.getDataType();
}

string Compiler::whichValue(string name)
{
    if (isInteger(name) || isBoolean(name))
        return name;

    auto it = symbolTable.find(name);

    if (it == symbolTable.end())
        processError("reference to undefined constant");

    return it->second.getValue();
}

void Compiler::code(string op, string operand1, string operand2)
{
    if (op == "program")
    {
        emitPrologue(operand1);
    }
    else if (op == "end")
    {
        emitEpilogue();
    }
    else
    {
        processError("compiler error since function code should not be called with illegal arguments");
    }
}

//////////////////// EMIT ////////////////////

void Compiler::emit(string label, string instruction, string operands,
		string comment)
{
    objectFile << std::left                      // left justification in objectFile
               << std::setw(8)  << label        // label field of width 8
               << std::setw(8)  << instruction  // instruction field of width 8
               << std::setw(24) << operands     // operands in field of width 24
               << comment                        // output comment
               << "\n";
}
		
void Compiler::emitPrologue(string progName, string)
{
    time_t now = time(0);
    string timeStr = ctime(&now);

    objectFile << "; " << progName << "\t" << timeStr;

    objectFile << "%INCLUDE \"Along32.inc\"\n";
    objectFile << "%INCLUDE \"Macros_Along.inc\"\n";

    emit("SECTION", ".text");
    emit("global", "_start", "", "; program " + progName);
    emit("_start:");
}

void Compiler::emitEpilogue(string, string)
{
    emit("", "Exit", "{0}");
    emitStorage();
}

void Compiler::emitStorage()
{
    emit("SECTION", ".data");

    for (const auto& pair : symbolTable)
    {
        const string& name = pair.first;
        const SymbolTableEntry& entry = pair.second;

        if (entry.getAlloc() == YES && entry.getMode() == CONSTANT)
        {
            string value = entry.getValue();

            if (value == "true") value = "-1";
            else if (value == "false") value = "0";
            else if (value == "") value = "0";

            emit(entry.getInternalName(), "dd", value, "; " + name);
        }
    }

    emit("SECTION", ".bss");

    for (const auto& pair : symbolTable)
    {
        const string& name = pair.first;
        const SymbolTableEntry& entry = pair.second;

        if (entry.getAlloc() == YES && entry.getMode() == VARIABLE)
        {
            emit(entry.getInternalName(), "resd",
                 to_string(entry.getUnits()), "; " + name);
        }
    }
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
        switch (ch)
        {
            case '{':
                do {
                    nextChar();
                } while (ch != '}' && ch != END_OF_FILE);

                if (ch == END_OF_FILE)
                    processError("unexpected end of file");

                nextChar();
                break;

            case '}':
                processError("'}' cannot begin token");
                break;

            default:
                if (isspace(ch))
                {
                    nextChar();
                }
                else if (isSpecialSymbol(ch))
                {
                    token = string(1, ch);
                    nextChar();
                }
                else if (islower(ch))
                {
                    token = string(1, ch);

                    while (true)
                    {
                        nextChar();

                        if (ch == END_OF_FILE)
                            processError("unexpected end of file");

                        if (isalnum(ch) || ch == '_')
                            token += ch;
                        else
                            break;
                    }
                }
                else if (isdigit(ch))
                {
                    token = string(1, ch);

                    while (true)
                    {
                        nextChar();

                        if (ch == END_OF_FILE)
                            processError("unexpected end of file");

                        if (isdigit(ch))
                            token += ch;
                        else
                            break;
                    }
                }
                else if (ch == END_OF_FILE)
                {
                    token = "EOF";
                    return token;
                }
                else
                {
                    processError("illegal symbol");
                }
        }
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
    listingFile << err << endl;
    errorCount++;
    exit(EXIT_FAILURE);
}

// ------------------------------------------------------------- //
// PRIVATE variables declared in stage0.h //
/*
map<string, SymbolTableEntry> symbolTable;
ifstream sourceFile;
ofstream listingFile;
ofstream objectFile;
string token;          // the next token
char ch;               // the next character of the source file
uint errorCount = 0;   // total number of errors encountered
uint lineNo = 0;       // line numbers for the listing
*/
// ------------------------------------------------------------- //

// To test the program:
/*
make stage0

DATA FILES:
ls /usr/local/4301/data/stage0/

cp /usr/local/4301/data/stage0/001.dat .
cat 001.dat
./stage0 001.dat 001.lst 001.asm

cat 001.lst
cat 001.asm

EDIT MAKEFILE:
targetsAsmLanguage = 001
./001
diff /usr/local/4301/data/stage0/001.lst 001.lst
diff /usr/local/4301/data/stage0/001.asm 001.asm
*/