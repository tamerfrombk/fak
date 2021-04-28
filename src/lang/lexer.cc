#include "fak/lang/token.h"
#include <cctype>
#include <unordered_map>

#include <fak/log.h>
#include <fak/lang/lexer.h>
#include <fak/lang/error_handler.h>

static const std::unordered_map<std::string, fk::lang::token_type> KEYWORDS = {
      { "true", fk::lang::token_type::BTRUE }
    , { "false", fk::lang::token_type::BFALSE }
    , { "nil", fk::lang::token_type::NIL }
    , { "print", fk::lang::token_type::PRINT }
};

fk::lang::lexer_t::lexer_t(std::string text, fk::lang::error_handler_t *error_handler)
    : text_(std::move(text))
    , cursor_(0)
    , error_handler_(error_handler)
{}

fk::lang::token_t fk::lang::lexer_t::next_token() noexcept
{
    skip_whitespace();

    if (is_eof()) {
        debug("EOF reached\n");
        return { "", fk::lang::token_type::T_EOF };
    }

    char c = advance();
    if (std::isalpha(c)) {
        return lex_alnum(c);
    } else if (std::isdigit(c)) {
        return lex_number(c);  
    } else if (c == '+') {
        return { "+", fk::lang::token_type::PLUS };
    } else if (c == '-') {
        return { "-", fk::lang::token_type::MINUS };
    } else if (c == '*') {
        return { "*", fk::lang::token_type::STAR };
    } else if (c == '/') {
        return { "/", fk::lang::token_type::FSLASH };
    } else if (c == '(') {
        return { "(", fk::lang::token_type::LPAREN };  
    } else if (c == ')') {
        return { ")", fk::lang::token_type::RPAREN };
    } else if (c == '<') {
        if (curr() == '=') {
            advance(); // eat the '='
            return {"<=", fk::lang::token_type::LTE};
        }
        return {"<", fk::lang::token_type::LT};
    } else if (c == '>') {
        if (curr() == '=') {
            advance(); // eat the '='
            return {">=", fk::lang::token_type::GTE};
        }
        return {">", fk::lang::token_type::GT};
    } else if (c == '=') {
        if (curr() == '=') {
            advance(); // eat the other equal
            return {"==", fk::lang::token_type::EQEQ};
        }
        return {"=", fk::lang::token_type::EQ};
    } else if (c == '!') {
        if (curr() == '=') {
            advance(); // eat the '='
            return {"!=", fk::lang::token_type::NEQ};
        }
        return {"!", fk::lang::token_type::BANG};
    } else if (c == '"') {
        return lex_string();
    } else if (c == '#') {
        skip_comment();
        return next_token();
    } else if (c == '{') {
        return { "{", fk::lang::token_type::LBRACE };
    } else if (c == '}') {
        return { "}", fk::lang::token_type::RBRACE };
    } else {
        error_handler_->report_error({"unknown token!"});
        return { "", fk::lang::token_type::UNKNOWN };
    }
}

fk::lang::token_t fk::lang::lexer_t::peek_token() noexcept
{
    size_t old_cursor = cursor_;
    
    fk::lang::token_t token = next_token();

    cursor_ = old_cursor;

    return token;
}

bool fk::lang::lexer_t::is_eof() const noexcept
{
    return cursor_ >= text_.length();
}

std::string fk::lang::lexer_t::rest() const noexcept
{
    return is_eof()
        ? ""
        : text_.substr(cursor_);   
}

void fk::lang::lexer_t::skip_whitespace() noexcept
{
    debug("LEXER: skipping whitespace\n");
    while (!is_eof() && std::isspace(text_[cursor_])) {
        advance();
    }
}

void fk::lang::lexer_t::skip_comment() noexcept
{
    while (!is_eof() && curr() != '\n') {
        advance();
    }
}

fk::lang::token_t fk::lang::lexer_t::lex_alnum(char init) noexcept
{
    debug("LEXER: lexing alnum token\n");

    std::string token(1, init);
    while (!is_eof()) {
        char c = curr();
        if (std::isalnum(c)) {
            token += c;
            advance();
        } else {
            break;
        }
    }

    fk::lang::token_type type = is_keyword(token)
        ? KEYWORDS.at(token)
        : fk::lang::token_type::IDENTIFIER;

    return { token, type };
}

fk::lang::token_t fk::lang::lexer_t::lex_string() noexcept
{
    std::string str;
    while (!is_eof()) {
        char c = advance();
        if (c == '"') {
            break;
        } else if (is_eof()) {
            error_handler_->report_error({"terminal \" not found"});
            return { str, fk::lang::token_type::UNKNOWN };
        } else {
            str += c;
        }
    }

    return { str, fk::lang::token_type::STRING };
}

fk::lang::token_t fk::lang::lexer_t::lex_number(char init) noexcept
{
    std::string num(1, init);

    bool decimal_found = false;
    while (!is_eof()) {
        char c = curr();
        if (std::isdigit(c)) {
            num += c;
            advance();
        } else if (c == '.') {
            if (decimal_found) {
                error_handler_->report_error({"'.' lexeme not expected"});
                return { ".", fk::lang::token_type::UNKNOWN };
            }
            num += c;
            decimal_found = true;
            advance();
        } else {
            break;
        }
    }

    return { num, fk::lang::token_type::NUMBER };
}

char fk::lang::lexer_t::curr() const noexcept
{
    return text_[cursor_];
}

char fk::lang::lexer_t::peek() const noexcept
{
    return text_[cursor_ + 1];
}

char fk::lang::lexer_t::advance() noexcept
{
    return text_[cursor_++];
}

bool fk::lang::lexer_t::is_keyword(const std::string& str) const noexcept
{
    return KEYWORDS.find(str) != KEYWORDS.cend();
}