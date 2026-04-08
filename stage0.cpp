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

// ------------------------------------------------------------- //
// GLOBAL VARIABLES
const char END_OF_FILE = '\0';
// ------------------------------------------------------------- //

class Compiler {
	
	// this function opens all files
	Compiler(char **argv) {					// constructor
		open sourceFile using argv[1];
		open listingFile using argv[2];
		open objectFile using argv[3];
	}
	
	// this function closes all files
	~Compiler(){							// destructor	// destructor
		// add error handling for if file already open
		close sourceFile using argv[1];
		close listingFile using argv[2];
		close objectFile using argv[3];
	}

// fill in using page 6 of OverallCompilerStructure.pdf for belief

//////// OUTPUT SECTION

	void createListingHeader(){
		return None;
	}
	
	void parser(){
		return None;
	}
	
	void createListingTrailer(){
		return None;
	}

// fill in using page 7 of OverallCompilerStructure.pdf

//////// GRAMMAR SECTION

	void prog(){           // stage 0, production 1
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
	
	void progStmt(){       // stage 0, production 2
		string x;
		
		if (token != "program")
			processError("keyword 'program' expected");
		
		x = nextToken();
		
		if (!isNonKeyId(x))
			processError("program name expected");
		
		if (nextToken() != ";")
			processError("semicolon expected");
		
		nextToken();
		
		code("program", x);
		insert(x, "PROG_NAME", "CONSTANT", x, "NO", 0);
	}
	
	void consts(){         // stage 0, production 3
		if (token != "const")
			processError("keyword 'const' expected");
		
		if (!isNonKeyId(nextToken)))
			processError("non-keyword identifier must follow 'const'");
		
		constStmts();
	}
	
	void vars(){           // stage 0, production 4
		if (token != "var")
			processError("keyword 'var' expected");
		
		if (!isNonKeyId(nextToken()))
			processError("non-keyword identifier must follow 'var'");
		
		varStmts();
	}
	
	void beginEndStmt(){   // stage 0, production 5
		if (token != "begin")
			processError("keyword 'begin' expected");
		
		if (nextToken() != "end")
			processError("keyword 'end' expected");
		
		if (nextToken() != ".")
			processError("period expected");
		
		nextToken();
		
		code("end", ".");
	}
	
	void constStmts(){     // stage 0, production 6
		cout << "Parsing const statements...\n";		// DEBUGGING
	}
	
	void varStmts(){       // stage 0, production 7
		cout << "Parsing var statements...\n";			// DEBUGGING
	}
	
	string ids(){          // stage 0, production 8
		return ids;
	}

//////// HELPER SECTION

	bool isKeyword(string s) const{  // determines if s is a keyword
		return true;
	}
	
	bool isSpecialSymbol(char c) const{
		// determines if c is a special symbol
		
		string symbols = "+-*/()=,;:.";
		return symbols.find(c) != string::npos;
	}
	
	bool isNonKeyId(string s) const{ // determines if s is a non_key_id
		if (s.empty() || !islower(s[0]))
			return false;
		
		for (char c : s)
			if (!isalnum(c) && c != '_')
				return false;
			
		// KEYWORDS TO EXCLUDE
		if (s == "program" || s == "const" || s == "var" ||
			s == "begin" || s == "end")
			return false;
			
		return true;
	}
	
	bool isInteger(string s) const{  // determines if s is an integer
		return true;
	}
	
	bool isBoolean(string s) const{  // determines if s is a boolean
		return true;
	}
	
	bool isLiteral(string s) const{  // determines if s is a literal
		return true;
	}

// fill in using page 12 of OverallCompilerStructure.pdf

//////// ACTION SECTION
	void insert(string externalName, storeTypes inType, modes inMode,
				string inValue, allocation inAlloc, int inUnits){
					cout << "INSERT: " << name << endl;		// for debugging
				}
				
	storeTypes whichType(string name){ // tells which data type a name has
		return name;
	}
	
	string whichValue(string name){ // tells which value a name has
		return name;
	}
	
	void code(string op, string operand1 = "", string operand2 = ""){
		cout << "CODE: " << op << " " << arg << endl;		// for debugging
	}

// fill in using page 14 of OverallCompilerStructure.pdf

//////// EMIT SECTION
	void emit(string label = "", string instruction = "", string operands = "",
			string comment = ""){
				return None;
			}
			
	void emitPrologue(string progName, string = ""){
		return None;
	}
	
	void emitEpilogue(string = "", string = ""){
		return None;
	}
	
	void emitStorage(){
		return None;
	}

// fill in using page 15 of OverallCompilerStructure.pdf, be detailed in printing

//////// LEXICON SECTION
	
	char nextChar(){			// gets raw chars
		if (!inFile.get(ch)){	// reads one char at a time from input
			ch = END_OF_FILE;	// stores char read in global var ch
		} else {
			cout << ch;			// outputs to console, change to listing file output
		}
		
		return ch;		// returns the next character or END_OF_FILE marker
	}
		
	string nextToken(){			// builds tokens out of raw chars
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
	  
//////// ERROR HANDLING SECTION

	string genInternalName(storeTypes stype) const{
		return stype;
	}
	
	void processError(string err){		// error handling
		cerr << "\nError: " << msg << endl;
		exit(1);
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

};