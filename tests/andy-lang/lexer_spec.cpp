#include <andy/tests.hpp>
#include <andy/lang/lexer.hpp>

describe of("lexer", []() {
  describe("tokenization", []() {
    describe("delimiters", []() {
      describe("delimiter tokens", []() {
        it("should have andy::lang::lexer::token_type::token_delimiter", [&]() {
          for(unsigned char c = 0; c < 255; c++) {
            switch(c)
            {
              case '{':
              case '}':
              case '(':
              case ')':
              case ';':
              case ',':
              case ':':
              {
                andy::lang::lexer l("", std::string(1, c));
                expect(l.tokens().front().type()).to<eq>(andy::lang::lexer::token_type::token_delimiter);
              }
              break;
              default:
              break;
            }
          }
        });
      });
      describe("non delimiter tokens", []() {
        it("should have andy::lang::lexer::token_type::token_delimiter", [&]() {
          for(unsigned char c = 0; c < 255; c++) {
            switch(c)
            {
              case '{':
              case '}':
              case '(':
              case ')':
              case ';':
              case ',':
              case ':':
              break;
              case '"':
              case '\'':
              continue;
              default:
              {
                andy::lang::lexer l("", std::string(1, c));
                expect(l.tokens().front().type()).to_not<eq>(andy::lang::lexer::token_type::token_delimiter);
              }
              break;
            }
          }
        });
      });
    });
    describe("operators", []() {
      describe("operators tokens", []() {
        it("should have andy::lang::lexer::token_type::token_operator", [&]() {
          for(unsigned char c = 0; c < 255; c++) {
            switch(c)
            {
              case '+':
              case '-':
              case '*':
              case '/':
              case '%':
              case '=':
              case '!':
              case '?':
              case '<':
              case '>':
              case '[':
              case ']':
              case '|':
              case '&':
              case '.': {
                andy::lang::lexer l("", std::string(1, c));
                expect(l.tokens().front().type()).to<eq>(andy::lang::lexer::token_type::token_operator);
              }
              break;
              case '"':
              case '\'':
              continue;
              default:
              break;
            }
          }
        });
      });
      describe("non delimiter tokens", []() {
        it("should not have andy::lang::lexer::token_type::token_delimiter", [&]() {
          for(unsigned char c = 0; c < 255; c++) {
            switch(c)
            {
              case '+':
              case '-':
              case '*':
              case '/':
              case '%':
              case '=':
              case '!':
              case '?':
              case '<':
              case '>':
              case '[':
              case ']':
              case '|':
              case '&':
              case '.':
              case '{':
              break;
              case '"':
              case '\'':
              continue;
              default:
              {
                andy::lang::lexer l("", std::string(1, c));
                expect(l.tokens().front().type()).to_not<eq>(andy::lang::lexer::token_type::token_operator);
              }
              break;
            }
          }
        });
      });
    });
  });
});