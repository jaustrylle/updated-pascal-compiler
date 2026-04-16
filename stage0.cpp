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

using namespace std;

/*
more work needed on emit functions
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
bool lexerInitialized = false;
bool begChar = true;

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
				
    // Listing header output to listingFile, not console; SOURCE STATEMENT begins in line 23
    listingFile << "STAGE0:\tAMIRAN FIELDS, SERENA REESE\t\t" << ctime(&now) << "\n\n";
    listingFile << std::left << "LINE NO.\t" << std::setw(23) << "SOURCE STATEMENT" << "\n\n";
	
	listingHeaderCreated = true;
}

void Compiler::parser()
{
	token = nextToken();
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
	if (token != "program")
	{
		processError("keyword 'program' expected");
	}
	
	token = nextToken();		// get program name
	
	if (!isNonKeyId(token))
	{
		processError("program name expected");
	}
	
	string progName = token;	// save program name
	
	token = nextToken();		// get semicolon
	
	if (token != ";")
	{
		processError("semicolon expected");
	}
		
	code("program", progName);
	insert(progName, PROG_NAME, CONSTANT, progName, NO, 0);
	
	token = nextToken();		// advance for next section
}

void Compiler::consts()         // stage 0, production 3
{	
	if (token != "const")
	{
		processError("keyword 'const' expected");
	}
	
	token = nextToken();
	
	if (!isNonKeyId(token))
	{
		processError("non-keyword identifier must follow 'const'");
	}
	
	constStmts();
}

void Compiler::vars()           // stage 0, production 4
{	
	if (token != "var")
	{
		processError("keyword 'var' expected");
	}
	
	token = nextToken();
	
	if (!isNonKeyId(token))
	{
		processError("non-keyword identifier must follow 'var'");
	}
	
	varStmts();
}

void Compiler::beginEndStmt()   // stage 0, production 5
{	
	if (token != "begin")
	{
		processError("keyword 'begin' expected");
	}
	
	token = nextToken();
	
	if (token != "end")
	{
		processError("keyword 'end' expected");
	}
	
	token = nextToken();
	
	if (token != ".")
	{
		processError("period expected");
	}
	
	token = nextToken();
	
	code("end", ".");
}

//////////////////// CONST ////////////////////

void Compiler::constStmts()     // stage 0, production 6
{
	string name = token;
	token = nextToken();
	
	if (token != "=")
	{
		processError("'=' expected");
	}

	token = nextToken();
	string value;

	if (token == "not")
	{
		token = nextToken();
		value = "not " + token;
	}
	else
	{
		value = token;
	}

	token = nextToken();
	
	if (token != ";")
	{
		processError("';' expected");
	}

	insert(name, whichType(value), CONSTANT, whichValue(value), YES, 1);

	token = nextToken();

	if (isNonKeyId(token))
	{
		constStmts();
	}
}

//////////////////// VAR ////////////////////

void Compiler::varStmts()       // stage 0, production 7
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

	// insert each id (simplified: assumes single for now)
	insert(x, (type == "integer" ? INTEGER : BOOLEAN),
		   VARIABLE, "", YES, 1);

	token = nextToken();

	if (isNonKeyId(token))
		varStmts();
}

string Compiler::ids()          // stage 0, production 8
{
	string id = token;

	if (!isNonKeyId(id))
		processError("identifier expected");

	token = nextToken();

	if (token == ",")
	{
		token = nextToken();
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
	if (symbolTable.count(externalName) > 0)
	{
		processError("multiple definition");
	}

	symbolTable.insert
	({
		externalName,
		SymbolTableEntry(externalName, inType, inMode, inValue, inAlloc, inUnits)
	});
}
			
storeTypes Compiler::whichType(string name) // tells which data type a name has
{
    if (isInteger(name)) return INTEGER;
    if (isBoolean(name)) return BOOLEAN;

    auto it = symbolTable.find(name);

    if (it == symbolTable.end())
    {
        processError("undefined identifier");
    }

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

void Compiler::code(string op, string operand1, string operand2)
{
    if(op == "program")
	{
        emitPrologue(operand1);
    }
	else if(op == "end")
	{
        emitEpilogue();
    }
	else
	{
        processError("compiler error: illegal arguments to code()");
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
		
void Compiler::emitPrologue(string progName, string s)
{
	// output identifying comments at beg of objectFile
	time_t now = time(0);
	
    string timeStr = ctime(&now);
    objectFile << "; SERENA REESE, AMIRAN FIELDS\t\t" << timeStr << "\n";

    // output %INCLUDE directives
    objectFile << "%INCLUDE \"Along32.inc\"\n";
    objectFile << "%INCLUDE \"Macros_Along.inc\"\n";

    objectFile << "\n"; // blank line
	
	// emit SECTION .text
    emit("SECTION", ".text");
	
	// emit global _start, program, progName
    emit("global", "_start", "", "; program " + progName);

    objectFile << "\n";	// blank line
	
	// emit _start:
    emit("_start:");
}

void Compiler::emitEpilogue(string x, string y)
{
	// emit Exit{0}
	emit("", "Exit", "{0}");
	
	objectFile << "\n";	// blank line
	
	// emitStorage
	emitStorage();
}

void Compiler::emitStorage()
{
	// emit SECTION .data for entries in symbolTable with YES and CONSTANT
	emit("SECTION", ".data");

    for(const auto& pair : symbolTable){
        const string& name = pair.first;
        const SymbolTableEntry& entry = pair.second;

        if (entry.getAlloc() == YES && entry.getMode() == CONSTANT){
            string value = entry.getValue();

            // Convert boolean constants
            if (value == "false" || value == "FALSE")
                value = "0";
            else if (value == "true" || value == "TRUE")
                value = "-1";
            else if (value.empty())
                value = "0";  // fallback

            emit(entry.getInternalName(), "dd", value, "; " + name);
        }
    }

	// call emit to output a line to objectFile 
    objectFile << "\n";

	// emit SECTION .bss for entries in symbolTable with YES and VARIABLE
    emit("SECTION", ".bss");

    for(const auto& pair : symbolTable){
        const string& name = pair.first;
        const SymbolTableEntry& entry = pair.second;

        if(entry.getAlloc() == YES && entry.getMode() == VARIABLE){
            emit(entry.getInternalName(), "resd", std::to_string(entry.getUnits()), "; " + name);
        }
    }
	
	// call emit to output line to objectFile
}

//////////////////// LEXICON ////////////////////

char Compiler::nextChar()			// gets raw chars
{
    char next;

    // Try to read next char
    if (!sourceFile.get(next)) {
        ch = END_OF_FILE;
        listingFile << "\n";
        return ch;
    }

    ch = next;

    // Print line number for the *first* line if nothing has printed yet
    if (listingHeaderCreated && begChar) {
        listingFile << std::right << std::setw(5) << lineNo << "|";
        begChar = false;
    }

    // Print the character
    if (listingHeaderCreated)
	{
		listingFile << ch;
	}
	
	if (ch == '\n')		// flag new line
	{
		lineNo++;
		begChar = true;
	}

    return ch;
}
	
string Compiler::nextToken()			// builds tokens out of raw chars
{
	if (!lexerInitialized)
	{
		lexerInitialized = true;
		nextChar();
	}
		
	token = "";
	
	while (token == ""){		// if ch not initialized, we lose chars
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
					token = string(1, ch);
					nextChar();
				} else if (islower(ch)){		// IDENTIFIER (starting with lowercase)
					token = string(1, ch);
					while (true){
						nextChar();
						if (isalnum(ch) || ch == '_')
							token += ch;
						else
							break;
					}
				} else if (isdigit(ch)){	// NUMBER
					token = string(1, ch);
					while (true){
						nextChar();
						if (isdigit(ch))
							token += ch;
						else
							break;
					}
				} else if (ch == END_OF_FILE){	// END OF FILE
					token = "EOF";
					return token;
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