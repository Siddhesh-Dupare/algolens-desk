#include "IRParser.h"
#include <json.hpp>
#include "../renderers/BaseRenderer.h"
#include "../renderers/components/array/Array.h"
#include "../renderers/components/variable/Variables.h"

using json = nlohmann::json;

std::vector<std::unique_ptr<BaseRenderer>> IRParser::buildRenderersFromIR(const std::string& jsonText) {
    std::vector<std::unique_ptr<BaseRenderer>> out;

    try {
        json doc = json::parse(jsonText);

        for (auto& c : doc["components"]) {
            std::string type = c.value("type", "");

            if (type == "array") {
                std::vector<int> vals = c["values"].get<std::vector<int>>();

                std::unordered_map<int, std::string> highlights;
                if (c.contains("highlights") && c["highlights"].is_object()) {
                    for (auto& el : c["highlights"].items()) {
                        highlights[std::stoi(el.key())] = el.value().get<std::string>();
                    }
                }

                std::vector<std::pair<std::string, int>> pointers;
                if (c.contains("pointers") && c["pointers"].is_object()) {
                    for (auto& el : c["pointers"].items()) {
                        pointers.emplace_back(el.key(), el.value().get<int>());
                    }
                }

                out.push_back(std::make_unique<Array>(
                    std::move(vals), std::move(highlights), std::move(pointers)));
            }
            else if (type == "variables") {
                std::vector<Variable> vars;
                for (auto& item : c["items"]) {
                    vars.push_back({ item.value("name", ""), item.value("value", "")});
                }
                out.push_back(std::make_unique<Variables>(std::move(vars)));
            }
        }
    } catch (const std::exception& e) {
        SDL_Log("Failed in IR Parsing: %s", e.what());
    }

    return out;
}
