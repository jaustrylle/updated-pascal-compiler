// Amiran Fields, Serena Reese
// CS4301
// Stage 0

#include <stage0.h>
#include <stage0main.C>

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include <stdio.h>		// to close files
#include <cctype>		// nextChar, nextToken
#include <iomanip>
#include <ctime>		// time in listing header

using namespace std;

/*
April 1, 2026

Amiran
1. Lexicon (2)
2. Constructor
3. Destructor
4. Output (3)

Serena
1. Error handling (2)
2. Grammar (8)

To-do
1. Helper (6)
2. Action (4)
3. Emit (4)

April 7, 2026

Serena
1. nextChar()
2. nextToken()
3. processError()
4. isSpecialSymbol()
5. isNonKeyId
6. prog()
7. progStmt()
8. consts()
9. vars()
10. beginEndStmt()
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
	
Compiler::Compiler(char **argv)		// constructor
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

// this function closes all files
Compiler::~Compiler()							// destructor
{
	// add error handling for if file already open
	sourceFile.close();
	listingFile.close();
	objectFile.close();
}

//////////////////// OUTPUT ////////////////////

void Compiler::Compiler::createListingHeader()
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

void Compiler::prog()           // stage 0, production 1
{
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
	string x = nextToken();

	if (token != "program")
		processError("keyword 'program' expected");
	
	if (!isNonKeyId(x))
		processError("program name expected");
	
	if (nextToken() != ";")
		processError("semicolon expected");
	
	nextToken();
	
	code("program", x);
	insert(x, "PROG_NAME", "CONSTANT", x, "NO", 0);
}

/*
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
*/

void Compiler::consts()         // stage 0, production 3
{
	string x = nextToken();
	
	if (token != "const")
	{
		processError("keyword 'const' expected");
	}
	
	if (!isNonKeyId(nextToken)))
	{
		processError("non-keyword identifier must follow 'const'");
	}
	
	constStmts();
}

void Compiler::vars()           // stage 0, production 4
{
	string x = nextToken();
	
	if (token != "var")
	{
		processError("keyword 'var' expected");
	}
	
	if (!isNonKeyId(nextToken()))
	{
		processError("non-keyword identifier must follow 'var'");
	}
	
	varStmts();
}

void Compiler::beginEndStmt()   // stage 0, production 5
{
	string x = nextToken();
	
	if (token != "begin")
	{
		processError("keyword 'begin' expected");
	}
	
	if (nextToken() != "end")
	{
		processError("keyword 'end' expected");
	}
	
	if (nextToken() != ".")
	{
		processError("period expected");
	}
	
	nextToken();
	
	code("end", ".");
}

//////////////////// CONST ////////////////////

void Compiler::constStmts()     // stage 0, production 6
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

void Compiler::varStmts()       // stage 0, production 7
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

string Compiler::ids()          // stage 0, production 8
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
	for (char c : s)
		if (!isdigit(c)) return false;
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

void Compiler::insert(string externalName, storeTypes inType, modes inMode,
			string inValue, allocation inAlloc, int inUnits)
{
	if (symbolTable.count(name) > 0)
		processError("multiple definition");

	symbolTable.insert({
		name,
		SymbolTableEntry(name, type, mode, value, alloc, units)
	});
}
			
storeTypes Compiler::whichType(string name) // tells which data type a name has
{
	if (isInteger(name)) return INTEGER;
	if (isBoolean(name)) return BOOLEAN;

	auto it = symbolTable.find(name);

	if (it == symbolTable.end())
		processError("undefined identifier");

	return it->second.getDataType();
}

string Compiler::whichValue(string name) // tells which value a name has
{
	if (isInteger(name) || isBoolean(name))
		return name;

	auto it = symbolTable.find(name);

	if (it == symbolTable.end())
		processError("undefined identifier");

	return it->second.getValue();
}

void Compiler::code(string op, string operand1 = "", string operand2 = "")
{
	cout << "CODE: " << op << " " << arg << endl;		// for debugging
}

// fill in using page 14 of OverallCompilerStructure.pdf

//////////////////// EMIT ////////////////////

void Compiler::emit(string label = "", string instruction = "", string operands = "",
		string comment = "")
{
	return None;
}
		
void Compiler::emitPrologue(string progName, string = "")
{
	return None;
}

void Compiler::emitEpilogue(string = "", string = "")
{
	return None;
}

void Compiler::emitStorage()
{
	return None;
}

//////////////////// LEXICON ////////////////////

char Compiler::nextChar()			// gets raw chars
{
	if (!sourceFile.get(ch)){	// reads one char at a time from input
		ch = END_OF_FILE;	// stores char read in global var ch
	} else {
		listingFile << ch;
		
		if (ch == '\n')
		{
			lineNo++;
			listingFile << setw(5) << lineNo << "|";
		}
	}
	
	return ch;		// returns the next character or END_OF_FILE marker
}
	
string Compiler::nextToken()			// builds tokens out of raw chars
{
	// groups chars into tokens (words, nums, symbols)
	token = "";
	
	while (token == ""){
		switch (ch){		// stores result in token
			case '{':		// COMMENT {...}
				do {
					nextChar();
				} while (ch != '}' && ch != END_OF_FILE);
				if (ch == END_OF_FILE)
					processError("Unexpected end of file inside comment");
				
				nextChar();		// move past the '}'
				break;
				
			case '}':		// INVALID '}'
				processError("'}' cannot begin a token");
				break;
				
			default:		// WHITESPACE
				if (isspace(ch)){
					nextChar();
				} else if (isSpecialSymbol(ch)){	// SPECIAL SYMBOLS
					token = ch;
					nextChar();
				} else if (islower(ch)){		// IDENTIFIER (starting with lowercase)
					token = ch;
					while (true){
						nextChar();
						if (isalnum(ch) || ch == '_')
							token += ch;
						else
							break;
					}
				} else if (isdigit(ch)){	// NUMBER
					token = ch;
					while (true){
						nextChar();
						if (isdigit(ch))
							token += ch;
						else
							break;
					}
				} else if (ch == END_OF_FILE){	// END OF FILE
					token = "EOF";
				} else {						// ILLEGAL SYMBOL
					processError("Illegal symbol");
				}
			}
		}
	return token;		// returns the next token or END_OF_FILE marker
}
  
//////////////////// ERROR ////////////////////

string Compiler::genInternalName(storeTypes stype) const
{
	return stype;
}

void Compiler::processError(string err)		// error handling
{
	listingFile << "\nError: " << err << endl;
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