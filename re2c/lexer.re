#include "lexer.h"

#include <cstdint>

#include <iostream>

namespace{
  
  constexpr uint64_t utf8Mask_(int bytes){
    if(bytes >= 2){
      int mask1 = 0x8000, mask2 = 0x80;
      for(; bytes > 2; --bytes){
        mask1 = (mask1 << 8) | (mask1 << 7);
        mask2 = (mask2 << 8) | 0x80;
      }
      return mask1 | mask2;
    }else return 0;
  }
  
  template<int bytes>
  inline uint64_t splitUtf8CodePoint_(uint64_t code_point){
    uint64_t mask = 0x3f;
    for(int i = 1; i < bytes; ++i){
      code_point = (code_point & mask) | ((code_point & ~mask) << 2);
      mask = (mask << 8) | 0xff;
    }
    return code_point;
  }
  
  template<int chars>
  inline void enterUtf8CodePoint_(std::string* dest, const char* code){
    uint64_t code_point;
    code -= chars;
    for(int i = 0; i < chars; ++i){
      code_point <<= 8;
      if(isdigit(*code)){
        code_point |= (*code) - '0';
      }else{
        code_point |= (*code | 0x20) - 'a' + 10;
      }
      ++code;
    }
    if(code_point < 0x80){
      dest->push_back((char)code_point);
      return;
    }else{
      int bytes = 2;
      for(uint64_t cp = code_point >> 11; cp > 0; ++bytes, cp >> 5);
      
      switch(bytes){
      case 2:
        code_point = splitUtf8CodePoint_<2>(code_point) | utf8Mask_(2);
        goto write_2_bytes;
      case 3:
        code_point = splitUtf8CodePoint_<3>(code_point) | utf8Mask_(3);
        goto write_3_bytes;
      case 4:
        code_point = splitUtf8CodePoint_<4>(code_point) | utf8Mask_(4);
        goto write_4_bytes;
      case 5:
        code_point = splitUtf8CodePoint_<5>(code_point) | utf8Mask_(5);
        goto write_5_bytes;
      case 6:
        code_point = splitUtf8CodePoint_<6>(code_point) | utf8Mask_(6);
        goto write_6_bytes;
      case 7:
        code_point = splitUtf8CodePoint_<7>(code_point) | utf8Mask_(7);
        goto write_7_bytes;
      }
      
    write_7_bytes:
      dest->push_back(code[-7]);
    write_6_bytes:
      dest->push_back(code[-6]);
    write_5_bytes:
      dest->push_back(code[-5]);
    write_4_bytes:
      dest->push_back(code[-4]);
    write_3_bytes:
      dest->push_back(code[-3]);
    write_2_bytes:
      dest->push_back(code[-2]);
      dest->push_back(code[-1]);
    }
  }
}

Lexer::Lexer(const char* mem){
  this->mem_ = mem;
  this->old_reader_ = nullptr;
  this->reader_ = mem;
}

