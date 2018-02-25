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
}

std::vector<Lexeme> Lexer::lex(){
  const char* marker;
  const char* token;
  
  const char* reader = this->mem_;
  int counter = 0;
  
  #define YYDEBUG(state, current)
  
  std::vector<Lexeme> lexemes;
  
  uint16_t line = 1;
  const char* line_start = reader;
  
  #define PLACE_LEXEME(params) \
    lexemes.emplace_back(params, std::make_pair(line, token - line_start));
  #define PUT_LEXEME(params) \
    lexemes.emplace_back(params, std::make_pair(line, token - line_start)); \
    continue;
  
  for(;;){
    token = reader;
    
    /*!re2c
      re2c:yyfill:enable = 0;
      re2c:define:YYCTYPE = char;
      re2c:define:YYCURSOR = reader;
      re2c:define:YYMARKER = marker;
      re2c:define:YYCTXMARKER = ctxmarker;
    */
    
    /*!re2c
      "\x00"  {break;}
      
      * {
        if(lexemes.back().type != LexemeType::Error){
          PUT_LEXEME(LexemeType::Error);
        } else continue;
      }
      
      //whitespace
      [ \t\r\v\f]         {continue;}
      
      "\n" {
        PLACE_LEXEME(LexemeType::Newline)
        ++line;
        line_start = reader;
        continue;
      }
      
      //comments
      "//" [^\x00\n]*     {continue;}
      "/""*"              {counter = 1; goto lex_block_comment;}
      
      //integer litterals
      [0-9]+              {PUT_LEXEME(strtoull(token, nullptr, 10))}
      '0'[bB][0-1]+       {PUT_LEXEME(strtoull(token + 2, nullptr, 2))}
      '0'[oO][0-7]+       {PUT_LEXEME(strtoull(token + 2, nullptr, 8))}
      '0'[xX][0-9a-fA-F]+ {PUT_LEXEME(strtoull(token + 2, nullptr, 16))}
      
      //float litterals
      dfrc = [0-9]* "." [0-9]+ | [0-9]+ ".";
      bfrc = [0-9a-fA-F]* "." [0-9a-fA-F]+ | [0-9a-fA-F]+ ".";
      dexp = 'e'|'E' [+-]? [0-9]+;
      bexp = 'p'|'P' [+-]? [0-9]+;
      dflt = (dfrc dexp? | [0-9]+ dexp);
      bflt = '0' [xX] (bfrc bexp? | [0-9a-fA-F]+ bexp);
      flt  = bflt | dflt;
      flt                 {PUT_LEXEME(atof(token))}
      
      //string litterals
      "\"" {
        goto lex_string;
      }
      
      //operators
      //"<-"  {PUT_LEXEME(LexemeType::Insert)}
      "+"   {PUT_LEXEME(LexemeType::Plus)}
      "-"   {PUT_LEXEME(LexemeType::Minus)}
      "*"   {PUT_LEXEME(LexemeType::Mul)}
      "/"   {PUT_LEXEME(LexemeType::Div)}
      "%"   {PUT_LEXEME(LexemeType::Mod)}
      "++"  {PUT_LEXEME(LexemeType::Append)}
      "="   {PUT_LEXEME(LexemeType::Assign)}
      "+="  {PUT_LEXEME(LexemeType::AddAssign)}
      "-="  {PUT_LEXEME(LexemeType::SubAssign)}
      "*="  {PUT_LEXEME(LexemeType::MulAssign)}
      "/="  {PUT_LEXEME(LexemeType::DivAssign)}
      "%="  {PUT_LEXEME(LexemeType::ModAssign)}
      "++=" {PUT_LEXEME(LexemeType::AppendAssign)}
      "=="  {PUT_LEXEME(LexemeType::Eq)}
      "!="  {PUT_LEXEME(LexemeType::Neq)}
      ">"   {PUT_LEXEME(LexemeType::Gt)}
      "<"   {PUT_LEXEME(LexemeType::Lt)}
      ">="  {PUT_LEXEME(LexemeType::Geq)}
      "<="  {PUT_LEXEME(LexemeType::Leq)}
      "<=>" {PUT_LEXEME(LexemeType::Cmp)}
      "("   {PUT_LEXEME(LexemeType::LParen)}
      ")"   {PUT_LEXEME(LexemeType::RParen)}
      "{"   {PUT_LEXEME(LexemeType::LBrace)}
      "}"   {PUT_LEXEME(LexemeType::RBrace)}
      "["   {PUT_LEXEME(LexemeType::LBracket)}
      "]"   {PUT_LEXEME(LexemeType::RBracket)}
      ","   {PUT_LEXEME(LexemeType::Comma)}
      //"@"   {PUT_LEXEME(LexemeType::Apply)}
      ";"   {PUT_LEXEME(LexemeType::Semicolon)}
      ":"   {PUT_LEXEME(LexemeType::Colon)}
      
      //keywords
      "var"   {PUT_LEXEME(LexemeType::Var)}
      "null"  {PUT_LEXEME(LexemeType::Null)}
      "true"  {PUT_LEXEME(true)}
      "false" {PUT_LEXEME(false)}
      "not"   {PUT_LEXEME(LexemeType::Not)}
      "and"   {PUT_LEXEME(LexemeType::And)}
      "or"    {PUT_LEXEME(LexemeType::Or)}
      "if"    {PUT_LEXEME(LexemeType::If)}
      "else"  {PUT_LEXEME(LexemeType::Else)}
      "while" {PUT_LEXEME(LexemeType::While)}
      "func"  {PUT_LEXEME(LexemeType::Func)}
      "print" {PUT_LEXEME(LexemeType::Print)}
      
      //identifiers
      [a-zA-Z_\x80-\xff][0-9a-zA-Z_\x80-\xff]* {
        StringView view(token, reader);
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
        lexemes.emplace_back(
          LexemeType::Identifier,
          str,
          std::make_pair(line, token - line_start)
        );
        
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
          PLACE_LEXEME(str);
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
        
        "\\" [0-7] {str.push_back(*(reader - 1)); goto lex_string_L1;}
        
        "\\" [0-3] [0-7]{2} {
          str.push_back(
            (*(reader - 3) * 0x40) |
            (*(reader - 2) * 0x8) |
            (*reader - 1)
          );
          goto lex_string_L1;
        }
        "\\x" [0-9a-fA-F]{2} {
          str.push_back((*(reader - 2) * 0x10) | (*reader - 1));
          goto lex_string_L1;
        }
        "\\u" [0-9a-fA-F]{4} {
          enterUtf8CodePoint_<4>(&str, reader);
          goto lex_string_L1;
        }
        "\\U" [0-9a-fA-F]{8} {
          enterUtf8CodePoint_<8>(&str, reader);
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
      "\n" "\n" {
        ++line;
        line_start = reader;
      }
    */
  }
  
  lexemes.emplace_back(LexemeType::End, std::make_pair(line, reader - line_start));
  
  #undef PLACE_LEXEME
  #undef PUT_LEXEME
  
  return lexemes;
}
