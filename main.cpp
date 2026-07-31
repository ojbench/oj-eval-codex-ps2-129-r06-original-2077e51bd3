#include <charconv>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

struct Variable {
    std::string type;
    std::string value;
};

static bool parseIntValue(const std::string &token, int &value) {
    long long parsed = 0;
    const char *begin = token.data();
    const char *end = token.data() + token.size();
    auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc() || result.ptr != end) {
        return false;
    }
    if (parsed < static_cast<long long>(std::numeric_limits<int>::min()) ||
        parsed > static_cast<long long>(std::numeric_limits<int>::max())) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

static bool isQuotedString(const std::string &token) {
    return token.size() >= 2 && token.front() == '"' && token.back() == '"';
}

static std::string unquote(const std::string &token) {
    return token.substr(1, token.size() - 2);
}

static std::string_view nextToken(const std::string &line, std::size_t &position) {
    while (position < line.size() && line[position] == ' ') {
        ++position;
    }
    std::size_t start = position;
    if (start >= line.size()) {
        return {};
    }
    if (line[start] == '"') {
        ++position;
        while (position < line.size() && line[position] != '"') {
            ++position;
        }
        if (position < line.size()) {
            ++position;
        }
        return std::string_view(line).substr(start, position - start);
    }
    while (position < line.size() && line[position] != ' ') {
        ++position;
    }
    return std::string_view(line).substr(start, position - start);
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string line;
    if (!std::getline(std::cin, line)) {
        return 0;
    }

    int instructionCount = 0;
    parseIntValue(line, instructionCount);

    std::vector<std::unordered_map<std::string, Variable>> scopes(1);
    std::string output;

    auto findVariable = [&](const std::string &name) -> Variable * {
        for (auto scopeIt = scopes.rbegin(); scopeIt != scopes.rend(); ++scopeIt) {
            auto found = scopeIt->find(name);
            if (found != scopeIt->end()) {
                return &found->second;
            }
        }
        return nullptr;
    };

    auto emitInvalid = [&]() {
        output += "Invalid operation\n";
    };

    for (int step = 0; step < instructionCount; ++step) {
        if (!std::getline(std::cin, line)) {
            break;
        }
        std::size_t position = 0;
        std::string command(nextToken(line, position));
        if (command == "Indent") {
            scopes.emplace_back();
            continue;
        }
        if (command == "Dedent") {
            if (scopes.size() == 1) {
                emitInvalid();
            } else {
                scopes.pop_back();
            }
            continue;
        }

        if (command == "Declare") {
            std::string type(nextToken(line, position));
            std::string name(nextToken(line, position));
            std::string valueToken(nextToken(line, position));
            auto &currentScope = scopes.back();
            if (currentScope.find(name) != currentScope.end()) {
                emitInvalid();
                continue;
            }

            Variable variable;
            variable.type = std::move(type);
            if (variable.type == "int") {
                int parsed = 0;
                if (!parseIntValue(valueToken, parsed)) {
                    emitInvalid();
                    continue;
                }
                variable.value = std::to_string(parsed);
            } else if (variable.type == "string") {
                if (!isQuotedString(valueToken)) {
                    emitInvalid();
                    continue;
                }
                variable.value = unquote(valueToken);
            } else {
                emitInvalid();
                continue;
            }
            currentScope.emplace(std::move(name), std::move(variable));
            continue;
        }

        if (command == "Add") {
            std::string resultName(nextToken(line, position));
            std::string leftName(nextToken(line, position));
            std::string rightName(nextToken(line, position));
            Variable *resultVar = findVariable(resultName);
            Variable *leftVar = findVariable(leftName);
            Variable *rightVar = findVariable(rightName);
            if (resultVar == nullptr || leftVar == nullptr || rightVar == nullptr ||
                resultVar->type != leftVar->type || resultVar->type != rightVar->type) {
                emitInvalid();
                continue;
            }
            if (resultVar->type == "int") {
                int leftValue = 0;
                int rightValue = 0;
                parseIntValue(leftVar->value, leftValue);
                parseIntValue(rightVar->value, rightValue);
                resultVar->value = std::to_string(leftValue + rightValue);
            } else {
                resultVar->value = leftVar->value + rightVar->value;
            }
            continue;
        }

        if (command == "SelfAdd") {
            std::string name(nextToken(line, position));
            std::string valueToken(nextToken(line, position));
            Variable *var = findVariable(name);
            if (var == nullptr) {
                emitInvalid();
                continue;
            }
            if (var->type == "int") {
                int value = 0;
                if (!parseIntValue(valueToken, value)) {
                    emitInvalid();
                    continue;
                }
                int current = 0;
                parseIntValue(var->value, current);
                var->value = std::to_string(current + value);
            } else {
                if (!isQuotedString(valueToken)) {
                    emitInvalid();
                    continue;
                }
                var->value += unquote(valueToken);
            }
            continue;
        }

        if (command == "Print") {
            std::string name(nextToken(line, position));
            Variable *var = findVariable(name);
            if (var == nullptr) {
                emitInvalid();
                continue;
            }
            output += name;
            output.push_back(':');
            output += var->value;
            output.push_back('\n');
            continue;
        }
    }

    std::cout << output;
    return 0;
}
