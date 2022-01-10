#include "imgui/imgui.h"
#include "imgui/imgui-SFML.h"
#include "imgui/implot.h"

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "toml.hpp"

#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <string>

const int queryBufLen = 128;

struct Plot {
    std::string id;
    std::string title;
    std::vector<std::string> plotNames;
    std::vector<std::vector<float>> xvalues;
    std::vector<std::vector<float>> yvalues;
};

// From https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
std::string exec(const std::string &cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

    if (!pipe)
        throw std::runtime_error("popen() failed!");

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();

    return result;
}

std::string fileAtCommit(const std::string name) {
    return exec("git show " + name);
}

std::vector<std::string> getCommitsByQuery(const std::string &query) {
    std::string result = exec("git log --format=format:\"%H\" " + query);

    std::vector<std::string> ids;

    std::istringstream is(result);

    std::string line;

    while (std::getline(is, line)) {
        std::string id = line;

        ids.push_back(id);   
    }

    return ids;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "ExpView requires a single argument, the path to the directory containing the expview.toml file" << std::endl;

        return 1;
    }

    std::string dir = argv[1];

    std::filesystem::path path = std::filesystem::absolute(std::filesystem::path(dir));

    std::cout << "Starting ExpView in " << path << "." << std::endl;

    // Change working directory
    std::filesystem::current_path(path);

    // --------------------------- Window ----------------------------

    sf::RenderWindow window;
    
    window.create(sf::VideoMode(1280, 720), "ExpView", sf::Style::Default);

    window.setVerticalSyncEnabled(true);

    ImGui::SFML::Init(window);

    ImPlot::CreateContext();

    // -------------------------- Permanent -------------------------

    std::array<char, queryBufLen> queryBuf;

    for (int i = 0; i < queryBufLen; i++)
        queryBuf[i] = '\0';

    std::vector<Plot> plots;

    // ---------------------------- Loop ----------------------------

    sf::Clock deltaClock;

    while (window.isOpen()) {
        sf::Event event;

        int mouseWheelDelta = 0;

        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;

                case sf::Event::MouseWheelMoved:
                    mouseWheelDelta = event.mouseWheel.delta;
                    break;
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Query");

        ImGui::InputText("Query", queryBuf.data(), queryBufLen);

        if (ImGui::Button("Find commits")) {
            std::vector<std::string> ids = getCommitsByQuery(queryBuf.data());

            // Load expview.toml at the commits
            plots.clear();
            
            for (int i = 0; i < ids.size(); i++) {
                std::string txt = fileAtCommit(ids[i] + ":expview.toml");

                if (txt.empty())
                    continue;

                Plot p;
                p.id = ids[i];

                toml::table tbl;

                try {
                    tbl = toml::parse(txt);
                }
                catch (const toml::parse_error &err) {
                   std::cerr << "Parsing expview.toml failed:" << std::endl << err << std::endl;

                   continue;
                }

                std::optional<std::string> opt_title = tbl["title"].value<std::string>();
                toml::array* opt_plotNames = tbl["names"].as_array();
                toml::array* opt_plots = tbl["plots"].as_array();

                std::string plotTitle = "Untitled";

                if (opt_title)
                    p.title = *opt_title;

                if (!opt_plots || !opt_plotNames) {
                    std::cout << "No plots found in expview.toml for commit " << ids[i] << std::endl;

                    continue;
                }

                toml::array names = *opt_plotNames;
                toml::array arrs = *opt_plots;

                if (names.size() != arrs.size()) {
                    std::cout << "Fields \"names\" and \"plots\" must match in length: expview.toml for commit " << ids[i] << std::endl;

                    continue;
                }

                p.plotNames.resize(arrs.size());
                p.xvalues.resize(arrs.size());
                p.yvalues.resize(arrs.size());

                for (int j = 0; j < arrs.size(); j++) {
                    std::optional<std::string> opt_name = names[j].value<std::string>();

                    if (opt_name)
                        p.plotNames[j] = *opt_name;    

                    toml::array* opt_xyvalues = arrs[j].as_array();

                    if (opt_xyvalues) {
                        toml::array xyvalues = *opt_xyvalues;

                        p.xvalues[j].resize(xyvalues.size());
                        p.yvalues[j].resize(xyvalues.size());

                        for (int k = 0; k < xyvalues.size(); k++) {
                            toml::array* opt_coord = xyvalues[k].as_array();

                            if (opt_coord) {
                                toml::array coord = *opt_coord;

                                if (coord.size() != 2) {
                                    std::cout << "Error: Incorrect formatting in coordinates: expview.toml for commit " << ids[i] << std::endl;

                                    return 1;
                                }

                                std::optional<float> opt_x = coord[0].value<float>();
                                std::optional<float> opt_y = coord[1].value<float>();

                                if (opt_x && opt_y) {
                                    p.xvalues[j][k] = *opt_x;
                                    p.yvalues[j][k] = *opt_y;
                                }
                            }
                        }
                    }
                }

                plots.push_back(p);
            }
        }

        ImGui::End();

        ImGui::Begin("Plot browser");

        for (int p = 0; p < plots.size(); p++) {
            if (ImPlot::BeginPlot((plots[p].title + " (" + plots[p].id + ")").c_str())) {
                for (int c = 0; c < plots[p].yvalues.size(); c++)
                    ImPlot::PlotLine(plots[p].plotNames[c].c_str(), plots[p].xvalues[c].data(), plots[p].yvalues[c].data(), plots[p].xvalues[c].size());

                ImPlot::EndPlot();
            }
        }

        ImGui::End();

        window.clear(sf::Color(50, 50, 50));

        ImGui::SFML::Render();

        window.display();
    }

    ImPlot::DestroyContext();
    ImGui::SFML::Shutdown();

    std:: cout << "Bye!" << std::endl;
}
