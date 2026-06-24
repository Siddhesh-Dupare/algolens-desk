#include "IRParser.h"
#include <json.hpp>
#include "../renderers/BaseRenderer.h"
#include "../renderers/components/array/Array.h"
#include "../renderers/components/variable/Variables.h"
#include "../renderers/components/grid/Grid.h"
#include "../renderers/components/stack/Stack.h"
#include "../renderers/components/queue/Queue.h"
#include "../renderers/components/linkedlist/LinkedList.h"
#include "../renderers/components/tree/Tree.h"
#include "../renderers/components/graph/Graph.h"
#include "../renderers/components/hashmap/HashMap.h"

using json = nlohmann::json;

// Build renderers for one scene's component array (shared by main + parked).
static std::vector<std::unique_ptr<BaseRenderer>> buildFromComponents(const json& comps) {
    std::vector<std::unique_ptr<BaseRenderer>> out;
    if (!comps.is_array()) return out;

    for (auto& c : comps) {
        {
            std::string type = c.value("type", "");

            if (type == "array") {
                // Values are display strings; accept JSON numbers or strings.
                std::vector<std::string> vals;
                for (auto& el : c["values"]) {
                    vals.push_back(el.is_string() ? el.get<std::string>() : el.dump());
                }

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
            else if (type == "grid") {
                // 2-D values as display strings (accept JSON numbers or strings).
                std::vector<std::vector<std::string>> grid;
                for (auto& row : c["values"]) {
                    std::vector<std::string> r;
                    for (auto& el : row) {
                        r.push_back(el.is_string() ? el.get<std::string>() : el.dump());
                    }
                    grid.push_back(std::move(r));
                }

                std::unordered_map<std::string, std::string> highlights;  // "r,c" -> state
                if (c.contains("highlights") && c["highlights"].is_object()) {
                    for (auto& el : c["highlights"].items()) {
                        highlights[el.key()] = el.value().get<std::string>();
                    }
                }

                std::vector<std::pair<std::string, std::string>> pointers;  // name -> "r,c"
                if (c.contains("pointers") && c["pointers"].is_object()) {
                    for (auto& el : c["pointers"].items()) {
                        pointers.emplace_back(el.key(), el.value().get<std::string>());
                    }
                }

                out.push_back(std::make_unique<Grid>(
                    std::move(grid), std::move(highlights), std::move(pointers)));
            }
            else if (type == "stack" || type == "queue") {
                // Both are 1-D: values (display strings) + int-keyed highlights.
                std::vector<std::string> vals;
                for (auto& el : c["values"]) {
                    vals.push_back(el.is_string() ? el.get<std::string>() : el.dump());
                }

                std::unordered_map<int, std::string> highlights;
                if (c.contains("highlights") && c["highlights"].is_object()) {
                    for (auto& el : c["highlights"].items()) {
                        highlights[std::stoi(el.key())] = el.value().get<std::string>();
                    }
                }

                if (type == "stack") {
                    const int top = c.value("top", (int)vals.size() - 1);
                    out.push_back(std::make_unique<Stack>(
                        std::move(vals), std::move(highlights), top));
                } else {
                    const int front = c.value("front", vals.empty() ? -1 : 0);
                    const int rear = c.value("rear", (int)vals.size() - 1);
                    out.push_back(std::make_unique<Queue>(
                        std::move(vals), std::move(highlights), front, rear));
                }
            }
            else if (type == "linkedlist") {
                std::vector<std::string> vals;
                for (auto& el : c["values"]) {
                    vals.push_back(el.is_string() ? el.get<std::string>() : el.dump());
                }

                std::unordered_map<int, std::string> highlights;
                if (c.contains("highlights") && c["highlights"].is_object()) {
                    for (auto& el : c["highlights"].items()) {
                        highlights[std::stoi(el.key())] = el.value().get<std::string>();
                    }
                }

                std::vector<std::pair<std::string, int>> pointers;  // label -> index
                if (c.contains("pointers") && c["pointers"].is_object()) {
                    for (auto& el : c["pointers"].items()) {
                        pointers.emplace_back(el.key(), el.value().get<int>());
                    }
                }

                const bool doubly = c.value("doubly", false);
                out.push_back(std::make_unique<LinkedList>(
                    std::move(vals), std::move(highlights), std::move(pointers), doubly));
            }
            else if (type == "tree") {
                std::vector<TreeNodeData> nodes;
                if (c.contains("nodes") && c["nodes"].is_array()) {
                    for (auto& nd : c["nodes"]) {
                        TreeNodeData node;
                        const auto& val = nd["value"];
                        node.value = val.is_string() ? val.get<std::string>() : val.dump();
                        if (nd.contains("children") && nd["children"].is_array()) {
                            for (auto& ch : nd["children"]) node.children.push_back(ch.get<int>());
                        }
                        nodes.push_back(std::move(node));
                    }
                }

                std::unordered_map<int, std::string> highlights;
                if (c.contains("highlights") && c["highlights"].is_object()) {
                    for (auto& el : c["highlights"].items()) {
                        highlights[std::stoi(el.key())] = el.value().get<std::string>();
                    }
                }

                std::vector<std::pair<std::string, int>> pointers;  // label -> index
                if (c.contains("pointers") && c["pointers"].is_object()) {
                    for (auto& el : c["pointers"].items()) {
                        pointers.emplace_back(el.key(), el.value().get<int>());
                    }
                }

                const bool binary = c.value("binary", true);
                out.push_back(std::make_unique<Tree>(
                    std::move(nodes), binary, std::move(highlights), std::move(pointers)));
            }
            else if (type == "graph") {
                std::vector<std::string> nodes;
                for (auto& el : c["nodes"]) {
                    nodes.push_back(el.is_string() ? el.get<std::string>() : el.dump());
                }

                std::vector<GraphEdge> edges;
                if (c.contains("edges") && c["edges"].is_array()) {
                    for (auto& e : c["edges"]) {
                        if (!e.is_array() || e.size() < 2) continue;
                        GraphEdge ge;
                        ge.from = e[0].get<int>();
                        ge.to = e[1].get<int>();
                        if (e.size() > 2) {
                            ge.weight = e[2].is_string() ? e[2].get<std::string>() : e[2].dump();
                        }
                        edges.push_back(std::move(ge));
                    }
                }

                std::unordered_map<int, std::string> highlights;
                if (c.contains("highlights") && c["highlights"].is_object()) {
                    for (auto& el : c["highlights"].items()) {
                        highlights[std::stoi(el.key())] = el.value().get<std::string>();
                    }
                }

                std::vector<std::pair<std::string, int>> pointers;  // label -> index
                if (c.contains("pointers") && c["pointers"].is_object()) {
                    for (auto& el : c["pointers"].items()) {
                        pointers.emplace_back(el.key(), el.value().get<int>());
                    }
                }

                const bool directed = c.value("directed", true);
                out.push_back(std::make_unique<Graph>(
                    std::move(nodes), std::move(edges), directed,
                    std::move(highlights), std::move(pointers)));
            }
            else if (type == "hashmap") {
                std::vector<std::pair<std::string, std::string>> entries;
                if (c.contains("entries") && c["entries"].is_array()) {
                    for (auto& e : c["entries"]) {
                        if (!e.is_array() || e.size() < 2) continue;
                        std::string k = e[0].is_string() ? e[0].get<std::string>() : e[0].dump();
                        std::string val = e[1].is_string() ? e[1].get<std::string>() : e[1].dump();
                        entries.emplace_back(std::move(k), std::move(val));
                    }
                }

                std::unordered_map<int, std::string> highlights;
                if (c.contains("highlights") && c["highlights"].is_object()) {
                    for (auto& el : c["highlights"].items()) {
                        highlights[std::stoi(el.key())] = el.value().get<std::string>();
                    }
                }

                out.push_back(std::make_unique<HashMap>(
                    std::move(entries), std::move(highlights)));
            }
        }
    }

    return out;
}

std::vector<std::unique_ptr<BaseRenderer>> IRParser::buildRenderersFromIR(const std::string& jsonText) {
    try {
        json doc = json::parse(jsonText);
        if (doc.contains("components")) return buildFromComponents(doc["components"]);
    } catch (const std::exception& e) {
        SDL_Log("Failed in IR Parsing: %s", e.what());
    }
    return {};
}

Scene IRParser::buildSceneFromIR(const std::string& jsonText) {
    Scene scene;
    try {
        json doc = json::parse(jsonText);
        if (doc.contains("components")) scene.main = buildFromComponents(doc["components"]);
        if (doc.contains("parked") && doc["parked"].is_array()) {
            for (auto& p : doc["parked"]) {
                ParkedScene ps;
                ps.label = p.value("label", "");
                if (p.contains("components")) ps.renderers = buildFromComponents(p["components"]);
                scene.parked.push_back(std::move(ps));
            }
        }
    } catch (const std::exception& e) {
        SDL_Log("Failed in IR Parsing: %s", e.what());
    }
    return scene;
}
