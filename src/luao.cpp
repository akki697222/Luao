#include <iostream>
#include <lolex.hpp>

static void print_help()
{
    std::cout << "Usage: luao [options] [script [args]]\n"
                 "Options:\n";
}

int main(int argc, char **argv)
{
    std::string source = R"(
        local x<const> = 42;
        for k, v each t do
            throw RuntimeError("Test!!");
        end
        -- single line comment
        --[[ 
        long comment 
        ]]
    )";

    Lexer lexer(source);
    try
    {
        while (true)
        {
            TokenInfo token = lexer.nextToken();
            int tokenIndex = static_cast<int>(token.type);
            std::cout << "Token: " << TokenNames[tokenIndex] 
                      << "("  << tokenIndex << ")"
                      << ", Value: \"" << token.value
                      << "\", Line: " << token.line << std::endl;
            if (token.type == Token::EOS)
                break;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}