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

using namespace std;

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
	// OUTPUT SECTION
	void createListingHeader();
	void parser();
	void createListingTrailer();

// fill in using page 7 of OverallCompilerStructure.pdf
	// GRAMMAR SECTION
	void prog();           // stage 0, production 1
	void progStmt();       // stage 0, production 2
	void consts();         // stage 0, production 3
	void vars();           // stage 0, production 4
	void beginEndStmt();   // stage 0, production 5
	void constStmts();     // stage 0, production 6
	void varStmts();       // stage 0, production 7
	string ids();          // stage 0, production 8

	// HELPER SECTION
	bool isKeyword(string s) const;  // determines if s is a keyword
	bool isSpecialSymbol(char c) const; // determines if c is a special symbol
	bool isNonKeyId(string s) const; // determines if s is a non_key_id
	bool isInteger(string s) const;  // determines if s is an integer
	bool isBoolean(string s) const;  // determines if s is a boolean
	bool isLiteral(string s) const;  // determines if s is a literal

// fill in using page 12 of OverallCompilerStructure.pdf
	// ACTION SECTION
	void insert(string externalName, storeTypes inType, modes inMode,
				string inValue, allocation inAlloc, int inUnits);
	storeTypes whichType(string name); // tells which data type a name has
	string whichValue(string name); // tells which value a name has
	void code(string op, string operand1 = "", string operand2 = "");

// fill in using page 14 of OverallCompilerStructure.pdf
	// EMIT SECTION
	void emit(string label = "", string instruction = "", string operands = "",
			string comment = "");
	void emitPrologue(string progName, string = "");
	void emitEpilogue(string = "", string = "");
	void emitStorage();

// fill in using page 15 of OverallCompilerStructure.pdf, be detailed in printing
	// LEXICON SECTION
	char nextChar(); // returns the next character or END_OF_FILE marker
	string nextToken(); // returns the next token or END_OF_FILE marker
	  
	// ERROR HANDLING SECTION
	string genInternalName(storeTypes stype) const;
	void processError(string err);

	// ------------------------------------------------------------- //
	// PRIVATE functions declared in stage0.h //
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

// To test the program, check out page 17 of OverallCompilerStructure.pdf
};