std::vector<Lexeme> Lexer::lex(){
  const char* marker;
  const char* token;
  
  int counter = 0;
  
  this->old_reader_ = this->reader_;
  
  #define YYDEBUG(state, current)
  
  std::vector<Lexeme> lexemes;
  
  #define NEW_LEXEME(params) \
    lexemes.emplace_back(params); \
    continue;
  
  for(;;){
    token = this->reader_;
    
    /*!re2c
      re2c:yyfill:enable = 0;
      re2c:define:YYCTYPE = char;
      re2c:define:YYCURSOR = this->reader_;
      re2c:define:YYMARKER = marker;
      re2c:define:YYCTXMARKER = ctxmarker;
    */
    
    /*!re2c
      "\x00"  {break;}
      
      *       {break;}
      
      //whitespace
      [ \t\r\v\f]         {continue;}
      "\n"                {NEW_LEXEME(LexemeType::Newline)}
      
      //comments
      "//" [^\x00\n]*     {continue;}
      "/""*"              {counter = 1; goto lex_block_comment;}
      
      //integer litterals
      [0-9]+              {NEW_LEXEME(strtoull(token, nullptr, 10))}
      '0'[bB][0-1]+       {NEW_LEXEME(strtoull(token + 2, nullptr, 2))}
      '0'[oO][0-7]+       {NEW_LEXEME(strtoull(token + 2, nullptr, 8))}
      '0'[xX][0-9a-fA-F]+ {NEW_LEXEME(strtoull(token + 2, nullptr, 16))}
      
      //float litterals
      dfrc = [0-9]* "." [0-9]+ | [0-9]+ ".";
      bfrc = [0-9a-fA-F]* "." [0-9a-fA-F]+ | [0-9a-fA-F]+ ".";
      dexp = 'e'|'E' [+-]? [0-9]+;
      bexp = 'p'|'P' [+-]? [0-9]+;
      dflt = (dfrc dexp? | [0-9]+ dexp);
      bflt = '0' [xX] (bfrc bexp? | [0-9a-fA-F]+ bexp);
      flt  = bflt | dflt;
      flt                 {NEW_LEXEME(atof(token))}
      
      //string litterals
      "\"" {
        goto lex_string;
      }
      
      //operators
      //"<-"  {NEW_LEXEME(LexemeType::Insert)}
      "+"   {NEW_LEXEME(LexemeType::Plus)}
      "-"   {NEW_LEXEME(LexemeType::Minus)}
      "*"   {NEW_LEXEME(LexemeType::Mul)}
      "/"   {NEW_LEXEME(LexemeType::Div)}
      "%"   {NEW_LEXEME(LexemeType::Mod)}
      "++"  {NEW_LEXEME(LexemeType::Append)}
      "="   {NEW_LEXEME(LexemeType::Assign)}
      "+="  {NEW_LEXEME(LexemeType::AddAssign)}
      "-="  {NEW_LEXEME(LexemeType::SubAssign)}
      "*="  {NEW_LEXEME(LexemeType::MulAssign)}
      "/="  {NEW_LEXEME(LexemeType::DivAssign)}
      "%="  {NEW_LEXEME(LexemeType::ModAssign)}
      "++=" {NEW_LEXEME(LexemeType::AppendAssign)}
      "=="  {NEW_LEXEME(LexemeType::Eq)}
      "!="  {NEW_LEXEME(LexemeType::Neq)}
      ">"   {NEW_LEXEME(LexemeType::Gt)}
      "<"   {NEW_LEXEME(LexemeType::Lt)}
      ">="  {NEW_LEXEME(LexemeType::Geq)}
      "<="  {NEW_LEXEME(LexemeType::Leq)}
      "<=>" {NEW_LEXEME(LexemeType::Cmp)}
      "("   {NEW_LEXEME(LexemeType::LParen)}
      ")"   {NEW_LEXEME(LexemeType::RParen)}
      "{"   {NEW_LEXEME(LexemeType::LBrace)}
      "}"   {NEW_LEXEME(LexemeType::RBrace)}
      "["   {NEW_LEXEME(LexemeType::LBracket)}
      "]"   {NEW_LEXEME(LexemeType::RBracket)}
      ","   {NEW_LEXEME(LexemeType::Comma)}
      //"@"   {NEW_LEXEME(LexemeType::Apply)}
      ";"   {NEW_LEXEME(LexemeType::Semicolon)}
      ":"   {NEW_LEXEME(LexemeType::Colon)}
      
      //keywords
      "var"   {NEW_LEXEME(LexemeType::Var)}
      "null"  {NEW_LEXEME(LexemeType::Null)}
      "true"  {NEW_LEXEME(true)}
      "false" {NEW_LEXEME(false)}
      "not"   {NEW_LEXEME(LexemeType::Not)}
      "and"   {NEW_LEXEME(LexemeType::And)}
      "or"    {NEW_LEXEME(LexemeType::Or)}
      "if"    {NEW_LEXEME(LexemeType::If)}
      "else"  {NEW_LEXEME(LexemeType::Else)}
      "while" {NEW_LEXEME(LexemeType::While)}
      "func"  {NEW_LEXEME(LexemeType::Func)}
      "print" {NEW_LEXEME(LexemeType::Print)}
      
      //identifiers
      [a-zA-Z_\x80-\xff][0-9a-zA-Z_\x80-\xff]* {
        StringView view(token, this->reader_);
        String* str;
        auto it = this->string_table.find(view);
        if(it == this->string_table.end()){
          str = this->string_table.insert(
            std::make_pair(
              view,
              rc_ptr<String>(make_new<String>(view.first, view.second))
            )
          ).first->second.get();
        }else{
          str = it->second.get();
        }
        lexemes.emplace_back(LexemeType::Identifier, str);
        continue;
      }
      
      //parameters
      //"$" [1-9][0-9]* {
      //  return new Token(TokenType::ParamToken, strtoull(token + 1, nullptr, 10));
      //}
    */
  
  lex_string: {
      std::string str;
      
    lex_string_L1:
      /*!re2c
        *       {break;}
        "\x00"  {break;}
        
        [^\n\\"] {
          str.push_back(yych);
          goto lex_string_L1;
        }
        
        "\"" {
          StringView view(str.c_str(), str.size());
          String* str;
          auto it = this->string_table.find(view);
          if(it == this->string_table.end()){
            str = this->string_table.insert(
              std::make_pair(
                view,
                rc_ptr<String>(make_new<String>(view.first, view.second))
              )
            ).first->second.get();
          }else{
            str = it->second.get();
          }
          lexemes.emplace_back(str);
          continue;
        }
        
        "\\a"   {str.push_back('\a'); goto lex_string_L1;}
        "\\b"   {str.push_back('\b'); goto lex_string_L1;}
        "\\f"   {str.push_back('\f'); goto lex_string_L1;}
        "\\n"   {str.push_back('\n'); goto lex_string_L1;}
        "\\r"   {str.push_back('\r'); goto lex_string_L1;}
        "\\t"   {str.push_back('\t'); goto lex_string_L1;}
        "\\v"   {str.push_back('\v'); goto lex_string_L1;}
        "\\\\"  {str.push_back('\\'); goto lex_string_L1;}
        "\\?"   {str.push_back('\?'); goto lex_string_L1;}
        "\\'"   {str.push_back('\''); goto lex_string_L1;}
        "\\\""  {str.push_back('\"'); goto lex_string_L1;}
        "\\\n"  {goto lex_string_L1;}
        
        "\\" [0-7] {str.push_back(*(this->reader_ - 1)); goto lex_string_L1;}
        
        "\\" [0-3] [0-7]{2} {
          str.push_back(
            (*(this->reader_ - 3) * 0x40) |
            (*(this->reader_ - 2) * 0x8) |
            (*this->reader_ - 1)
          );
          goto lex_string_L1;
        }
        "\\x" [0-9a-fA-F]{2} {
          str.push_back((*(this->reader_ - 2) * 0x10) | (*this->reader_ - 1));
          goto lex_string_L1;
        }
        "\\u" [0-9a-fA-F]{4} {
          enterUtf8CodePoint_<4>(&str, this->reader_);
          goto lex_string_L1;
        }
        "\\U" [0-9a-fA-F]{8} {
          enterUtf8CodePoint_<8>(&str, this->reader_);
          goto lex_string_L1;
        }
      */
    }
    
  lex_block_comment:
    /*!re2c
      "\x00"  {break;}
      *       {goto lex_block_comment;}
      "/""*"  {++counter; goto lex_block_comment;}
      "*""/"  {if(--counter) goto lex_block_comment; else continue;}
    */
  }
  
  #undef NEW_LEXEME
  
  lexemes.emplace_back(LexemeType::End);
  
  for(auto lex: lexemes){
    std::cout << lex.toStr() << std::endl;
  }
  
  return lexemes;
}

Token* Lexer::next(){
  return nullptr;
}

void Lexer::setCheckpoint(){
  this->_checkpoints.push_back(this->old_reader_);
}

void Lexer::removeCheckpoint(){
  this->_checkpoints.pop_back();
}

void Lexer::revert(){
  this->reader_ = this->_checkpoints.back();
  this->_checkpoints.pop_back();
}