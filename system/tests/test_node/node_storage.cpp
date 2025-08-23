#include <iostream>
#include <nodes/core/def/node_def.hpp>
#include <string>

struct StringStorage {
    std::string data;
    static constexpr bool has_storage = false;
};

NODE_DEF_OPEN_SCOPE

NODE_DECLARATION_UI(storage)
{
    return "Storage Test";
}

NODE_DECLARATION_FUNCTION(storage)
{
    b.add_input<std::string>("Input").default_val("");
}

NODE_EXECUTION_FUNCTION(storage)
{
    auto input = params.get_input<std::string>("Input");
    std::cout << "Real input: " << input << std::endl;
    if (params.get_storage<StringStorage&>().data.length() == 0) {
        params.set_storage<StringStorage>({ input });
    }
    std::cout << "Storage data: " << params.get_storage<StringStorage>().data
              << std::endl;
    return true;
}

NODE_DECLARATION_REQUIRED(storage)

NODE_DEF_CLOSE_SCOPE