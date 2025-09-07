#include <iostream>
#include <lexer.hpp>
#include <parser.hpp>
#include <vector>

static void print_help()
{
    std::cout << "Usage: luao [options] [script [args]]\n"
                 "Options:\n";
}

int main(int argc, char **argv)
{
    std::vector<std::string> test_cases = {
        // 単一の代入文
        "x = 42",
        // 複数変数の代入
        "x, y = 1, 2",
        // if文（elseなし）
        "if true then x = 10 end",
        // if文（elseあり）
        "if x > 0 then x = 1 else x = 0 end",
        // while文
        "while x < 10 do x = x + 1 end",
        // 複雑な式（数値、文字列、単項/二項演算子、括弧）
        "result = -(a + b * 2) / (3 - 1) .. \"test\""
        // break文
        ,"while true do break end"};

    for (size_t i = 0; i < test_cases.size(); ++i)
    {
        std::cout << "=== Test Case " << (i + 1) << ": " << test_cases[i] << " ===\n";
        try
        {
            Lexer lexer(test_cases[i]);
            std::vector<TokenInfo> tokens;
            while (true)
            {
                TokenInfo token = lexer.nextToken();
                tokens.push_back(token);
                if (token.type == Token::EOS)
                    break;
            }

            Parser parser(std::move(tokens));
            auto ast = parser.parse();
            std::cout << "AST Dump:\n"
                      << ast->dump(0) << "\n\n";
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Error: " << e.what() << "\n\n";
        }
    }

    return 0;
}