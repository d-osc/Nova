#include <gtest/gtest.h>
#include "nova/Frontend/Lexer.h"
#include "nova/Frontend/Token.h"

using namespace nova;

TEST(LexerTest, BasicTokens) {
    Lexer lexer("let x = 42;");
    
    auto token1 = lexer.nextToken();
    EXPECT_EQ(token1.type, TokenType::Let);
    
    auto token2 = lexer.nextToken();
    EXPECT_EQ(token2.type, TokenType::Identifier);
    EXPECT_EQ(token2.value, "x");
    
    auto token3 = lexer.nextToken();
    EXPECT_EQ(token3.type, TokenType::Equal);
    
    auto token4 = lexer.nextToken();
    EXPECT_EQ(token4.type, TokenType::Number);
    EXPECT_EQ(token4.value, "42");
    
    auto token5 = lexer.nextToken();
    EXPECT_EQ(token5.type, TokenType::Semicolon);
    
    auto token6 = lexer.nextToken();
    EXPECT_EQ(token6.type, TokenType::EndOfFile);
}

TEST(LexerTest, Keywords) {
    Lexer lexer("const function class interface");
    
    EXPECT_EQ(lexer.nextToken().type, TokenType::Const);
    EXPECT_EQ(lexer.nextToken().type, TokenType::Function);
    EXPECT_EQ(lexer.nextToken().type, TokenType::Class);
    EXPECT_EQ(lexer.nextToken().type, TokenType::Interface);
}

TEST(LexerTest, Operators) {
    Lexer lexer("+ - * / === !== ??");
    
    EXPECT_EQ(lexer.nextToken().type, TokenType::Plus);
    EXPECT_EQ(lexer.nextToken().type, TokenType::Minus);
    EXPECT_EQ(lexer.nextToken().type, TokenType::Star);
    EXPECT_EQ(lexer.nextToken().type, TokenType::Slash);
    EXPECT_EQ(lexer.nextToken().type, TokenType::StrictEqual);
    EXPECT_EQ(lexer.nextToken().type, TokenType::StrictNotEqual);
    EXPECT_EQ(lexer.nextToken().type, TokenType::NullishCoalescing);
}

TEST(LexerTest, Strings) {
    Lexer lexer("\"hello\" 'world' `template`");
    
    auto token1 = lexer.nextToken();
    EXPECT_EQ(token1.type, TokenType::String);
    EXPECT_EQ(token1.value, "hello");
    
    auto token2 = lexer.nextToken();
    EXPECT_EQ(token2.type, TokenType::String);
    EXPECT_EQ(token2.value, "world");
    
    auto token3 = lexer.nextToken();
    EXPECT_EQ(token3.type, TokenType::TemplateLiteral);
}

TEST(LexerTest, Numbers) {
    Lexer lexer("42 3.14 0x1A 0b1010 123n");
    
    auto token1 = lexer.nextToken();
    EXPECT_EQ(token1.type, TokenType::Number);
    EXPECT_EQ(token1.value, "42");
    
    auto token2 = lexer.nextToken();
    EXPECT_EQ(token2.type, TokenType::Number);
    EXPECT_EQ(token2.value, "3.14");
    
    auto token3 = lexer.nextToken();
    EXPECT_EQ(token3.type, TokenType::Number);
    EXPECT_EQ(token3.value, "0x1A");
    
    auto token4 = lexer.nextToken();
    EXPECT_EQ(token4.type, TokenType::Number);
    EXPECT_EQ(token4.value, "0b1010");
    
    auto token5 = lexer.nextToken();
    EXPECT_EQ(token5.type, TokenType::BigInt);
    EXPECT_EQ(token5.value, "123n");
}
