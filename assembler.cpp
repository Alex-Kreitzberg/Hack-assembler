#include<iostream>
#include<unordered_map>
#include<string>
#include<bitset>
#include<boost/regex.hpp>
#include<stdexcept>
#include<fstream>
#include<algorithm>

#include "appLib/algorithm.h"
#include "appLib/cast.h"
//g++ assembler.cpp -o assembler -lboost_regex
using namespace std;
using namespace boost;
namespace{

struct computation{
  string destination;
  string computation;
  string jump;
};

string dest(computation to_read_dest){

  unordered_map< string, string > 
  dest_mappings{ 
    { "", "000" },
    { "M", "001" },
    { "D", "010" },
    { "MD", "011" },
    { "A","100" },
    { "AM", "101" },
    { "AD", "110"},
    { "AMD", "111" }
  }; 
  string dest_try;
  try{
    dest_try = dest_mappings.at(to_read_dest.destination);
  }catch(out_of_range& not_valid_command){
    cerr << "dest failed with: " << to_read_dest.destination;
    throw;
  }
  return dest_try; 
}

string comp(computation to_read_computation){

  unordered_map< string, string > 
  comp_mappings{
    { "0" , "0101010"},
    { "1", "0111111"},
    { "-1", "0111010"},
    { "D", "0001100" },
    { "A", "0110000" },
    { "M", "1110000" },
    { "!D", "0001101" },
    { "!A", "0110001" },
    { "!M", "1110001" },
    { "-D", "0001111" },
    { "-A", "0110011" },
    { "-M", "1110011" },
    { "D+1", "0011111" },
    { "A+1", "0110111" },
    { "M+1", "1110111" },
    { "D-1", "0001110" },
    { "A-1", "0110010" },
    { "M-1", "1110010" },
    { "D+A", "0000010" },
    { "D+M", "1000010" },
    { "D-A", "0010011" },
    { "D-M", "1010011" },
    { "A-D", "0000111" },
    { "M-D", "1000111" },
    { "D&A", "0000000" },
    { "D&M", "1000000" },
    { "D|A", "0010101" },
    { "D|M", "1010101" }
  };
  string comp_try;
  try{
    comp_try = comp_mappings.at(to_read_computation.computation);
  }catch(out_of_range& not_valid_command){
    cerr << "comp failed with: " << to_read_computation.computation;
    throw;
  }
  return comp_try;
}

string jump(computation to_read_jump){

  unordered_map< string, string > 
  jump{
    { "", "000" },
    { "JGT", "001" },
    { "JEQ", "010" },
    { "JGE", "011" },
    { "JLT", "100" },
    { "JNE", "101" },
    { "JLE", "110" },
    { "JMP", "111" }
  };
  string jump_try;
  try{
    jump_try = jump.at(to_read_jump.jump);
  }catch(out_of_range& not_valid_command){
    cerr << "jump failed with: " << to_read_jump.jump;
    throw;
  }
  //  return jump.at(to_read_jump.jump);
  return jump_try;
}

unordered_map< string, string  > symbol_table{
  { "SP", "0" },
  { "LCL", "1" },
  { "ARG", "2" },
  { "THIS", "3" },
  { "THAT", "4" },
  { "R0", "0" },
  { "R1", "1" },
  { "R2", "2" },
  { "R3", "3" },
  { "R4", "4" },
  { "R5", "5" },
  { "R6", "6" },
  { "R7", "7" },
  { "R8", "8" },
  { "R9", "9" },
  { "R10", "10" },
  { "R11", "11" },
  { "R12", "12" },
  { "R13", "13" },
  { "R14", "14" },
  { "R15", "15" },
  { "SCREEN", "16384" },
  { "KBD", "24576" }
};
const unsigned int FIRST_FREE_MEMORY_ADDRESS = 16;

const string comment_notation = R"(\s*((//).*)?)";

//TODO: perhaps this isn't cohesive, presumably this is just a tokenizer.
//however it also translates address into binary.
bool to_binary_address(
  const string& possible_address, 
  string& binary_address){

  regex address_pattern{"\\s*@([0-9]+)" + comment_notation};

  smatch matches;
  bool is_address 
    = regex_match(possible_address, matches, address_pattern);
  if(is_address){
    auto address_value = 
      appLib::to<unsigned long long>(matches[1].str());
    binary_address = bitset<15>{ address_value }.to_string();
  }
  return is_address;
}

bool to_computation(
  const string& possible_computation, 
  computation& computation_token ){

  regex computation_pattern{
    "\\s*"  //arbitrary spaces in front.
    "(?:([AMD]+)=)?"  //AMD= may be omitted
    "([-+01!&|A-Z]+)"
    "(?:;([A-Z]+))?"  //;JMP may be omitted
    + comment_notation
  };

  smatch matches;
  bool is_computation 
    = regex_match(possible_computation, matches, computation_pattern);
  if(is_computation){
    string destination = matches[1];
    string computation = matches[2];
    string jump = matches[3];
    computation_token = {destination, computation, jump};
  }
  return is_computation;
}

bool is_comment(const string& possible_comment){
  regex comment_pattern{comment_notation};
  return regex_match(possible_comment, comment_pattern);
}

const string name_notation{"[^0-9][$A-Z:a-z.0-9_]*"};
bool to_label(const string& possible_label, string& label){
  //Error: "not digit" block breaks other requirements
  //regex specifies patterns like (LOOP) where the token value is LOOP.
  //TODO: does the label have white space at the beginnings?
  regex label_pattern{"\\(" "("+name_notation+")" "\\)" + comment_notation}; 
  smatch matches;
  bool is_label =
    regex_match(possible_label, matches, label_pattern);
  if(is_label)
    label = matches[1];

  return is_label;
}
//TODO: the following "is"s are abstracted funny, rewrite to in terms of them
//rather then the other way around.
bool is_label(const string& possible_label){
  string dummy{};
  return to_label(possible_label, dummy);
}
//TODO: THIS MUST ACCEPT TABS
bool to_variable(const string& possible_variable, string& variable){
  regex variable_pattern{"\\s*@("+name_notation+")"+comment_notation};
  smatch matches;
  bool is_variable =
    regex_match(possible_variable, matches, variable_pattern);
  if(is_variable)
    variable = matches[1];

  return is_variable;
} 
bool is_variable(const string& possible_variable){
  string dummy{};
  return to_variable(possible_variable, dummy);
}
vector<string> to_vector(istream& input_stream, string delim = "\n"){
  string entire_stream{ 
    istreambuf_iterator<char>{input_stream},
    istreambuf_iterator<char>{}
  };
  vector<string> vectored_input_stream{
    sregex_token_iterator{
      begin(entire_stream),
      end(entire_stream),
      regex{delim},
      -1
    },
    sregex_token_iterator{}
  };
  return vectored_input_stream; //NOTE: c++11 feature.
}

void display(const vector<string>& to_display){
  for(auto& line : to_display)
    cout << line << '\n';
}
void display(const unordered_map<string,string>& to_display){
  for(auto& p : to_display)
    cout << "{" << p.first << "," << p.second << "}\n";
}

struct multiple_labels_with_same_name{};
void replace_symbols(vector<string>& program){
  //TODO: separate remove and erase idiom into header file.
  //strip comments and empty lines

  appLib::remove_if(program,
    is_comment
  );

  //build symbol table for labels.
  unsigned int line_number = 0;
  for(auto& line : program){
    string label;
    if(to_label(line, label)){
      symbol_table[label] = appLib::to<string>(line_number);
      --line_number;
    }
    //multiple labels with the same name aren't errors.
    //TODO: maybe they should be...

     ++line_number;                           
  }

  //remove labels
  appLib::remove_if(program,
    is_label
  );

  //associate variables with memory addresses.
  //if variable is already defined assign it to the line
  //TODO: cohesion problem?
  unsigned int memory_address = FIRST_FREE_MEMORY_ADDRESS; 
  for(string& line : program){
    string variable;
    if(to_variable(line, variable)){
      if(!symbol_table.count(variable))
	symbol_table[variable] = appLib::to<string>(memory_address++);
      line = "@"+symbol_table[variable];
    }
  }

}

struct failed_parse{};

void assembler(istream& input_stream, ostream& output_stream){
  vector<string> program = to_vector(input_stream);

  //TODO: First pass through. replace variables with unique addresses
  replace_symbols(program);  

  vector<string> assembly{};
  for(const string& current_line : program){
    string address;
    computation command;
    if(to_binary_address(current_line, address)){
      //TODO: think about how to encapsulate "Magic literals" like 0
      assembly.emplace_back(
        "0" + 
        address
      );
    }
    else if(to_computation(current_line, command)){
      assembly.emplace_back(
        "111" + 
	comp(command) +
	dest(command) +
	jump(command)
      );
    }
    else if(is_comment(current_line)){
      //Don't do anything
    }
    else{
      cout << "the failed line is: " << current_line << '\n';
      display(symbol_table);
      cout << '\n';
      throw failed_parse{}; 
    }
  }
  ostream_iterator<string> assembly_output{
    output_stream,
    "\n"
  };
  copy(
    begin(assembly),
    end(assembly),
    assembly_output
  );
}

}

int main(){
  string from, to;
  cin >> from >> to;
  ifstream is{from};
  ofstream os{to};
  assembler(is, os);
}

//Max.asm Max.hack
