#include<iostream>
#include<unordered_map>
#include<string>
#include<bitset>
#include<boost/regex.hpp>
#include<stdexcept>
#include<fstream>

#include"appLib/cast.h"
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
  { "AMD", "111" }
};

 return dest_mappings.at(to_read_dest.destination);
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
  { "D&A", "1000000" },
  { "D|A", "0010101" },
  { "D|M", "1010101" }
};

 return comp_mappings.at(to_read_computation.computation);
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

 return jump.at(to_read_jump.jump);
}

string comment_notation = R"( *((//).*)?)"; //TODO: consider adding ?:
regex address_pattern{R"(@([0-9]+))" + comment_notation};
//The regex is barely readable, think about how to simplify
regex computation_pattern{R"((?:([AMD]+)=)*([-+01!&|A-Z]+)(?:;([A-Z]+))*)" + comment_notation};
regex comment_pattern{comment_notation};

//TODO: perhaps this isn't cohesive, presumably this is just a tokenizer.
//however it also translates address into binary.
bool to_binary_address(
const string& possible_address, 
string& binary_address){

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
  return regex_match(possible_comment, comment_pattern);
}

struct failed_parse{};

vector<string> to_vector(istream& input_stream, char delim = '\n'){
  vector<string> vectored_input_stream;

  string current_line;
  while( getline(input_stream, current_line, delim) )
    vectored_input_stream.push_back(current_line);

  return vectored_input_stream; //NOTE: c++11 feature.
}

void assembler(istream& input_stream, ostream& output_stream){
  vector<string> program = to_vector(input_stream);
  for(const string& current_line : program){
    string address;
    computation command;
    if(to_binary_address(current_line, address)){
      output_stream << "0" << address << '\n';
    }
    else if(to_computation(current_line, command)){
      output_stream << "111" 
	            << comp(command)
		    << dest(command) 
		    << jump(command) 
                    << '\n';
    }
    else if(is_comment(current_line)){
      //Don't do anything
    }
    else{
      failed_parse{}; 
    }
  }
}

}

int main(){
  string from, to;
  cin >> from >> to;
  ifstream is{from};
  ofstream os{to};
  assembler(is, os);
}

//add.asm add.hack
