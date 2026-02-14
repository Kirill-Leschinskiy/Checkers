#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        reload();
    }

    void reload() // загружает настройки из файла settings.json
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }
    // оператор () позволяет получать нужную настройку из файла settings.json
    // по setting_dir (например, WindowSize) и setting_name (например, Width)
    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        return config[setting_dir][setting_name];
    }

  private:
    json config; // сохраняет в config
};
